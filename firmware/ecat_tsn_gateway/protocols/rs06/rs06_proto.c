/*
 * rs06_proto.c - RobStride RS06 CAN protocol encoder/decoder.
 * Author: BearKing
 */

#include "rs06_proto.h"
#include <string.h>

#define RS06_STD_ID_MASK                0x7FFUL

static uint16_t read_be_u16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static void write_be_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)value;
}

static uint16_t read_le_u16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
}

static uint32_t read_le_u32(const uint8_t *data)
{
    return ((uint32_t)data[0])
        | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16)
        | ((uint32_t)data[3] << 24);
}

static void write_le_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void write_le_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void write_le_f32(uint8_t *data, float value)
{
    uint32_t raw;

    memcpy(&raw, &value, sizeof(raw));
    write_le_u32(data, raw);
}

static float read_le_f32(const uint8_t *data)
{
    uint32_t raw = read_le_u32(data);
    float value;

    memcpy(&value, &raw, sizeof(value));
    return value;
}

static bool prepare_private_frame(uint8_t bus, uint8_t type, uint16_t data_field, uint8_t target_id, can_if_frame_t *frame)
{
    if ((frame == NULL) || (bus >= CAN_IF_BUS_COUNT) || (target_id == 0U)) {
        return false;
    }

    memset(frame, 0, sizeof(*frame));
    frame->bus = bus;
    frame->id_format = CAN_IF_ID_EXTENDED;
    frame->frame_format = CAN_IF_FRAME_CLASSIC;
    frame->id = rs06_private_make_id(type, data_field, target_id);
    frame->dlc = RS06_PRIVATE_DLC;
    return true;
}

static bool prepare_mit_frame(uint8_t bus, uint8_t motor_id, rs06_mit_frame_type_t type, can_if_frame_t *frame)
{
    if ((frame == NULL) || (bus >= CAN_IF_BUS_COUNT) || (motor_id == 0U)) {
        return false;
    }

    memset(frame, 0, sizeof(*frame));
    frame->bus = bus;
    frame->id_format = CAN_IF_ID_STANDARD;
    frame->frame_format = CAN_IF_FRAME_CLASSIC;
    frame->id = (((uint32_t)type & 0x7UL) << 8) | motor_id;
    frame->dlc = RS06_PRIVATE_DLC;
    return true;
}

uint16_t rs06_float_to_uint(float value, float min_value, float max_value, uint8_t bits)
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

    return (uint16_t)(scaled + 0.5f);
}

float rs06_uint_to_float(uint16_t value, float min_value, float max_value, uint8_t bits)
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

uint32_t rs06_private_make_id(uint8_t type, uint16_t data, uint8_t target_id)
{
    return ((((uint32_t)type) & 0x1FUL) << 24)
         | (((uint32_t)data) << 8)
         | target_id;
}

bool rs06_private_parse_id(uint32_t id, rs06_private_id_fields_t *fields)
{
    if ((fields == NULL) || ((id & ~RS06_PRIVATE_CAN_ID_MASK) != 0U)) {
        return false;
    }

    fields->type = (uint8_t)((id >> 24) & 0x1FU);
    fields->data = (uint16_t)((id >> 8) & 0xFFFFU);
    fields->target_id = (uint8_t)id;
    return true;
}

bool rs06_private_encode_device_id_query(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame)
{
    return prepare_private_frame(bus, RS06_PRIVATE_TYPE_DEVICE_ID, host_id, motor_id, frame);
}

bool rs06_private_decode_device_id_response(const can_if_frame_t *frame, rs06_device_id_t *device)
{
    rs06_private_id_fields_t fields;
    uint8_t i;
    uint64_t uid = 0;

    if ((frame == NULL) || (device == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc < RS06_PRIVATE_DLC) ||
        !rs06_private_parse_id(frame->id, &fields) ||
        (fields.type != RS06_PRIVATE_TYPE_DEVICE_ID) ||
        (fields.target_id != RS06_BROADCAST_RESPONSE_ID)) {
        return false;
    }

    for (i = 0; i < 8U; i++) {
        uid = (uid << 8) | frame->data[i];
    }

    memset(device, 0, sizeof(*device));
    device->bus = frame->bus;
    device->motor_id = (uint8_t)fields.data;
    device->host_id = fields.target_id;
    device->uid = uid;
    return true;
}

