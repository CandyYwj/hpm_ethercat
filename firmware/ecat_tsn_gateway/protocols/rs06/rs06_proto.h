/*
 * rs06_proto.h - RobStride RS06 CAN protocol encoder/decoder.
 * Author: BearKing
 *
 * This layer is pure protocol code. It does not initialize CAN, schedule
 * motors, or touch EtherCAT objects.
 */
#ifndef RS06_PROTO_H
#define RS06_PROTO_H

#include "can_if.h"
#include <stdbool.h>
#include <stdint.h>

#define RS06_DEFAULT_HOST_ID            0xFDU
#define RS06_BROADCAST_RESPONSE_ID      0xFEU
#define RS06_PRIVATE_CAN_ID_MASK        0x1FFFFFFFUL
#define RS06_PRIVATE_DLC                8U

#define RS06_P_MIN_RAD                  (-12.57f)
#define RS06_P_MAX_RAD                  (12.57f)
#define RS06_V_MIN_RAD_S                (-50.0f)
#define RS06_V_MAX_RAD_S                (50.0f)
#define RS06_T_MIN_NM                   (-36.0f)
#define RS06_T_MAX_NM                   (36.0f)
#define RS06_KP_MIN                     (0.0f)
#define RS06_KP_MAX                     (5000.0f)
#define RS06_KD_MIN                     (0.0f)
#define RS06_KD_MAX                     (100.0f)

typedef enum {
    RS06_PRIVATE_TYPE_DEVICE_ID       = 0x00U,
    RS06_PRIVATE_TYPE_MOTION_CONTROL  = 0x01U,
    RS06_PRIVATE_TYPE_FEEDBACK        = 0x02U,
    RS06_PRIVATE_TYPE_ENABLE          = 0x03U,
    RS06_PRIVATE_TYPE_STOP            = 0x04U,
    RS06_PRIVATE_TYPE_SET_ZERO        = 0x06U,
    RS06_PRIVATE_TYPE_SET_CAN_ID      = 0x07U,
    RS06_PRIVATE_TYPE_READ_PARAM      = 0x11U,
    RS06_PRIVATE_TYPE_WRITE_PARAM     = 0x12U,
    RS06_PRIVATE_TYPE_FAULT_REPORT    = 0x15U,
    RS06_PRIVATE_TYPE_SAVE_PARAMS     = 0x16U,
    RS06_PRIVATE_TYPE_SET_BAUDRATE    = 0x17U,
    RS06_PRIVATE_TYPE_REPORT_CONTROL  = 0x18U,
    RS06_PRIVATE_TYPE_SET_PROTOCOL    = 0x19U,
} rs06_private_type_t;

typedef enum {
    RS06_MODE_RESET = 0U,
    RS06_MODE_CALIBRATION = 1U,
    RS06_MODE_MOTOR = 2U,
} rs06_motor_mode_t;

typedef enum {
    RS06_PROTOCOL_PRIVATE = 0U,
    RS06_PROTOCOL_CANOPEN = 1U,
    RS06_PROTOCOL_MIT = 2U,
} rs06_protocol_t;

typedef enum {
    RS06_MIT_FRAME_MOTION = 0U,
    RS06_MIT_FRAME_POSITION = 1U,
    RS06_MIT_FRAME_VELOCITY = 2U,
    RS06_MIT_FRAME_CONFIG = 3U,
} rs06_mit_frame_type_t;

typedef struct {
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float kp;
    float kd;
} rs06_motion_cmd_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint8_t host_id;
    rs06_motor_mode_t mode;
    uint8_t fault_bits;
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float temperature_c;
} rs06_feedback_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint8_t host_id;
    uint64_t uid;
} rs06_device_id_t;

typedef struct {
    uint8_t bus;
    uint8_t host_id;
    uint8_t status;
    uint16_t index;
    uint32_t raw_u32;
    float value_f32;
} rs06_param_response_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint32_t fault_value;
    uint32_t warning_value;
} rs06_fault_report_t;

typedef struct {
    uint8_t type;
    uint16_t data;
    uint8_t target_id;
} rs06_private_id_fields_t;

uint16_t rs06_float_to_uint(float value, float min_value, float max_value, uint8_t bits);
float rs06_uint_to_float(uint16_t value, float min_value, float max_value, uint8_t bits);

uint32_t rs06_private_make_id(uint8_t type, uint16_t data, uint8_t target_id);
bool rs06_private_parse_id(uint32_t id, rs06_private_id_fields_t *fields);

bool rs06_private_encode_device_id_query(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame);
bool rs06_private_decode_device_id_response(const can_if_frame_t *frame, rs06_device_id_t *device);
bool rs06_private_encode_enable(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame);
bool rs06_private_encode_stop(uint8_t bus, uint8_t motor_id, uint8_t host_id, bool clear_fault, can_if_frame_t *frame);
bool rs06_private_encode_set_zero(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame);
bool rs06_private_encode_set_can_id(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint8_t new_motor_id, can_if_frame_t *frame);
bool rs06_private_encode_motion_control(uint8_t bus, uint8_t motor_id, const rs06_motion_cmd_t *cmd, can_if_frame_t *frame);
bool rs06_private_decode_feedback(const can_if_frame_t *frame, rs06_feedback_t *feedback);

bool rs06_private_encode_read_param(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint16_t index, can_if_frame_t *frame);
bool rs06_private_encode_write_param_f32(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint16_t index, float value, can_if_frame_t *frame);
bool rs06_private_encode_save_params(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame);
bool rs06_private_encode_report_control(uint8_t bus, uint8_t motor_id, uint8_t host_id, bool enable, can_if_frame_t *frame);
bool rs06_private_encode_set_protocol(uint8_t bus, uint8_t motor_id, uint8_t host_id, rs06_protocol_t protocol, can_if_frame_t *frame);
bool rs06_private_decode_param_response(const can_if_frame_t *frame, rs06_param_response_t *response);
bool rs06_private_decode_fault_report(const can_if_frame_t *frame, rs06_fault_report_t *report);

bool rs06_mit_encode_enable(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame);
bool rs06_mit_encode_stop(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame);
bool rs06_mit_encode_set_zero(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame);
bool rs06_mit_encode_motion_control(uint8_t bus, uint8_t motor_id, const rs06_motion_cmd_t *cmd, can_if_frame_t *frame);
bool rs06_mit_decode_feedback(const can_if_frame_t *frame, uint8_t expected_host_id, rs06_feedback_t *feedback);

#endif
