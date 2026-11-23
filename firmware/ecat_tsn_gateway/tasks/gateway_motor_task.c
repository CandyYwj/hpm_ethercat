/*
 * gateway_motor_task.c - Motor CAN scheduler task.
 * Author: BearKing
 */

#include "gateway_task_config.h"
#include "gateway_perf.h"
#include "gateway_tasks_internal.h"
#include "motor_manager.h"

#include "FreeRTOS.h"
#include "task.h"

static void gateway_motor_task(void *parameter)
{
    TickType_t last_wake_time;

    (void)parameter;
    last_wake_time = xTaskGetTickCount();

    while (gateway_tasks_are_running()) {
        uint64_t start_ticks;

        gateway_perf_record_wake(GATEWAY_PERF_TASK_MOTOR, last_wake_time);
        start_ticks = gateway_perf_get_ticks();
        motor_manager_poll();
        gateway_perf_record_exec(GATEWAY_PERF_TASK_MOTOR,
                                 start_ticks,
                                 GATEWAY_MOTOR_POLL_PERIOD_MS * 1000U);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GATEWAY_MOTOR_POLL_PERIOD_MS));
    }

    motor_manager_stop_all();
    vTaskDelete(NULL);
}

bool gateway_motor_task_start(void)
{
    TaskHandle_t task_handle = NULL;
    bool created = xTaskCreate(gateway_motor_task,
                               "motor",
                               GATEWAY_MOTOR_STACK_DEPTH,
                               NULL,
                               GATEWAY_MOTOR_TASK_PRIORITY,
                               &task_handle) == pdPASS;

    if (created) {
        gateway_perf_register_task(GATEWAY_PERF_TASK_MOTOR, task_handle);
    }

    return created;
}
