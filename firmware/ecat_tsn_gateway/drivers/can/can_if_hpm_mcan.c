/*
 * can_if_hpm_mcan.c - HPM MCAN backend for eight classic CAN motor buses.
 * Author: BearKing
 */

#include "can_if.h"

#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_mcan_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_soc.h"
#include <string.h>

#define CAN_IF_RX_RING_SIZE    64U
#define CAN_IF_EXT_ID_MASK     0x1FFFFFFFUL
#define CAN_IF_STD_ID_MASK     0x7FFUL
/* Matches the default RX FIFO element count (MCAN_RXFIFO_SIZE_CAN_DEFAULT). */
#define CAN_IF_RX_FIFO_DRAIN_BUDGET  32U

/*
 * CAN0 and CAN1 are wired for the six RS02/RS03 motors.
 *
 * Initializing floating/unconnected CAN buses can create continuous MCAN error
 * interrupts, which hides the real status of the buses under test. Change this
 * mask when the wired bus set changes.
 */
#ifndef CAN_IF_ENABLED_BUS_MASK
#define CAN_IF_ENABLED_BUS_MASK 0x03UL
#endif

typedef struct {
    MCAN_Type *base;
    uint32_t irq_num;
    bool initialized;
    volatile uint32_t tx_count;
    volatile uint32_t rx_count;
    volatile uint32_t error_count;
    volatile uint32_t drop_count;
    volatile uint32_t isr_count;
    volatile uint32_t isr_max_us;
    volatile uint32_t rx_fifo0_count;
    volatile uint32_t rx_fifo1_count;
    volatile uint32_t rx_fifo_full_count;
    volatile uint32_t rx_fifo_lost_count;
    volatile uint32_t rx_ring_high_water;
    volatile uint32_t tx_fifo_full_count;
    volatile uint32_t rx_ring_full_count;
    volatile uint32_t bus_off_count;
    volatile uint32_t bus_off_recovery_count;
    volatile uint32_t error_passive_count;
    volatile uint8_t bus_state;
} can_if_bus_t;

