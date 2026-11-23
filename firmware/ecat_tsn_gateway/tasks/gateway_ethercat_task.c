/*
 * gateway_ethercat_task.c - EtherCAT stack service task.
 * Author: BearKing
 */

#include "gateway_task_config.h"
#include "gateway_perf.h"
#include "gateway_tasks_internal.h"
#include "gateway_system.h"
#include "motor_config.h"

#include "FreeRTOS.h"
#include "SSC/Src/applInterface.h"
#include "SSC/Src/ecatappl.h"
#include "task.h"
#include <stdio.h>

static void gateway_ethercat_task(void *parameter)
{
    TickType_t last_wake_time;

    (void)parameter;
    gateway_perf_init();
    gateway_perf_register_task(GATEWAY_PERF_TASK_ETHERCAT, xTaskGetCurrentTaskHandle());

    if (!gateway_system_init()) {
        gateway_tasks_fail_stop();
    }

    bRunApplication = TRUE;
    gateway_tasks_set_running(true);
    if (!gateway_tasks_start_worker_tasks()) {
        gateway_tasks_set_running(false);
        gateway_tasks_fail_stop();
    }

    printf("runtime: EtherCAT=%ums motorPoll=%ums motorControl=%ums imuPoll=%ums status=%ums\r\n",
           (unsigned int)GATEWAY_ETHERCAT_PERIOD_MS,
           (unsigned int)GATEWAY_MOTOR_POLL_PERIOD_MS,
           (unsigned int)MOTOR_MANAGER_CONTROL_PERIOD_MS,
           (unsigned int)GATEWAY_IMU_POLL_PERIOD_MS,
           (unsigned int)GATEWAY_STATUS_PERIOD_MS);

    last_wake_time = xTaskGetTickCount();
    while (bRunApplication == TRUE) {
        uint64_t start_ticks;

        gateway_perf_record_wake(GATEWAY_PERF_TASK_ETHERCAT, last_wake_time);
        start_ticks = gateway_perf_get_ticks();
        MainLoop();
        gateway_perf_record_exec(GATEWAY_PERF_TASK_ETHERCAT,
                                 start_ticks,
                                 GATEWAY_ETHERCAT_PERIOD_MS * 1000U);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GATEWAY_ETHERCAT_PERIOD_MS));
    }

    gateway_tasks_set_running(false);
    vTaskDelete(NULL);
}

bool gateway_ethercat_task_start(void)
{
    TaskHandle_t task_handle = NULL;
    bool created = xTaskCreate(gateway_ethercat_task,
                               "ethercat",
                               GATEWAY_ETHERCAT_STACK_DEPTH,
                               NULL,
                               GATEWAY_ETHERCAT_TASK_PRIORITY,
                               &task_handle) == pdPASS;

    if (created) {
        gateway_perf_register_task(GATEWAY_PERF_TASK_ETHERCAT, task_handle);
    }

    return created;
}
