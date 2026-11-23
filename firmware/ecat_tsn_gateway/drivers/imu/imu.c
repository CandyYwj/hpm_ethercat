/*
 * Author: BearKing
 */
#include "imu.h"

#include "FreeRTOS.h"
#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_common.h"
#include "hpm_dmav2_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_interrupt.h"
#include "hpm_uart_drv.h"
#include "task.h"
#include "yesense_parser.h"
#include "yesense_query.h"

#include <stddef.h>
#include <string.h>

#define IMU_UART                       HPM_UART12
#define IMU_UART_CLOCK                 clock_uart12
#define IMU_UART_BAUD_RATE             460800U

#define IMU_DMA                        HPM_HDMA
#define IMU_DMAMUX                     HPM_DMAMUX
#define IMU_DMA_RX_CHANNEL             30U
#define IMU_DMA_RX_DMAMUX_CHANNEL      DMA_SOC_CHN_TO_DMAMUX_CHN(IMU_DMA, IMU_DMA_RX_CHANNEL)
#define IMU_DMA_RX_REQUEST             HPM_DMA_SRC_UART12_RX
#define IMU_DMA_IRQ                    IRQn_HDMA
#define IMU_DMA_IRQ_PRIORITY           2U
#define IMU_DMA_BUFFER_SIZE            2048U

#define IMU_RX_READ_SIZE_BYTES         128U
#define IMU_RX_MAX_READS_PER_POLL      4U

#define IMU_QUERY_TIMEOUT_TICKS         pdMS_TO_TICKS(50U)

typedef enum {
    IMU_QUERY_OK = 0,
    IMU_QUERY_SEND_ERROR,
    IMU_QUERY_TIMEOUT,
} imu_query_status;

typedef struct {
    uint8_t data_class;
    const uint8_t *payload;
    uint16_t payload_size_bytes;
    const char *name;
} imu_query_request;

typedef struct {
    uint32_t sent_bytes;
    uint32_t received_bytes;
    uint32_t streaming_frames;
    uint32_t query_checksum_errors;
    uint32_t query_format_errors;
} imu_query_diagnostics;

typedef struct {
    volatile uint32_t dma_wrap_count;
    volatile uint32_t dma_error_count;
    uint32_t dma_consumed_bytes;
    uint32_t observed_overrun_count;
    yesense_parser_t parser;
    imu_sample_t latest_sample;
    imu_stats_t stats;
    bool has_sample;
} imu_runtime_state;

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4)
static uint8_t s_dma_buffer[IMU_DMA_BUFFER_SIZE];

static imu_runtime_state s_imu;

static imu_query_status query_device(uint8_t data_class,
                                     const uint8_t *request_data,
                                     uint16_t request_size_bytes,
                                     yesense_query_response_t *response,
                                     imu_query_diagnostics *diagnostics)
{
    uint8_t query_frame[YESENSE_QUERY_MAX_FRAME_SIZE];
    size_t frame_size_bytes;
    TickType_t start_tick;
    yesense_parser_t streaming_parser;
    yesense_query_parser_t query_parser;

    memset(diagnostics, 0, sizeof(*diagnostics));
    frame_size_bytes = yesense_query_build(data_class,
                                           request_data,
                                           request_size_bytes,
                                           query_frame,
                                           sizeof(query_frame));
    if ((frame_size_bytes == 0U)
        || (uart_send_data(IMU_UART, query_frame, (uint32_t)frame_size_bytes) != status_success)
        || (uart_flush(IMU_UART) != status_success)) {
        return IMU_QUERY_SEND_ERROR;
    }
    diagnostics->sent_bytes = (uint32_t)frame_size_bytes;

    yesense_parser_init(&streaming_parser);
    yesense_query_parser_init(&query_parser, data_class);
    start_tick = xTaskGetTickCount();
    while ((TickType_t)(xTaskGetTickCount() - start_tick) < IMU_QUERY_TIMEOUT_TICKS) {
        uint8_t received_byte;

        if (uart_try_receive_byte(IMU_UART, &received_byte) == status_success) {
            imu_sample_t streaming_sample;
            yesense_parse_result_t streaming_result;
            yesense_query_parse_result_t query_result;

            diagnostics->received_bytes++;
            streaming_result = yesense_parser_push(&streaming_parser,
                                                    received_byte,
                                                    &streaming_sample);
            if (streaming_result == yesense_parse_frame) {
                diagnostics->streaming_frames++;
            }

            query_result = yesense_query_parser_push(&query_parser, received_byte, response);
            if (query_result == yesense_query_parse_response) {
                return IMU_QUERY_OK;
            } else if (query_result == yesense_query_parse_checksum_error) {
                diagnostics->query_checksum_errors++;
            } else if (query_result == yesense_query_parse_format_error) {
                diagnostics->query_format_errors++;
            }
        } else {
            taskYIELD();
        }
    }

    return IMU_QUERY_TIMEOUT;
}

