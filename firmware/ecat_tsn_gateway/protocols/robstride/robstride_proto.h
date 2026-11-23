/*
 * robstride_proto.h - RobStride RS02/RS03 private CAN protocol encoder/decoder.
 * Author: BearKing
 *
 * This layer is pure protocol code. It does not initialize CAN, schedule
 * motors, or touch EtherCAT objects.
 */
#ifndef ROBSTRIDE_PROTO_H
#define ROBSTRIDE_PROTO_H

#include "can_if.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROBSTRIDE_DEFAULT_HOST_ID            0xFDU
#define ROBSTRIDE_BROADCAST_RESPONSE_ID      0xFEU
#define ROBSTRIDE_PRIVATE_CAN_ID_MASK        0x1FFFFFFFUL
#define ROBSTRIDE_PRIVATE_DLC                8U

/* Private parameter index for the run-mode register (Type 18 write / Type 17
 * read). Shared with motor_manager.c so mode readback uses one definition. */
#define ROBSTRIDE_PARAM_INDEX_RUN_MODE       0x7005U

typedef enum {
    ROBSTRIDE_PRIVATE_TYPE_DEVICE_ID       = 0x00U,
    ROBSTRIDE_PRIVATE_TYPE_MOTION_CONTROL  = 0x01U,
    ROBSTRIDE_PRIVATE_TYPE_FEEDBACK        = 0x02U,
    ROBSTRIDE_PRIVATE_TYPE_ENABLE          = 0x03U,
    ROBSTRIDE_PRIVATE_TYPE_STOP            = 0x04U,
    ROBSTRIDE_PRIVATE_TYPE_SET_ZERO        = 0x06U,
    ROBSTRIDE_PRIVATE_TYPE_SET_CAN_ID      = 0x07U,
    ROBSTRIDE_PRIVATE_TYPE_READ_PARAM      = 0x11U,
    ROBSTRIDE_PRIVATE_TYPE_WRITE_PARAM     = 0x12U,
    ROBSTRIDE_PRIVATE_TYPE_FAULT_REPORT    = 0x15U,
    ROBSTRIDE_PRIVATE_TYPE_SAVE_PARAMS     = 0x16U,
    ROBSTRIDE_PRIVATE_TYPE_SET_BAUDRATE    = 0x17U,
    ROBSTRIDE_PRIVATE_TYPE_REPORT_CONTROL  = 0x18U,
    ROBSTRIDE_PRIVATE_TYPE_SET_PROTOCOL    = 0x19U,
} robstride_private_type_t;

typedef enum {
    ROBSTRIDE_CONTROL_MODE_RESET = 0U,
    ROBSTRIDE_CONTROL_MODE_CALIBRATION = 1U,
    ROBSTRIDE_CONTROL_MODE_MOTOR = 2U,
} robstride_motor_mode_t;

typedef enum {
    ROBSTRIDE_MODEL_RS02 = 0U,
    ROBSTRIDE_MODEL_RS03 = 1U,
} robstride_motor_model_t;

typedef enum {
    ROBSTRIDE_RUN_MODE_OPERATION = 0U,
} robstride_run_mode_t;

typedef struct {
    float position_min_rad;
    float position_max_rad;
    float velocity_min_rad_s;
    float velocity_max_rad_s;
    float torque_min_nm;
    float torque_max_nm;
    float kp_min;
    float kp_max;
    float kd_min;
    float kd_max;
    float current_min_a;
    float current_max_a;
    float mode_velocity_min_rad_s;
    float mode_velocity_max_rad_s;
} robstride_motor_limits_t;

typedef struct {
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float kp;
    float kd;
} robstride_motion_cmd_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint8_t host_id;
    robstride_motor_mode_t mode;
    uint8_t fault_bits;
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float temperature_c;
} robstride_feedback_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint8_t host_id;
    uint8_t status;
    uint16_t index;
    uint32_t raw_u32;
    float value_f32;
} robstride_param_response_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint32_t fault_value;
    uint32_t warning_value;
} robstride_fault_report_t;

typedef struct {
    uint8_t type;
    uint16_t data;
    uint8_t target_id;
} robstride_private_id_fields_t;

uint16_t robstride_float_to_uint(float value, float min_value, float max_value, uint8_t bits);
float robstride_uint_to_float(uint16_t value, float min_value, float max_value, uint8_t bits);
const robstride_motor_limits_t *robstride_get_motor_limits(robstride_motor_model_t model);

uint32_t robstride_private_make_id(uint8_t type, uint16_t data, uint8_t target_id);
bool robstride_private_parse_id(uint32_t id, robstride_private_id_fields_t *fields);

bool robstride_private_encode_enable(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame);
bool robstride_private_encode_stop(uint8_t bus, uint8_t motor_id, uint8_t host_id, bool clear_fault, can_if_frame_t *frame);
bool robstride_private_encode_motion_control(uint8_t bus, uint8_t motor_id,
                                        robstride_motor_model_t model,
                                        const robstride_motion_cmd_t *cmd,
                                        can_if_frame_t *frame);
bool robstride_private_decode_feedback(const can_if_frame_t *frame,
                                  robstride_motor_model_t model,
                                  robstride_feedback_t *feedback);

bool robstride_private_encode_read_param(uint8_t bus, uint8_t motor_id,
                                    uint8_t host_id, uint16_t index,
                                    can_if_frame_t *frame);
bool robstride_private_encode_write_param_u8(uint8_t bus, uint8_t motor_id,
                                        uint8_t host_id, uint16_t index,
                                        uint8_t value, can_if_frame_t *frame);
bool robstride_private_encode_set_run_mode(uint8_t bus, uint8_t motor_id,
                                      uint8_t host_id, robstride_run_mode_t mode,
                                      can_if_frame_t *frame);
bool robstride_private_decode_param_response(const can_if_frame_t *frame, robstride_param_response_t *response);
bool robstride_private_decode_fault_report(const can_if_frame_t *frame, robstride_fault_report_t *report);

#endif