typedef struct {
    can_if_frame_t entries[CAN_IF_RX_RING_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} can_if_rx_ring_t;

#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
#if defined(HPM_MCAN0)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan0_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN1)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan1_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN2)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan2_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN3)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan3_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN4)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan4_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN5)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan5_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN6)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan6_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#if defined(HPM_MCAN7)
ATTR_PLACE_AT(".ahb_sram") static uint32_t s_mcan7_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
#endif
#endif

static can_if_bus_t s_buses[CAN_IF_BUS_COUNT] = {
#if defined(HPM_MCAN0)
    [0] = { .base = HPM_MCAN0, .irq_num = IRQn_MCAN0 },
#endif
#if defined(HPM_MCAN1)
    [1] = { .base = HPM_MCAN1, .irq_num = IRQn_MCAN1 },
#endif
#if defined(HPM_MCAN2)
    [2] = { .base = HPM_MCAN2, .irq_num = IRQn_MCAN2 },
#endif
#if defined(HPM_MCAN3)
    [3] = { .base = HPM_MCAN3, .irq_num = IRQn_MCAN3 },
#endif
#if defined(HPM_MCAN4)
    [4] = { .base = HPM_MCAN4, .irq_num = IRQn_MCAN4 },
#endif
#if defined(HPM_MCAN5)
    [5] = { .base = HPM_MCAN5, .irq_num = IRQn_MCAN5 },
#endif
#if defined(HPM_MCAN6)
    [6] = { .base = HPM_MCAN6, .irq_num = IRQn_MCAN6 },
#endif
#if defined(HPM_MCAN7)
    [7] = { .base = HPM_MCAN7, .irq_num = IRQn_MCAN7 },
#endif
};

static can_if_rx_ring_t s_rx_rings[CAN_IF_BUS_COUNT];
static uint8_t s_receive_cursor;
static uint32_t s_mchtmr_freq_hz;

static uint64_t read_perf_counter_ticks(void)
{
    return mchtmr_get_count(HPM_MCHTMR);
}

static uint32_t perf_ticks_to_microseconds(uint64_t ticks)
{
    if (s_mchtmr_freq_hz == 0U) {
        return 0U;
    }

    return (uint32_t)((ticks * 1000000ULL) / s_mchtmr_freq_hz);
}

static void update_maximum_u32(volatile uint32_t *maximum, uint32_t value)
{
    if (value > *maximum) {
        *maximum = value;
    }
}

void can_if_board_init_pins(uint8_t bus, MCAN_Type *base)
{
    (void)base;

    switch (bus) {
    case 0U:
        HPM_IOC->PAD[IOC_PAD_PC00].FUNC_CTL = IOC_PC00_FUNC_CTL_MCAN0_TXD;
        HPM_IOC->PAD[IOC_PAD_PC01].FUNC_CTL = IOC_PC01_FUNC_CTL_MCAN0_RXD;
        HPM_IOC->PAD[IOC_PAD_PC02].FUNC_CTL = IOC_PC02_FUNC_CTL_GPIO_C_02;
        break;
    case 1U:
        HPM_IOC->PAD[IOC_PAD_PC05].FUNC_CTL = IOC_PC05_FUNC_CTL_MCAN1_TXD;
        HPM_IOC->PAD[IOC_PAD_PC04].FUNC_CTL = IOC_PC04_FUNC_CTL_MCAN1_RXD;
        HPM_IOC->PAD[IOC_PAD_PC03].FUNC_CTL = IOC_PC03_FUNC_CTL_GPIO_C_03;
        break;
    default:
        break;
    }
}

void can_if_board_set_transceiver(uint8_t bus, bool enable)
{
    uint8_t standby_pin;

    if (bus == 0U) {
        standby_pin = 2U;
    } else if (bus == 1U) {
        standby_pin = 3U;
    } else {
        return;
    }

    /* TJA1042 STBY is active high: drive low to enable normal operation. */
    gpio_set_pin_output_with_initial(HPM_GPIO0, GPIO_DO_GPIOC, standby_pin,
                                     enable ? 0U : 1U);
}

static uint16_t ring_next(uint16_t value)
{
    return (uint16_t)((value + 1U) % CAN_IF_RX_RING_SIZE);
}

static uint16_t ring_used_count(const can_if_rx_ring_t *ring)
{
    if (ring->head >= ring->tail) {
        return (uint16_t)(ring->head - ring->tail);
    }

    return (uint16_t)(CAN_IF_RX_RING_SIZE - ring->tail + ring->head);
}

static bool ring_push(uint8_t bus, const can_if_frame_t *frame)
{
    can_if_rx_ring_t *ring = &s_rx_rings[bus];
    uint16_t next = ring_next(ring->head);

    if (next == ring->tail) {
        s_buses[bus].drop_count++;
        s_buses[bus].rx_ring_full_count++;
        return false;
    }

    ring->entries[ring->head] = *frame;
    ring->head = next;
    update_maximum_u32(&s_buses[bus].rx_ring_high_water, ring_used_count(ring));
    return true;
}

static bool ring_pop(uint8_t bus, can_if_frame_t *frame)
{
    can_if_rx_ring_t *ring = &s_rx_rings[bus];

    if (ring->tail == ring->head) {
        return false;
    }

    *frame = ring->entries[ring->tail];
    ring->tail = ring_next(ring->tail);
    return true;
}

#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
static bool can_if_set_msg_buffer(uint8_t bus)
{
    mcan_msg_buf_attr_t attr;

    switch (bus) {
#if defined(HPM_MCAN0)
    case 0:
        attr.ram_base = (uint32_t)&s_mcan0_msg_buf[0];
        attr.ram_size = sizeof(s_mcan0_msg_buf);
        break;
#endif
#if defined(HPM_MCAN1)
    case 1:
        attr.ram_base = (uint32_t)&s_mcan1_msg_buf[0];
        attr.ram_size = sizeof(s_mcan1_msg_buf);
        break;
#endif
#if defined(HPM_MCAN2)
    case 2:
        attr.ram_base = (uint32_t)&s_mcan2_msg_buf[0];
        attr.ram_size = sizeof(s_mcan2_msg_buf);
        break;
#endif
#if defined(HPM_MCAN3)
    case 3:
        attr.ram_base = (uint32_t)&s_mcan3_msg_buf[0];
        attr.ram_size = sizeof(s_mcan3_msg_buf);
        break;
#endif
#if defined(HPM_MCAN4)
    case 4:
        attr.ram_base = (uint32_t)&s_mcan4_msg_buf[0];
        attr.ram_size = sizeof(s_mcan4_msg_buf);
        break;
#endif
#if defined(HPM_MCAN5)
    case 5:
        attr.ram_base = (uint32_t)&s_mcan5_msg_buf[0];
        attr.ram_size = sizeof(s_mcan5_msg_buf);
        break;
#endif
#if defined(HPM_MCAN6)
    case 6:
        attr.ram_base = (uint32_t)&s_mcan6_msg_buf[0];
        attr.ram_size = sizeof(s_mcan6_msg_buf);
        break;
#endif
#if defined(HPM_MCAN7)
    case 7:
        attr.ram_base = (uint32_t)&s_mcan7_msg_buf[0];
        attr.ram_size = sizeof(s_mcan7_msg_buf);
        break;
#endif
    default:
        return false;
    }

    return mcan_set_msg_buf_attr(s_buses[bus].base, &attr) == status_success;
}
#endif

static uint32_t can_if_init_clock(uint8_t bus)
{
    clock_name_t clock_name;

    switch (bus) {
    case 0:
        clock_name = clock_can0;
        break;
    case 1:
        clock_name = clock_can1;
        break;
    case 2:
        clock_name = clock_can2;
        break;
    case 3:
        clock_name = clock_can3;
        break;
    case 4:
        clock_name = clock_can4;
        break;
    case 5:
        clock_name = clock_can5;
        break;
    case 6:
        clock_name = clock_can6;
        break;
    case 7:
        clock_name = clock_can7;
        break;
    default:
        return 0;
    }

    clock_add_to_group(clock_name, BOARD_RUNNING_CORE & 0x1);
    clock_set_source_divider(clock_name, clk_src_pll1_clk0, 10);
    return clock_get_frequency(clock_name);
}

static bool can_if_rx_from_fifo(uint8_t bus, uint32_t fifo_index)
{
    mcan_rx_message_t rx;
    can_if_frame_t frame;

    if (mcan_read_rxfifo(s_buses[bus].base, fifo_index, &rx) != status_success) {
        return false;
    }

    memset(&frame, 0, sizeof(frame));
    frame.bus = bus;
    frame.id_format = rx.use_ext_id ? CAN_IF_ID_EXTENDED : CAN_IF_ID_STANDARD;
    frame.frame_format = rx.canfd_frame ? CAN_IF_FRAME_FD : CAN_IF_FRAME_CLASSIC;
    frame.id = rx.use_ext_id ? rx.ext_id : rx.std_id;
    frame.dlc = rx.dlc;
    if (frame.dlc > CAN_IF_CLASSIC_MAX_DLC) {
        frame.dlc = CAN_IF_CLASSIC_MAX_DLC;
    }
    memcpy(frame.data, rx.data_8, frame.dlc);

    s_buses[bus].rx_count++;
    if (fifo_index == 0U) {
        s_buses[bus].rx_fifo0_count++;
    } else {
        s_buses[bus].rx_fifo1_count++;
    }
    (void)ring_push(bus, &frame);
    return true;
}

static void can_if_isr(uint8_t bus)
{
    uint32_t flags;
    MCAN_Type *base = s_buses[bus].base;
    uint64_t start_ticks;

    if ((base == NULL) || !s_buses[bus].initialized) {
        return;
    }

    start_ticks = read_perf_counter_ticks();
    flags = mcan_get_interrupt_flags(base);
    s_buses[bus].isr_count++;

    /*
     * Drain each RX FIFO in a bounded loop instead of reading one message per
     * ISR. The MCAN new-message flag would otherwise re-trigger this ISR for
     * every queued frame, paying a full ISR entry/exit per frame. Reading is
     * bounded by CAN_IF_RX_FIFO_DRAIN_BUDGET so a single ISR can never starve
     * the lower-priority motor task or overflow the software RX ring.
     */
    if ((flags & MCAN_INT_RXFIFO0_NEW_MSG) != 0U) {
        uint32_t drain = CAN_IF_RX_FIFO_DRAIN_BUDGET;
        while ((drain-- != 0U) && can_if_rx_from_fifo(bus, 0U)) {
        }
    }

    if ((flags & MCAN_INT_RXFIFO1_NEW_MSG) != 0U) {
        uint32_t drain = CAN_IF_RX_FIFO_DRAIN_BUDGET;
        while ((drain-- != 0U) && can_if_rx_from_fifo(bus, 1U)) {
        }
    }

    if ((flags & (MCAN_INT_RXFIFO0_FULL | MCAN_INT_RXFIFO1_FULL)) != 0U) {
        s_buses[bus].rx_fifo_full_count++;
    }

    if ((flags & (MCAN_INT_RXFIFO0_MSG_LOST | MCAN_INT_RXFIFO1_MSG_LOST)) != 0U) {
        s_buses[bus].rx_fifo_lost_count++;
        s_buses[bus].drop_count++;
    }

    if ((flags & MCAN_EVENT_ERROR) != 0U) {
        s_buses[bus].error_count++;
    }

    if ((flags & MCAN_INT_BUS_OFF_STATUS) != 0U) {
        s_buses[bus].bus_off_count++;
        s_buses[bus].bus_state = (uint8_t)CAN_IF_BUS_OFF;
    } else if ((flags & MCAN_INT_ERROR_PASSIVE) != 0U) {
        s_buses[bus].error_passive_count++;
        if (s_buses[bus].bus_state == (uint8_t)CAN_IF_BUS_OK) {
            s_buses[bus].bus_state = (uint8_t)CAN_IF_BUS_ERROR_PASSIVE;
        }
    }

    mcan_clear_interrupt_flags(base, flags);
    update_maximum_u32(&s_buses[bus].isr_max_us,
                       perf_ticks_to_microseconds(read_perf_counter_ticks() - start_ticks));
}

#if defined(HPM_MCAN0)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN0, can_if_mcan0_isr)
void can_if_mcan0_isr(void) { can_if_isr(0U); }
#endif

#if defined(HPM_MCAN1)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN1, can_if_mcan1_isr)
void can_if_mcan1_isr(void) { can_if_isr(1U); }
#endif

#if defined(HPM_MCAN2)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN2, can_if_mcan2_isr)
void can_if_mcan2_isr(void) { can_if_isr(2U); }
#endif

#if defined(HPM_MCAN3)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN3, can_if_mcan3_isr)
void can_if_mcan3_isr(void) { can_if_isr(3U); }
#endif

#if defined(HPM_MCAN4)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN4, can_if_mcan4_isr)
void can_if_mcan4_isr(void) { can_if_isr(4U); }
#endif

#if defined(HPM_MCAN5)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN5, can_if_mcan5_isr)
void can_if_mcan5_isr(void) { can_if_isr(5U); }
#endif

#if defined(HPM_MCAN6)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN6, can_if_mcan6_isr)
void can_if_mcan6_isr(void) { can_if_isr(6U); }
#endif

#if defined(HPM_MCAN7)
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN7, can_if_mcan7_isr)
void can_if_mcan7_isr(void) { can_if_isr(7U); }
#endif

