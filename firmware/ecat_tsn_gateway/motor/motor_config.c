/*
 * motor_config.c - Default motor layout.
 * Author: BearKing
 *
 * PDO axes 0..5 map to the six physical RS02/RS03 motors. The EtherCAT PDO
 * layout remains unchanged; axes 6..15 are not scheduled on CAN.
 */

#include "motor_config.h"

static const motor_axis_config_t s_axis_config[MOTOR_MANAGER_AXIS_COUNT] = {
    { .bus = 0U, .motor_id = 1U, .host_id = MOTOR_MANAGER_HOST_ID,
      .model = ROBSTRIDE_MODEL_RS03,
      .enabled = true, .reverse = false },
    { .bus = 0U, .motor_id = 2U, .host_id = MOTOR_MANAGER_HOST_ID,
      .model = ROBSTRIDE_MODEL_RS03,
      .enabled = true, .reverse = false },
    { .bus = 0U, .motor_id = 3U, .host_id = MOTOR_MANAGER_HOST_ID,
      .model = ROBSTRIDE_MODEL_RS02,
      .enabled = true, .reverse = false },
    { .bus = 1U, .motor_id = 4U, .host_id = MOTOR_MANAGER_HOST_ID,
      .model = ROBSTRIDE_MODEL_RS03,
      .enabled = true, .reverse = false },
    { .bus = 1U, .motor_id = 5U, .host_id = MOTOR_MANAGER_HOST_ID,
      .model = ROBSTRIDE_MODEL_RS03,
      .enabled = true, .reverse = false },
    { .bus = 1U, .motor_id = 6U, .host_id = MOTOR_MANAGER_HOST_ID,
      .model = ROBSTRIDE_MODEL_RS02,
      .enabled = true, .reverse = false },
};

const motor_axis_config_t *motor_config_get_axis(uint8_t axis_index)
{
    if (axis_index >= MOTOR_MANAGER_AXIS_COUNT) {
        return NULL;
    }

    return &s_axis_config[axis_index];
}

int motor_config_find_axis_index(uint8_t bus, uint8_t motor_id)
{
    const motor_axis_config_t *config;
    uint8_t axis_index;

    if ((motor_id == 0U) || (motor_id > MOTOR_MANAGER_AXIS_COUNT)) {
        return -1;
    }

    axis_index = (uint8_t)(motor_id - 1U);
    config = &s_axis_config[axis_index];
    if (!config->enabled || (config->bus != bus)
        || (config->motor_id != motor_id)) {
        return -1;
    }

    return (int)axis_index;
}
