/*
 * gateway_task_config.h - Gateway feature switches and task settings.
 * Author: BearKing
 */
#ifndef GATEWAY_TASK_CONFIG_H
#define GATEWAY_TASK_CONFIG_H

#include "FreeRTOS.h"

#define GATEWAY_ETHERCAT_TASK_PRIORITY   (configMAX_PRIORITIES - 2U)
#define GATEWAY_ETHERCAT_STACK_DEPTH     2048U
#define GATEWAY_ETHERCAT_PERIOD_MS       1U

#define GATEWAY_MOTOR_TASK_PRIORITY      (configMAX_PRIORITIES - 3U)
#define GATEWAY_MOTOR_STACK_DEPTH        1536U
#define GATEWAY_MOTOR_POLL_PERIOD_MS     1U

#define GATEWAY_IMU_TASK_PRIORITY        (configMAX_PRIORITIES - 3U)
#define GATEWAY_IMU_STACK_DEPTH          1024U
#define GATEWAY_IMU_POLL_PERIOD_MS       1U

#define GATEWAY_STATUS_TASK_PRIORITY     1U
#define GATEWAY_STATUS_STACK_DEPTH       1024U
#define GATEWAY_STATUS_PERIOD_MS         1000U

#endif /* GATEWAY_TASK_CONFIG_H */