bool rs06_private_encode_enable(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame)
{
    return prepare_private_frame(bus, RS06_PRIVATE_TYPE_ENABLE, host_id, motor_id, frame);
}

bool rs06_private_encode_stop(uint8_t bus, uint8_t motor_id, uint8_t host_id, bool clear_fault, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_STOP, host_id, motor_id, frame)) {
        return false;
    }

    frame->data[0] = clear_fault ? 0x01U : 0x00U;
    return true;
}

bool rs06_private_encode_set_zero(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame)
{
    return prepare_private_frame(bus, RS06_PRIVATE_TYPE_SET_ZERO, host_id, motor_id, frame);
}

bool rs06_private_encode_set_can_id(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint8_t new_motor_id, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_SET_CAN_ID, host_id, motor_id, frame)) {
        return false;
    }

    frame->data[7] = new_motor_id;
    return true;
}

bool rs06_private_encode_motion_control(uint8_t bus, uint8_t motor_id, const rs06_motion_cmd_t *cmd, can_if_frame_t *frame)
{
    uint16_t p;
    uint16_t v;
    uint16_t t;
    uint16_t kp;
    uint16_t kd;

    if (cmd == NULL) {
        return false;
    }

    p = rs06_float_to_uint(cmd->position_rad, RS06_P_MIN_RAD, RS06_P_MAX_RAD, 16U);
    v = rs06_float_to_uint(cmd->velocity_rad_s, RS06_V_MIN_RAD_S, RS06_V_MAX_RAD_S, 16U);
    t = rs06_float_to_uint(cmd->torque_nm, RS06_T_MIN_NM, RS06_T_MAX_NM, 16U);
    kp = rs06_float_to_uint(cmd->kp, RS06_KP_MIN, RS06_KP_MAX, 16U);
    kd = rs06_float_to_uint(cmd->kd, RS06_KD_MIN, RS06_KD_MAX, 16U);

    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_MOTION_CONTROL, t, motor_id, frame)) {
        return false;
    }

    write_be_u16(&frame->data[0], p);
    write_be_u16(&frame->data[2], v);
    write_be_u16(&frame->data[4], kp);
    write_be_u16(&frame->data[6], kd);
    return true;
}

bool rs06_private_decode_feedback(const can_if_frame_t *frame, rs06_feedback_t *feedback)
{
    rs06_private_id_fields_t fields;
    uint16_t raw_p;
    uint16_t raw_v;
    uint16_t raw_t;
    uint16_t raw_temp;

    if ((frame == NULL) || (feedback == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc < RS06_PRIVATE_DLC) ||
        !rs06_private_parse_id(frame->id, &fields) ||
        (fields.type != RS06_PRIVATE_TYPE_FEEDBACK)) {
        return false;
    }

    raw_p = read_be_u16(&frame->data[0]);
    raw_v = read_be_u16(&frame->data[2]);
    raw_t = read_be_u16(&frame->data[4]);
    raw_temp = read_be_u16(&frame->data[6]);

    memset(feedback, 0, sizeof(*feedback));
    feedback->bus = frame->bus;
    feedback->host_id = fields.target_id;
    feedback->motor_id = (uint8_t)(fields.data & 0xFFU);
    feedback->fault_bits = (uint8_t)((fields.data >> 8) & 0x3FU);
    feedback->mode = (rs06_motor_mode_t)((fields.data >> 14) & 0x03U);
    feedback->position_rad = rs06_uint_to_float(raw_p, RS06_P_MIN_RAD, RS06_P_MAX_RAD, 16U);
    feedback->velocity_rad_s = rs06_uint_to_float(raw_v, RS06_V_MIN_RAD_S, RS06_V_MAX_RAD_S, 16U);
    feedback->torque_nm = rs06_uint_to_float(raw_t, RS06_T_MIN_NM, RS06_T_MAX_NM, 16U);
    feedback->temperature_c = (float)raw_temp * 0.1f;
    return true;
}

