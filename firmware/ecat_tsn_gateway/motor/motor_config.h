/*
 * motor_config.h - Axis-to-motor mapping.
 * Author: BearKing
 */
#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

#include "robstride_proto.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MOTOR_MANAGER_AXIS_COUNT              6U
#define MOTOR_MANAGER_HOST_ID                 ROBSTRIDE_DEFAULT_HOST_ID

/* Direct-control mode: EtherCAT RxPDO commands are sent to configured motors. */
#define MOTOR_MANAGER_ENABLE_AUTO_ENABLE      1
#ifndef MOTOR_MANAGER_ENABLE_MOTION_CONTROL
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL   1
#endif

/* Task 10 fault injection hooks (bench builds only). */
#ifndef MOTOR_MANAGER_ENABLE_FAULT_INJECTION
#define MOTOR_MANAGER_ENABLE_FAULT_INJECTION  0
#endif

#define MOTOR_MANAGER_CONTROL_PERIOD_MS       2U
#define MOTOR_MANAGER_COMMAND_TIMEOUT_MS      100U

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint8_t host_id;
    robstride_motor_model_t model;
    bool enabled;
    bool reverse;
} motor_axis_config_t;

const motor_axis_config_t *motor_config_get_axis(uint8_t axis_index);
int motor_config_find_axis_index(uint8_t bus, uint8_t motor_id);

#endif