bool can_if_init(void)
{
    uint8_t bus;
    bool any_initialized = false;

    memset(s_rx_rings, 0, sizeof(s_rx_rings));
    s_receive_cursor = 0U;
    s_mchtmr_freq_hz = clock_get_frequency(clock_mchtmr0);

    for (bus = 0; bus < CAN_IF_BUS_COUNT; bus++) {
        mcan_config_t config;
        uint32_t src_clk_freq;
        uint32_t interrupt_mask;

        if ((CAN_IF_ENABLED_BUS_MASK & (1UL << bus)) == 0U) {
            continue;
        }

        if (s_buses[bus].base == NULL) {
            continue;
        }

#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
        if (!can_if_set_msg_buffer(bus)) {
            s_buses[bus].error_count++;
            continue;
        }
#endif

        mcan_get_default_config(s_buses[bus].base, &config);
        config.baudrate = CAN_IF_DEFAULT_BITRATE_BPS;
        config.mode = mcan_mode_normal;
        /* Bring-up: keep the controller on the plain default configuration; the
         * extra filter/format settings are what separated us from the working
         * standalone test. Re-add them one by one once the link is verified. */
        (void)interrupt_mask;

        can_if_board_init_pins(bus, s_buses[bus].base);
        can_if_board_set_transceiver(bus, true);
        src_clk_freq = can_if_init_clock(bus);
        if (src_clk_freq == 0U) {
            s_buses[bus].error_count++;
            continue;
        }

        interrupt_mask = MCAN_EVENT_RECEIVE
                       | MCAN_EVENT_ERROR
                       | MCAN_INT_RXFIFO0_FULL
                       | MCAN_INT_RXFIFO1_FULL
                       | MCAN_INT_RXFIFO0_MSG_LOST
                       | MCAN_INT_RXFIFO1_MSG_LOST;

        if (mcan_init(s_buses[bus].base, &config, src_clk_freq) != status_success) {
            s_buses[bus].error_count++;
            continue;
        }

        mcan_enable_interrupts(s_buses[bus].base, interrupt_mask);
        /* Keep CAN interrupts at the FreeRTOS syscall-safe priority. */
        intc_m_enable_irq_with_priority(s_buses[bus].irq_num, 2);
        s_buses[bus].initialized = true;
        any_initialized = true;
    }

    return any_initialized;
}