static const char *imu_frequency_name(uint8_t output_rate_code)
{
    switch (output_rate_code) {
    case 0x01U:
        return "1 Hz";
    case 0x02U:
        return "2 Hz";
    case 0x03U:
        return "5 Hz";
    case 0x04U:
        return "10 Hz";
    case 0x05U:
        return "20 Hz";
    case 0x06U:
        return "25 Hz";
    case 0x07U:
        return "50 Hz";
    case 0x08U:
    case 0x0EU:
        return "100 Hz";
    case 0x09U:
        return "200 Hz";
    case 0x0AU:
        return "250/400 Hz";
    case 0x0BU:
        return "400/800 Hz";
    case 0x0CU:
        return "1000/1600 Hz";
    case 0x0DU:
        return "2000/3200 Hz";
    default:
        return "unknown";
    }
}

static const char *imu_algorithm_name(uint8_t algorithm_mode)
{
    switch (algorithm_mode) {
    case 0x01U:
        return "AHRS";
    case 0x02U:
        return "VRU";
    case 0x03U:
        return "IMU";
    default:
        return "unknown";
    }
}

static void imu_print_device_info(const yesense_device_info_t *device_info)
{
    printf("YESENSE device information:\r\n");
    if ((device_info->valid_fields & YESENSE_DEVICE_INFO_PRODUCT) != 0U) {
        printf("  Product      : %s\r\n", device_info->product);
    } else {
        printf("  Product      : query failed\r\n");
    }

    if ((device_info->valid_fields & YESENSE_DEVICE_INFO_BAUD_RATE) != 0U) {
        if (device_info->baud_rate != 0U) {
            printf("  Baud rate    : %lu bps (code 0x%02X)\r\n",
                   (unsigned long)device_info->baud_rate,
                   (unsigned int)device_info->baud_rate_code);
        } else {
            printf("  Baud rate    : unknown (code 0x%02X)\r\n",
                   (unsigned int)device_info->baud_rate_code);
        }
    } else {
        printf("  Baud rate    : query failed\r\n");
    }

    if ((device_info->valid_fields & YESENSE_DEVICE_INFO_OUTPUT_FREQUENCY) != 0U) {
        printf("  Output rate  : %s (code 0x%02X)\r\n",
               imu_frequency_name(device_info->output_frequency_code),
               (unsigned int)device_info->output_frequency_code);
    } else {
        printf("  Output rate  : query failed\r\n");
    }

    if ((device_info->valid_fields & YESENSE_DEVICE_INFO_OUTPUT_CONTENT) != 0U) {
        uint16_t output_content_mask = device_info->output_content;

        printf("  Output mask  : 0x%04X ts=%u temp=%u acc=%u gyro=%u mag=%u euler=%u quat=%u\r\n",
               (unsigned int)output_content_mask,
               (unsigned int)((output_content_mask >> 15) & 1U),
               (unsigned int)((output_content_mask >> 10) & 1U),
               (unsigned int)((output_content_mask >> 7) & 1U),
               (unsigned int)((output_content_mask >> 6) & 1U),
               (unsigned int)((output_content_mask >> 5) & 1U),
               (unsigned int)((output_content_mask >> 4) & 1U),
               (unsigned int)((output_content_mask >> 3) & 1U));
    } else {
        printf("  Output mask  : query failed\r\n");
    }

    if ((device_info->valid_fields & YESENSE_DEVICE_INFO_ALGORITHM_MODE) != 0U) {
        printf("  Algorithm    : %s (code 0x%02X)\r\n",
               imu_algorithm_name(device_info->algorithm_mode),
               (unsigned int)device_info->algorithm_mode);
    } else {
        printf("  Algorithm    : query failed\r\n");
    }
}

