/*
 * encos_proto.h - ENCOS classic CAN protocol encoder/decoder.
 * Author: BearKing
 */
#ifndef ENCOS_PROTO_H
#define ENCOS_PROTO_H

#include "can_if.h"
#include <stdbool.h>
#include <stdint.h>

#define ENCOS_DLC_MIT_CONTROL       8U
#define ENCOS_DLC_ENABLE            2U
#define ENCOS_DLC_FEEDBACK_TYPE1    8U

/* 电机零点设置指令 (协议文档 7.2): CAN ID = 0x7FF, 数据 4 字节,
 * Byte0/1 = 电机 ID 高/低 8 位, Byte2 = 0x00, Byte3 = 0x03 (命令字). */
#define ENCOS_DLC_SET_ZERO          4U
#define ENCOS_ZERO_CAN_ID           0x7FFUL
#define ENCOS_TYPE_SET_ZERO         0x03U

#define ENCOS_P_MIN_RAD             (-12.5f)
#define ENCOS_P_MAX_RAD             (12.5f)
#define ENCOS_V_MIN_RAD_S           (-18.0f)
#define ENCOS_V_MAX_RAD_S           (18.0f)
#define ENCOS_T_MIN_NM              (-130.0f)
#define ENCOS_T_MAX_NM              (130.0f)
#define ENCOS_I_MIN_A               (-70.0f)
#define ENCOS_I_MAX_A               (70.0f)
#define ENCOS_KP_MIN                (0.0f)
#define ENCOS_KP_MAX                (500.0f)
#define ENCOS_KD_MIN                (0.0f)
#define ENCOS_KD_MAX                (5.0f)

typedef struct {
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float kp;
    float kd;
} encos_motion_cmd_t;

typedef struct {
    uint8_t bus;
    uint8_t motor_id;
    uint8_t fault_bits;
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float temperature_c;
} encos_feedback_t;

bool encos_encode_enable(uint8_t bus, uint8_t motor_id, bool enable, can_if_frame_t *frame);
bool encos_encode_stop(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame);
bool encos_encode_motion_control(uint8_t bus, uint8_t motor_id, const encos_motion_cmd_t *cmd, can_if_frame_t *frame);
bool encos_encode_set_zero(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame);
bool encos_decode_feedback_type1(const can_if_frame_t *frame, encos_feedback_t *feedback);

#endif