bool can_if_send(const can_if_frame_t *frame)
{
    mcan_tx_frame_t tx;
    hpm_stat_t tx_status;
    uint8_t bus;

    if (frame == NULL) {
        return false;
    }

    bus = frame->bus;
    if ((bus >= CAN_IF_BUS_COUNT) || !s_buses[bus].initialized) {
        return false;
    }

    if ((frame->frame_format != CAN_IF_FRAME_CLASSIC) || (frame->dlc > CAN_IF_CLASSIC_MAX_DLC)) {
        return false;
    }

    memset(&tx, 0, sizeof(tx));
    tx.dlc = frame->dlc;
    tx.canfd_frame = 0;
    tx.bitrate_switch = 0;

    if (frame->id_format == CAN_IF_ID_EXTENDED) {
        tx.use_ext_id = 1;
        tx.ext_id = frame->id & CAN_IF_EXT_ID_MASK;
    } else {
        tx.use_ext_id = 0;
        tx.std_id = frame->id & CAN_IF_STD_ID_MASK;
    }

    memcpy(tx.data_8, frame->data, frame->dlc);

    /*
     * Non-blocking enqueue: the frame is written into the hardware TX FIFO
     * and the transmission request is queued; the MCAN controller transmits
     * it asynchronously on the bus. The control task does NOT spin waiting
     * for the frame to reach the wire, so CPU is not held for ~130 us per
     * frame (classic CAN at 1 Mbit/s). The default TX FIFO is 16 elements,
     * so the 8 frames per control tick enqueue without blocking. If the FIFO
     * is full the frame is dropped this tick and retried on the next control
     * tick; tx_count counts frames accepted into the FIFO.
     */
    tx_status = mcan_transmit_via_txfifo_nonblocking(s_buses[bus].base, &tx, NULL);
    if (tx_status != status_success) {
        if (tx_status == status_mcan_txfifo_full) {
            s_buses[bus].tx_fifo_full_count++;
        }
        s_buses[bus].error_count++;
        return false;
    }

    s_buses[bus].tx_count++;
    return true;
}