static void imu_query_device_info(void)
{
    static const uint8_t product_info_subtype = YESENSE_QUERY_SUBTYPE_PRODUCT_INFO;
    static const uint8_t algorithm_mode_subtype = YESENSE_QUERY_SUBTYPE_ALGORITHM_MODE;
    static const imu_query_request requests[] = {
        {YESENSE_QUERY_CLASS_PRODUCT, &product_info_subtype, 1U, "product"},
        {YESENSE_QUERY_CLASS_BAUD_RATE, NULL, 0U, "baud rate"},
        {YESENSE_QUERY_CLASS_OUTPUT_FREQUENCY, NULL, 0U, "output frequency"},
        {YESENSE_QUERY_CLASS_OUTPUT_CONTENT, NULL, 0U, "output content"},
        {YESENSE_QUERY_CLASS_ALGORITHM_MODE, &algorithm_mode_subtype, 1U, "algorithm mode"},
    };
    yesense_device_info_t device_info = {0};

    for (size_t request_index = 0U;
         request_index < (sizeof(requests) / sizeof(requests[0]));
         request_index++) {
        yesense_query_response_t response;
        imu_query_diagnostics diagnostics;
        const imu_query_request *request = &requests[request_index];
        imu_query_status query_status = query_device(request->data_class,
                                                     request->payload,
                                                     request->payload_size_bytes,
                                                     &response,
                                                     &diagnostics);

        if (query_status == IMU_QUERY_SEND_ERROR) {
            printf("IMU query %s send failed\r\n", request->name);
        } else if (query_status == IMU_QUERY_TIMEOUT) {
            printf("IMU query %s timed out: tx=%lu rx=%lu stream=%lu qck=%lu qfmt=%lu; "
                   "no query reply, check PF16->IMU RX path/interface\r\n",
                   request->name,
                   (unsigned long)diagnostics.sent_bytes,
                   (unsigned long)diagnostics.received_bytes,
                   (unsigned long)diagnostics.streaming_frames,
                   (unsigned long)diagnostics.query_checksum_errors,
                   (unsigned long)diagnostics.query_format_errors);
        } else if (!yesense_device_info_apply(&device_info, &response)) {
            printf("IMU query %s returned invalid data\r\n", request->name);
        }
    }

    imu_print_device_info(&device_info);
}

static void imu_dma_collect_status(void)
{
    uint32_t transfer_status = dma_check_transfer_status(IMU_DMA, IMU_DMA_RX_CHANNEL);

    if ((transfer_status & DMA_CHANNEL_STATUS_TC) != 0U) {
        s_imu.dma_wrap_count++;
    }
    if ((transfer_status & (DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) != 0U) {
        s_imu.dma_error_count++;
    }
}

SDK_DECLARE_EXT_ISR_M(IMU_DMA_IRQ, imu_dma_isr)
void imu_dma_isr(void)
{
    imu_dma_collect_status();
}

static hpm_stat_t imu_uart_init(void)
{
    uart_config_t uart_config = {0};

    HPM_IOC->PAD[IOC_PAD_PF16].FUNC_CTL = IOC_PF16_FUNC_CTL_UART12_TXD;
    HPM_IOC->PAD[IOC_PAD_PF17].FUNC_CTL = IOC_PF17_FUNC_CTL_UART12_RXD;

    clock_set_source_divider(IMU_UART_CLOCK, clk_src_pll1_clk0, 10U);
    clock_add_to_group(IMU_UART_CLOCK, 0U);

    uart_default_config(IMU_UART, &uart_config);
    uart_config.baudrate = IMU_UART_BAUD_RATE;
    uart_config.fifo_enable = true;
    uart_config.dma_enable = true;
    uart_config.src_freq_in_hz = clock_get_frequency(IMU_UART_CLOCK);
    uart_config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
    return uart_init(IMU_UART, &uart_config);
}

static hpm_stat_t imu_dma_init(void)
{
    dma_handshake_config_t dma_config;

    dma_default_handshake_config(IMU_DMA, &dma_config);
    dma_config.ch_index = IMU_DMA_RX_CHANNEL;
    dma_config.dst = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)s_dma_buffer);
    dma_config.dst_fixed = false;
    dma_config.src = (uint32_t)&IMU_UART->RBR;
    dma_config.src_fixed = true;
    dma_config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    dma_config.size_in_byte = IMU_DMA_BUFFER_SIZE;
    dma_config.en_infiniteloop = true;
    dma_config.interrupt_mask = DMA_INTERRUPT_MASK_HALF_TC;

    dmamux_config(IMU_DMAMUX, IMU_DMA_RX_DMAMUX_CHANNEL, IMU_DMA_RX_REQUEST, true);
    return dma_setup_handshake(IMU_DMA, &dma_config, true);
}

