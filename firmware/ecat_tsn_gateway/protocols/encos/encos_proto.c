/*
 * encos_proto.c - ENCOS classic CAN protocol encoder/decoder.
 * Author: BearKing
 */

#include "encos_proto.h"
#include <string.h>

#define ENCOS_TYPE_MIT_CONTROL       0x00U
#define ENCOS_TYPE_FEEDBACK_1        0x01U
#define ENCOS_TYPE_ENABLE            0x04U
#define ENCOS_STD_ID_MASK            0x7FFUL

/* 电机零点设置指令 (协议文档 7.2):
 *   CAN ID = 0x7FF, 数据 4 字节:
 *   Byte0/1 = 电机 ID 高/低 8 位
 *   Byte2   = 0x00
 *   Byte3   = 0x03 (设置当前位置为零点命令字)
 */

static uint16_t encos_float_to_uint(float value, float min_value, float max_value, uint8_t bits)
{
    uint32_t max_raw;
    float span;
    float scaled;

    if ((bits == 0U) || (bits > 16U)) {
        return 0U;
    }

    if (value < min_value) {
        value = min_value;
    } else if (value > max_value) {
        value = max_value;
    }

    max_raw = (1UL << bits) - 1UL;
    span = max_value - min_value;
    if (span <= 0.0f) {
        return 0U;
    }

    scaled = (value - min_value) * (float)max_raw / span;
    if (scaled <= 0.0f) {
        return 0U;
    }
    if (scaled >= (float)max_raw) {
        return (uint16_t)max_raw;
    }

    return (uint16_t)scaled;
}

static float encos_uint_to_float(uint16_t value, float min_value, float max_value, uint8_t bits)
{
    uint32_t max_raw;

    if ((bits == 0U) || (bits > 16U)) {
        return min_value;
    }

    max_raw = (1UL << bits) - 1UL;
    if (value > max_raw) {
        value = (uint16_t)max_raw;
    }

    return min_value + ((float)value * (max_value - min_value) / (float)max_raw);
}

static bool prepare_std_frame(uint8_t bus, uint8_t motor_id, uint8_t dlc, can_if_frame_t *frame)
{
    if ((frame == NULL) || (bus >= CAN_IF_BUS_COUNT) || (motor_id == 0U) || (dlc > CAN_IF_CLASSIC_MAX_DLC)) {
        return false;
    }

    memset(frame, 0, sizeof(*frame));
    frame->bus = bus;
    frame->id_format = CAN_IF_ID_STANDARD;
    frame->frame_format = CAN_IF_FRAME_CLASSIC;
    frame->id = ((uint32_t)motor_id) & ENCOS_STD_ID_MASK;
    frame->dlc = dlc;
    return true;
}

bool encos_encode_enable(uint8_t bus, uint8_t motor_id, bool enable, can_if_frame_t *frame)
{
    if (!prepare_std_frame(bus, motor_id, ENCOS_DLC_ENABLE, frame)) {
        return false;
    }

    frame->data[0] = (uint8_t)(ENCOS_TYPE_ENABLE << 5);
    frame->data[1] = enable ? 1U : 0U;
    return true;
}

bool encos_encode_stop(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame)
{
    return encos_encode_enable(bus, motor_id, false, frame);
}

bool encos_encode_set_zero(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame)
{
    if ((frame == NULL) || (bus >= CAN_IF_BUS_COUNT) || (motor_id == 0U)) {
        return false;
    }

    memset(frame, 0, sizeof(*frame));
    frame->bus = bus;
    frame->id_format = CAN_IF_ID_STANDARD;
    frame->frame_format = CAN_IF_FRAME_CLASSIC;
    frame->id = ENCOS_ZERO_CAN_ID;
    frame->dlc = ENCOS_DLC_SET_ZERO;
    frame->data[0] = (uint8_t)((motor_id >> 8) & 0xFFU);
    frame->data[1] = (uint8_t)(motor_id & 0xFFU);
    frame->data[2] = 0x00U;
    frame->data[3] = ENCOS_TYPE_SET_ZERO;
    return true;
}

