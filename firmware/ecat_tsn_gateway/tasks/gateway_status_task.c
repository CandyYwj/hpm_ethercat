/*
 * gateway_status_task.c - Low-rate LED and status output task.
 * Author: BearKing
 */

#include "gateway_task_config.h"
#include "gateway_perf.h"
#include "gateway_tasks_internal.h"
#include "imu.h"
#include "motor_manager.h"
#include "gateway_app.h"

#include "FreeRTOS.h"
#include "full_port_led_drv.h"
#include "task.h"

static void gateway_status_task(void *parameter)
{
    TickType_t last_wake_time;
    (void)parameter;
    last_wake_time = xTaskGetTickCount();

    while (gateway_tasks_are_running()) {
        uint64_t start_ticks;

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GATEWAY_STATUS_PERIOD_MS));
        if (!gateway_tasks_are_running()) {
            break;
        }

        gateway_perf_record_wake(GATEWAY_PERF_TASK_STATUS, last_wake_time);
        start_ticks = gateway_perf_get_ticks();
        /* Debug: task-scheduler perf dump is noise for motor debugging. */
        /* gateway_perf_print_status(); */
        full_port_led_ctrl(FULL_PORT_LED_INDEX_LEDA_ID, FULL_PORT_LED_CTRL_TOGGLE);
        /* Fixed-format 1 Hz output for host telemetry parsing. */
        motor_manager_print_telemetry();
        /* Debug: IMU status is noise (no IMU data expected during motor debug). */
        /* imu_print_status(); */
        gateway_perf_record_exec(GATEWAY_PERF_TASK_STATUS,
                                 start_ticks,
                                 GATEWAY_STATUS_PERIOD_MS * 1000U);
    }

    vTaskDelete(NULL);
}

bool gateway_status_task_start(void)
{
    TaskHandle_t task_handle = NULL;
    bool created = xTaskCreate(gateway_status_task,
                               "status",
                               GATEWAY_STATUS_STACK_DEPTH,
                               NULL,
                               GATEWAY_STATUS_TASK_PRIORITY,
                               &task_handle) == pdPASS;

    if (created) {
        gateway_perf_register_task(GATEWAY_PERF_TASK_STATUS, task_handle);
    }

    return created;
}
