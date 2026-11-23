/*
 * main.c - HPM6E80 EtherCAT motor gateway entry point.
 * Author: BearKing
 */

#include "gateway_tasks.h"
#include "FreeRTOS.h"
#include "board.h"
#include "full_port_lcd_drv.h"
#include "task.h"
#include <stdio.h>

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    __asm volatile("ebreak");
    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    taskDISABLE_INTERRUPTS();
    __asm volatile("ebreak");
    for (;;) {
    }
}

int main(void)
{
    board_init();
    full_port_lcd_init();
    printf("hpm6e00 gateway\r\n");

    if (!gateway_tasks_start()) {
        printf("Gateway task creation FAILED\r\n");
        while (1) {
        }
    }

    printf("Gateway FreeRTOS scheduler starting\r\n");
    vTaskStartScheduler();

    printf("FreeRTOS scheduler start FAILED\r\n");
    while (1) {
    }
}
