/*
 * gateway_tasks_internal.h - Internal task lifecycle interfaces.
 * Author: BearKing
 */
#ifndef GATEWAY_TASKS_INTERNAL_H
#define GATEWAY_TASKS_INTERNAL_H

#include <stdbool.h>

bool gateway_ethercat_task_start(void);
bool gateway_motor_task_start(void);
bool gateway_imu_task_start(void);
bool gateway_status_task_start(void);

bool gateway_tasks_are_running(void);
void gateway_tasks_set_running(bool is_running);
bool gateway_tasks_start_worker_tasks(void);
void gateway_tasks_fail_stop(void);

#endif /* GATEWAY_TASKS_INTERNAL_H */