static uint32_t imu_dma_get_produced_bytes(void)
{
    uint32_t irq_state;
    uint32_t write_position;
    uint32_t completed_wraps;

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    for (;;) {
        imu_dma_collect_status();
        completed_wraps = s_imu.dma_wrap_count;
        write_position = IMU_DMA_BUFFER_SIZE
                       - dma_get_remaining_transfer_size(IMU_DMA, IMU_DMA_RX_CHANNEL);
        if ((IMU_DMA->INTTCSTS & (1UL << IMU_DMA_RX_CHANNEL)) == 0U) {
            break;
        }
    }
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);

    return (completed_wraps * IMU_DMA_BUFFER_SIZE) + write_position;
}

static size_t imu_dma_read(uint8_t *destination, size_t capacity_bytes)
{
    uint32_t produced_bytes;
    uint32_t available_bytes;
    uint32_t read_index;
    size_t read_size_bytes;
    size_t first_size_bytes;

    produced_bytes = imu_dma_get_produced_bytes();
    available_bytes = produced_bytes - s_imu.dma_consumed_bytes;
    if (available_bytes >= IMU_DMA_BUFFER_SIZE) {
        s_imu.dma_consumed_bytes = produced_bytes;
        s_imu.stats.dma_overruns++;
        return 0U;
    }

    read_size_bytes = (available_bytes < capacity_bytes)
                    ? (size_t)available_bytes
                    : capacity_bytes;
    read_index = s_imu.dma_consumed_bytes % IMU_DMA_BUFFER_SIZE;
    first_size_bytes = IMU_DMA_BUFFER_SIZE - read_index;
    if (first_size_bytes > read_size_bytes) {
        first_size_bytes = read_size_bytes;
    }

    memcpy(destination, &s_dma_buffer[read_index], first_size_bytes);
    if (read_size_bytes > first_size_bytes) {
        memcpy(&destination[first_size_bytes],
               s_dma_buffer,
               read_size_bytes - first_size_bytes);
    }
    s_imu.dma_consumed_bytes += (uint32_t)read_size_bytes;
    return read_size_bytes;
}

static void imu_publish_sample(const imu_sample_t *sample)
{
    uint32_t irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);

    s_imu.latest_sample = *sample;
    s_imu.has_sample = true;
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
}

bool imu_init(void)
{
    memset(&s_imu, 0, sizeof(s_imu));
    yesense_parser_init(&s_imu.parser);

    if (imu_uart_init() != status_success) {
        return false;
    }
    imu_query_device_info();
    if (imu_dma_init() != status_success) {
        return false;
    }

    intc_m_enable_irq_with_priority(IMU_DMA_IRQ, IMU_DMA_IRQ_PRIORITY);
    return true;
}

