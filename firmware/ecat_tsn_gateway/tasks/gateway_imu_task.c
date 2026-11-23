/*
 * gateway_imu_task.c - Bounded IMU receive processing task.
 * Author: BearKing
 */

#include "gateway_task_config.h"
#include "gateway_perf.h"
#include "gateway_tasks_internal.h"
#include "imu.h"

#include "FreeRTOS.h"
#include "task.h"

static void gateway_imu_task(void *parameter)
{
    TickType_t last_wake_time;

    (void)parameter;
    last_wake_time = xTaskGetTickCount();

    while (gateway_tasks_are_running()) {
        uint64_t start_ticks;

        gateway_perf_record_wake(GATEWAY_PERF_TASK_IMU, last_wake_time);
        start_ticks = gateway_perf_get_ticks();
        imu_process_rx();
        gateway_perf_record_exec(GATEWAY_PERF_TASK_IMU,
                                 start_ticks,
                                 GATEWAY_IMU_POLL_PERIOD_MS * 1000U);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GATEWAY_IMU_POLL_PERIOD_MS));
    }

    vTaskDelete(NULL);
}

bool gateway_imu_task_start(void)
{
    TaskHandle_t task_handle = NULL;
    bool created = xTaskCreate(gateway_imu_task,
                               "imu_rx",
                               GATEWAY_IMU_STACK_DEPTH,
                               NULL,
                               GATEWAY_IMU_TASK_PRIORITY,
                               &task_handle) == pdPASS;

    if (created) {
        gateway_perf_register_task(GATEWAY_PERF_TASK_IMU, task_handle);
    }

    return created;
}