bool rs06_private_encode_read_param(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint16_t index, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_READ_PARAM, host_id, motor_id, frame)) {
        return false;
    }

    write_le_u16(&frame->data[0], index);
    return true;
}

bool rs06_private_encode_write_param_f32(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint16_t index, float value, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_WRITE_PARAM, host_id, motor_id, frame)) {
        return false;
    }

    write_le_u16(&frame->data[0], index);
    write_le_f32(&frame->data[4], value);
    return true;
}

bool rs06_private_encode_save_params(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame)
{
    return prepare_private_frame(bus, RS06_PRIVATE_TYPE_SAVE_PARAMS, host_id, motor_id, frame);
}

bool rs06_private_encode_report_control(uint8_t bus, uint8_t motor_id, uint8_t host_id, bool enable, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_REPORT_CONTROL, host_id, motor_id, frame)) {
        return false;
    }

    frame->data[0] = enable ? 0x01U : 0x00U;
    return true;
}

bool rs06_private_encode_set_protocol(uint8_t bus, uint8_t motor_id, uint8_t host_id, rs06_protocol_t protocol, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, RS06_PRIVATE_TYPE_SET_PROTOCOL, host_id, motor_id, frame)) {
        return false;
    }

    frame->data[6] = (uint8_t)protocol;
    return true;
}

bool rs06_private_decode_param_response(const can_if_frame_t *frame, rs06_param_response_t *response)
{
    rs06_private_id_fields_t fields;

    if ((frame == NULL) || (response == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc < RS06_PRIVATE_DLC) ||
        !rs06_private_parse_id(frame->id, &fields) ||
        (fields.type != RS06_PRIVATE_TYPE_READ_PARAM)) {
        return false;
    }

    memset(response, 0, sizeof(*response));
    response->bus = frame->bus;
    response->status = (uint8_t)((fields.data >> 8) & 0xFFU);
    response->host_id = (uint8_t)(fields.data & 0xFFU);
    response->index = read_le_u16(&frame->data[0]);
    response->raw_u32 = read_le_u32(&frame->data[4]);
    response->value_f32 = read_le_f32(&frame->data[4]);
    return true;
}

bool rs06_private_decode_fault_report(const can_if_frame_t *frame, rs06_fault_report_t *report)
{
    rs06_private_id_fields_t fields;

    if ((frame == NULL) || (report == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc < RS06_PRIVATE_DLC) ||
        !rs06_private_parse_id(frame->id, &fields) ||
        (fields.type != RS06_PRIVATE_TYPE_FAULT_REPORT)) {
        return false;
    }

    memset(report, 0, sizeof(*report));
    report->bus = frame->bus;
    report->motor_id = (uint8_t)(fields.data & 0xFFU);
    report->fault_value = read_le_u32(&frame->data[0]);
    report->warning_value = read_le_u32(&frame->data[4]);
    return true;
}

bool rs06_mit_encode_enable(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame)
{
    if (!prepare_mit_frame(bus, motor_id, RS06_MIT_FRAME_MOTION, frame)) {
        return false;
    }

    memset(frame->data, 0xFF, sizeof(frame->data));
    frame->data[7] = 0xFCU;
    return true;
}

bool rs06_mit_encode_stop(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame)
{
    if (!prepare_mit_frame(bus, motor_id, RS06_MIT_FRAME_MOTION, frame)) {
        return false;
    }

    memset(frame->data, 0xFF, sizeof(frame->data));
    frame->data[7] = 0xFDU;
    return true;
}

