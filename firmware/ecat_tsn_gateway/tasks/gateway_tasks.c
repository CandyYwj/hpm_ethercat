/*
 * gateway_tasks.c - Shared task lifecycle and top-level startup.
 * Author: BearKing
 */

#include "gateway_tasks.h"
#include "gateway_tasks_internal.h"

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

static volatile bool s_is_gateway_running;

bool gateway_tasks_start(void)
{
    s_is_gateway_running = false;
    return gateway_ethercat_task_start();
}

bool gateway_tasks_are_running(void)
{
    return s_is_gateway_running;
}

void gateway_tasks_set_running(bool is_running)
{
    s_is_gateway_running = is_running;
}

bool gateway_tasks_start_worker_tasks(void)
{
    if (!gateway_motor_task_start()) {
        printf("Motor task creation FAILED\r\n");
        return false;
    }

    if (!gateway_imu_task_start()) {
        printf("IMU task creation FAILED\r\n");
        return false;
    }

    if (!gateway_status_task_start()) {
        printf("Status task creation FAILED\r\n");
        return false;
    }

    return true;
}

void gateway_tasks_fail_stop(void)
{
    taskDISABLE_INTERRUPTS();
    __asm volatile("ebreak");
    for (;;) {
    }
}
