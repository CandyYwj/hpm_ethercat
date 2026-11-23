/*
 * gateway_perf.c - Low-overhead task timing counters.
 * Author: BearKing
 */

#include "gateway_perf.h"

#include "hpm_clock_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_soc.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    TaskHandle_t handle;
    uint32_t deadline_us;
    volatile uint32_t exec_count;
    volatile uint32_t exec_max_us;
    volatile uint32_t exec_overrun_count;
    volatile uint32_t wake_late_max_ms;
    volatile uint32_t wake_late_count;
} gateway_perf_task_stats_t;

static gateway_perf_task_stats_t s_task_stats[GATEWAY_PERF_TASK_COUNT];
static uint32_t s_mchtmr_freq_hz;

static void set_task_names(void)
{
    s_task_stats[GATEWAY_PERF_TASK_ETHERCAT].name = "ethercat";
    s_task_stats[GATEWAY_PERF_TASK_MOTOR].name = "motor";
    s_task_stats[GATEWAY_PERF_TASK_IMU].name = "imu";
    s_task_stats[GATEWAY_PERF_TASK_STATUS].name = "status";
}

static void update_maximum_u32(volatile uint32_t *maximum, uint32_t value)
{
    if (value > *maximum) {
        *maximum = value;
    }
}

void gateway_perf_init(void)
{
    memset(s_task_stats, 0, sizeof(s_task_stats));
    set_task_names();
    s_mchtmr_freq_hz = clock_get_frequency(clock_mchtmr0);
}

void gateway_perf_register_task(gateway_perf_task_id_t task_id, TaskHandle_t task_handle)
{
    if (task_id >= GATEWAY_PERF_TASK_COUNT) {
        return;
    }

    s_task_stats[task_id].handle = task_handle;
}

uint64_t gateway_perf_get_ticks(void)
{
    return mchtmr_get_count(HPM_MCHTMR);
}

uint32_t gateway_perf_ticks_to_us(uint64_t ticks)
{
    if (s_mchtmr_freq_hz == 0U) {
        return 0U;
    }

    return (uint32_t)((ticks * 1000000ULL) / s_mchtmr_freq_hz);
}

void gateway_perf_record_wake(gateway_perf_task_id_t task_id, TickType_t scheduled_tick)
{
    TickType_t now_tick;
    TickType_t late_ticks;
    uint32_t late_ms;

    if (task_id >= GATEWAY_PERF_TASK_COUNT) {
        return;
    }

    now_tick = xTaskGetTickCount();
    late_ticks = now_tick - scheduled_tick;
    if (late_ticks == 0U) {
        return;
    }

    late_ms = (uint32_t)((late_ticks * 1000U) / configTICK_RATE_HZ);
    update_maximum_u32(&s_task_stats[task_id].wake_late_max_ms, late_ms);
    s_task_stats[task_id].wake_late_count++;
}

void gateway_perf_record_exec(gateway_perf_task_id_t task_id,
                              uint64_t start_ticks,
                              uint32_t deadline_us)
{
    uint32_t elapsed_us;

    if (task_id >= GATEWAY_PERF_TASK_COUNT) {
        return;
    }

    elapsed_us = gateway_perf_ticks_to_us(gateway_perf_get_ticks() - start_ticks);
    s_task_stats[task_id].deadline_us = deadline_us;
    s_task_stats[task_id].exec_count++;
    update_maximum_u32(&s_task_stats[task_id].exec_max_us, elapsed_us);
    if ((deadline_us != 0U) && (elapsed_us > deadline_us)) {
        s_task_stats[task_id].exec_overrun_count++;
    }
}

void gateway_perf_print_status(void)
{
    uint8_t task_id;

    printf("perf: mchtmrHz=%lu build=%s\r\n",
           (unsigned long)s_mchtmr_freq_hz,
#if defined(GATEWAY_PERFORMANCE_BUILD)
           "O2g"
#else
           "debug"
#endif
    );

    for (task_id = 0U; task_id < GATEWAY_PERF_TASK_COUNT; task_id++) {
        UBaseType_t stack_words = 0U;
        const gateway_perf_task_stats_t *stats = &s_task_stats[task_id];

        if (stats->handle != NULL) {
            stack_words = uxTaskGetStackHighWaterMark(stats->handle);
        }

        printf("perf task %s: count=%lu maxUs=%lu deadlineUs=%lu overrun=%lu "
               "wakeLateMaxMs=%lu wakeLate=%lu stackFreeWords=%lu\r\n",
               stats->name,
               (unsigned long)stats->exec_count,
               (unsigned long)stats->exec_max_us,
               (unsigned long)stats->deadline_us,
               (unsigned long)stats->exec_overrun_count,
               (unsigned long)stats->wake_late_max_ms,
               (unsigned long)stats->wake_late_count,
               (unsigned long)stack_words);
    }
}
