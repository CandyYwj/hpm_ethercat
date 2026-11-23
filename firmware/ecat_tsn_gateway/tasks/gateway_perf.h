/*
 * gateway_perf.h - Low-overhead task timing counters.
 * Author: BearKing
 */
#ifndef GATEWAY_PERF_H
#define GATEWAY_PERF_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

typedef enum {
    GATEWAY_PERF_TASK_ETHERCAT = 0,
    GATEWAY_PERF_TASK_MOTOR,
    GATEWAY_PERF_TASK_IMU,
    GATEWAY_PERF_TASK_STATUS,
    GATEWAY_PERF_TASK_COUNT,
} gateway_perf_task_id_t;

void gateway_perf_init(void);
void gateway_perf_register_task(gateway_perf_task_id_t task_id, TaskHandle_t task_handle);
uint64_t gateway_perf_get_ticks(void);
uint32_t gateway_perf_ticks_to_us(uint64_t ticks);
void gateway_perf_record_wake(gateway_perf_task_id_t task_id, TickType_t scheduled_tick);
void gateway_perf_record_exec(gateway_perf_task_id_t task_id,
                              uint64_t start_ticks,
                              uint32_t deadline_us);
void gateway_perf_print_status(void);

#endif /* GATEWAY_PERF_H */