void imu_process_rx(void)
{
    uint8_t received_bytes[IMU_RX_READ_SIZE_BYTES];
    uint32_t read_batch_index;

    for (read_batch_index = 0U;
         read_batch_index < IMU_RX_MAX_READS_PER_POLL;
         read_batch_index++) {
        size_t received_size_bytes = imu_dma_read(received_bytes, sizeof(received_bytes));

        if (s_imu.stats.dma_overruns != s_imu.observed_overrun_count) {
            s_imu.observed_overrun_count = s_imu.stats.dma_overruns;
            yesense_parser_init(&s_imu.parser);
        }
        if (received_size_bytes == 0U) {
            break;
        }

        s_imu.stats.received_bytes += (uint32_t)received_size_bytes;
        for (size_t byte_index = 0U; byte_index < received_size_bytes; byte_index++) {
            imu_sample_t sample;
            yesense_parse_result_t parse_result;

            parse_result = yesense_parser_push(&s_imu.parser,
                                               received_bytes[byte_index],
                                               &sample);
            if (parse_result == yesense_parse_frame) {
                sample.received_tick = (uint32_t)xTaskGetTickCount();
                imu_publish_sample(&sample);
                s_imu.stats.valid_frames++;
                s_imu.stats.last_frame_tick = sample.received_tick;
            } else if (parse_result == yesense_parse_checksum_error) {
                s_imu.stats.checksum_errors++;
            } else if (parse_result == yesense_parse_format_error) {
                s_imu.stats.format_errors++;
            }
        }

        if (received_size_bytes < sizeof(received_bytes)) {
            break;
        }
    }
}

bool imu_get_latest(imu_sample_t *sample)
{
    bool has_sample;
    uint32_t irq_state;

    if (sample == NULL) {
        return false;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    has_sample = s_imu.has_sample;
    if (has_sample) {
        *sample = s_imu.latest_sample;
    }
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
    return has_sample;
}

void imu_get_stats(imu_stats_t *stats)
{
    uint32_t irq_state;

    if (stats == NULL) {
        return;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    *stats = s_imu.stats;
    stats->dma_errors = s_imu.dma_error_count;
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
}

void imu_print_status(void)
{
    static uint32_t previous_frame_count;
    static TickType_t previous_rate_tick;
    imu_stats_t stats;
    imu_sample_t sample;
    TickType_t now_tick = xTaskGetTickCount();
    float measured_rate_hz = 0.0f;

    imu_get_stats(&stats);
    if (previous_rate_tick != 0U) {
        uint32_t frame_delta = stats.valid_frames - previous_frame_count;
        TickType_t tick_delta = now_tick - previous_rate_tick;

        if (tick_delta != 0U) {
            measured_rate_hz = ((float)frame_delta * (float)configTICK_RATE_HZ)
                             / (float)tick_delta;
        }
    }
    previous_frame_count = stats.valid_frames;
    previous_rate_tick = now_tick;

    if (!imu_get_latest(&sample)) {
        printf("IMU: waiting bytes=%lu frames=%lu rate=%.1fHz ck=%lu fmt=%lu ovf=%lu dma=%lu\r\n",
               (unsigned long)stats.received_bytes,
               (unsigned long)stats.valid_frames,
               measured_rate_hz,
               (unsigned long)stats.checksum_errors,
               (unsigned long)stats.format_errors,
               (unsigned long)stats.dma_overruns,
               (unsigned long)stats.dma_errors);
        return;
    }

    printf("IMU: tid=%u fields=0x%03lX acc=%.3f,%.3f,%.3f gyro=%.3f,%.3f,%.3f ",
           (unsigned int)sample.transaction_id,
           (unsigned long)sample.valid_fields,
           sample.acceleration_mps2[0], sample.acceleration_mps2[1], sample.acceleration_mps2[2],
           sample.angular_rate_dps[0], sample.angular_rate_dps[1], sample.angular_rate_dps[2]);
    printf("euler=%.3f,%.3f,%.3f temp=%.2f tick=%lu frames=%lu "
           "rate=%.1fHz ck=%lu fmt=%lu ovf=%lu dma=%lu\r\n",
           sample.euler_deg[0], sample.euler_deg[1], sample.euler_deg[2],
           sample.temperature_c,
           (unsigned long)sample.received_tick,
           (unsigned long)stats.valid_frames,
           measured_rate_hz,
           (unsigned long)stats.checksum_errors,
           (unsigned long)stats.format_errors,
           (unsigned long)stats.dma_overruns,
           (unsigned long)stats.dma_errors);
}