bool rs06_mit_encode_set_zero(uint8_t bus, uint8_t motor_id, can_if_frame_t *frame)
{
    if (!prepare_mit_frame(bus, motor_id, RS06_MIT_FRAME_MOTION, frame)) {
        return false;
    }

    memset(frame->data, 0xFF, sizeof(frame->data));
    frame->data[7] = 0xFEU;
    return true;
}

bool rs06_mit_encode_motion_control(uint8_t bus, uint8_t motor_id, const rs06_motion_cmd_t *cmd, can_if_frame_t *frame)
{
    uint16_t p;
    uint16_t v;
    uint16_t kp;
    uint16_t kd;
    uint16_t t;

    if (cmd == NULL) {
        return false;
    }

    p = rs06_float_to_uint(cmd->position_rad, RS06_P_MIN_RAD, RS06_P_MAX_RAD, 16U);
    v = rs06_float_to_uint(cmd->velocity_rad_s, RS06_V_MIN_RAD_S, RS06_V_MAX_RAD_S, 12U);
    kp = rs06_float_to_uint(cmd->kp, RS06_KP_MIN, RS06_KP_MAX, 12U);
    kd = rs06_float_to_uint(cmd->kd, RS06_KD_MIN, RS06_KD_MAX, 12U);
    t = rs06_float_to_uint(cmd->torque_nm, RS06_T_MIN_NM, RS06_T_MAX_NM, 12U);

    if (!prepare_mit_frame(bus, motor_id, RS06_MIT_FRAME_MOTION, frame)) {
        return false;
    }

    frame->data[0] = (uint8_t)(p >> 8);
    frame->data[1] = (uint8_t)p;
    frame->data[2] = (uint8_t)(v >> 4);
    frame->data[3] = (uint8_t)(((v & 0x0FU) << 4) | ((kp >> 8) & 0x0FU));
    frame->data[4] = (uint8_t)kp;
    frame->data[5] = (uint8_t)(kd >> 4);
    frame->data[6] = (uint8_t)(((kd & 0x0FU) << 4) | ((t >> 8) & 0x0FU));
    frame->data[7] = (uint8_t)t;
    return true;
}

bool rs06_mit_decode_feedback(const can_if_frame_t *frame, uint8_t expected_host_id, rs06_feedback_t *feedback)
{
    uint16_t raw_p;
    uint16_t raw_v;
    uint16_t raw_t;
    uint16_t raw_temp;

    /* RS06 MIT feedback is sent on the host CAN ID (default 0xFD); the motor
     * ID is carried in Data[0]. ENCOS feedback instead uses the motor ID as
     * the CAN ID (1..12). Checking the CAN ID keeps this decoder from
     * consuming ENCOS frames in the shared receive path. */
    if ((frame == NULL) || (feedback == NULL) ||
        (frame->id_format != CAN_IF_ID_STANDARD) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc < RS06_PRIVATE_DLC) ||
        ((frame->id & RS06_STD_ID_MASK) != expected_host_id)) {
        return false;
    }

    raw_p = read_be_u16(&frame->data[1]);
    raw_v = (uint16_t)(((uint16_t)frame->data[3] << 4) | (frame->data[4] >> 4));
    raw_t = (uint16_t)(((uint16_t)(frame->data[4] & 0x0FU) << 8) | frame->data[5]);
    raw_temp = read_be_u16(&frame->data[6]);

    memset(feedback, 0, sizeof(*feedback));
    feedback->bus = frame->bus;
    feedback->host_id = expected_host_id;
    feedback->motor_id = frame->data[0];
    feedback->position_rad = rs06_uint_to_float(raw_p, RS06_P_MIN_RAD, RS06_P_MAX_RAD, 16U);
    feedback->velocity_rad_s = rs06_uint_to_float(raw_v, RS06_V_MIN_RAD_S, RS06_V_MAX_RAD_S, 12U);
    feedback->torque_nm = rs06_uint_to_float(raw_t, RS06_T_MIN_NM, RS06_T_MAX_NM, 12U);
    feedback->temperature_c = (float)raw_temp * 0.1f;
    return true;
}