bool can_if_receive(can_if_frame_t *frame)
{
    uint8_t offset;

    if (frame == NULL) {
        return false;
    }

    for (offset = 0U; offset < CAN_IF_BUS_COUNT; offset++) {
        uint8_t bus = (uint8_t)((s_receive_cursor + offset) % CAN_IF_BUS_COUNT);

        if (ring_pop(bus, frame)) {
            s_receive_cursor = (uint8_t)((bus + 1U) % CAN_IF_BUS_COUNT);
            return true;
        }
    }

    return false;
}

uint32_t can_if_get_tx_count(uint8_t bus)
{
    return (bus < CAN_IF_BUS_COUNT) ? s_buses[bus].tx_count : 0U;
}

uint32_t can_if_get_rx_count(uint8_t bus)
{
    return (bus < CAN_IF_BUS_COUNT) ? s_buses[bus].rx_count : 0U;
}

uint32_t can_if_get_error_count(uint8_t bus)
{
    return (bus < CAN_IF_BUS_COUNT) ? s_buses[bus].error_count : 0U;
}

uint32_t can_if_get_drop_count(uint8_t bus)
{
    return (bus < CAN_IF_BUS_COUNT) ? s_buses[bus].drop_count : 0U;
}

uint32_t can_if_get_initialized_mask(void)
{
    uint32_t mask = 0;
    uint8_t bus;

    for (bus = 0; bus < CAN_IF_BUS_COUNT; bus++) {
        if (s_buses[bus].initialized) {
            mask |= (1UL << bus);
        }
    }

    return mask;
}