bool encos_encode_motion_control(uint8_t bus, uint8_t motor_id, const encos_motion_cmd_t *cmd, can_if_frame_t *frame)
{
    uint16_t kp;
    uint16_t kd;
    uint16_t pos;
    uint16_t spd;
    uint16_t tor;

    if (cmd == NULL) {
        return false;
    }

    kp = encos_float_to_uint(cmd->kp, ENCOS_KP_MIN, ENCOS_KP_MAX, 12U);
    kd = encos_float_to_uint(cmd->kd, ENCOS_KD_MIN, ENCOS_KD_MAX, 9U);
    pos = encos_float_to_uint(cmd->position_rad, ENCOS_P_MIN_RAD, ENCOS_P_MAX_RAD, 16U);
    spd = encos_float_to_uint(cmd->velocity_rad_s, ENCOS_V_MIN_RAD_S, ENCOS_V_MAX_RAD_S, 12U);
    tor = encos_float_to_uint(cmd->torque_nm, ENCOS_T_MIN_NM, ENCOS_T_MAX_NM, 12U);

    if (!prepare_std_frame(bus, motor_id, ENCOS_DLC_MIT_CONTROL, frame)) {
        return false;
    }

    frame->data[0] = (uint8_t)((ENCOS_TYPE_MIT_CONTROL << 5) | ((kp >> 7) & 0x1FU));
    frame->data[1] = (uint8_t)(((kp & 0x7FU) << 1) | ((kd >> 8) & 0x01U));
    frame->data[2] = (uint8_t)kd;
    frame->data[3] = (uint8_t)(pos >> 8);
    frame->data[4] = (uint8_t)pos;
    frame->data[5] = (uint8_t)(spd >> 4);
    frame->data[6] = (uint8_t)(((spd & 0x0FU) << 4) | ((tor >> 8) & 0x0FU));
    frame->data[7] = (uint8_t)tor;
    return true;
}

bool encos_decode_feedback_type1(const can_if_frame_t *frame, encos_feedback_t *feedback)
{
    uint8_t type;
    uint16_t raw_pos;
    uint16_t raw_spd;
    uint16_t raw_tor;

    if ((frame == NULL) || (feedback == NULL) ||
        (frame->id_format != CAN_IF_ID_STANDARD) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc < ENCOS_DLC_FEEDBACK_TYPE1) ||
        ((frame->id & ~ENCOS_STD_ID_MASK) != 0U)) {
        return false;
    }

    type = (uint8_t)(frame->data[0] >> 5);
    if (type != ENCOS_TYPE_FEEDBACK_1) {
        return false;
    }

    raw_pos = (uint16_t)(((uint16_t)frame->data[1] << 8) | frame->data[2]);
    raw_spd = (uint16_t)(((uint16_t)frame->data[3] << 4) | (frame->data[4] >> 4));
    raw_tor = (uint16_t)(((uint16_t)(frame->data[4] & 0x0FU) << 8) | frame->data[5]);

    memset(feedback, 0, sizeof(*feedback));
    feedback->bus = frame->bus;
    feedback->motor_id = (uint8_t)(frame->id & 0xFFU);
    feedback->fault_bits = frame->data[0] & 0x1FU;
    feedback->position_rad = encos_uint_to_float(raw_pos, ENCOS_P_MIN_RAD, ENCOS_P_MAX_RAD, 16U);
    feedback->velocity_rad_s = encos_uint_to_float(raw_spd, ENCOS_V_MIN_RAD_S, ENCOS_V_MAX_RAD_S, 12U);
    feedback->torque_nm = encos_uint_to_float(raw_tor, ENCOS_I_MIN_A, ENCOS_I_MAX_A, 12U);
    feedback->temperature_c = (((float)frame->data[6]) - 50.0f) * 0.5f;
    return true;
}