bool can_if_get_bus_stats(uint8_t bus, can_if_bus_stats_t *stats)
{
    const can_if_rx_ring_t *ring;

    if ((stats == NULL) || (bus >= CAN_IF_BUS_COUNT)) {
        return false;
    }

    ring = &s_rx_rings[bus];
    stats->tx_count = s_buses[bus].tx_count;
    stats->rx_count = s_buses[bus].rx_count;
    stats->error_count = s_buses[bus].error_count;
    stats->drop_count = s_buses[bus].drop_count;
    stats->isr_count = s_buses[bus].isr_count;
    stats->isr_max_us = s_buses[bus].isr_max_us;
    stats->rx_fifo0_count = s_buses[bus].rx_fifo0_count;
    stats->rx_fifo1_count = s_buses[bus].rx_fifo1_count;
    stats->rx_fifo_full_count = s_buses[bus].rx_fifo_full_count;
    stats->rx_fifo_lost_count = s_buses[bus].rx_fifo_lost_count;
    stats->rx_ring_level = ring_used_count(ring);
    stats->rx_ring_high_water = s_buses[bus].rx_ring_high_water;
    stats->tx_fifo_full_count = s_buses[bus].tx_fifo_full_count;
    stats->rx_ring_full_count = s_buses[bus].rx_ring_full_count;
    stats->bus_off_count = s_buses[bus].bus_off_count;
    stats->bus_off_recovery_count = s_buses[bus].bus_off_recovery_count;
    stats->error_passive_count = s_buses[bus].error_passive_count;
    stats->bus_state = can_if_get_bus_state(bus);
    return true;
}

can_if_bus_state_t can_if_get_bus_state(uint8_t bus)
{
    MCAN_Type *base;

    if ((bus >= CAN_IF_BUS_COUNT) || !s_buses[bus].initialized) {
        return CAN_IF_BUS_OK;
    }

    base = s_buses[bus].base;
    if (base == NULL) {
        return CAN_IF_BUS_OK;
    }

    /* Read the controller live instead of trusting only the ISR cache: a
     * bus-off that happens between two status prints must still be visible. */
    if (mcan_is_in_busoff_state(base)) {
        return CAN_IF_BUS_OFF;
    }
    if (mcan_is_in_err_passive_state(base)) {
        return CAN_IF_BUS_ERROR_PASSIVE;
    }

    return CAN_IF_BUS_OK;
}

/*
 * Bus-off recovery, one init-mode cycle per call so the caller decides the
 * cadence. The controller leaves bus-off only after it observes 129 x 11
 * recessive bits, so recovery is inherently asynchronous: this function never
 * waits for the bus, and it never restarts motor motion. The caller keeps the
 * affected axes stopped and latched until an explicit reset.
 */
bool can_if_service_bus_off(uint8_t bus)
{
    MCAN_Type *base;

    if ((bus >= CAN_IF_BUS_COUNT) || !s_buses[bus].initialized) {
        return false;
    }

    base = s_buses[bus].base;
    if ((base == NULL) || !mcan_is_in_busoff_state(base)) {
        return false;
    }

    mcan_enter_init_mode(base);
    mcan_enter_normal_mode(base);
    s_buses[bus].bus_off_recovery_count++;
    return true;
}
