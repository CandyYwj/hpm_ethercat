/*
 * robstride_proto.c - RobStride RS02/RS03 private CAN protocol encoder/decoder.
 * Author: BearKing
 */

#include "robstride_proto.h"
#include <math.h>
#include <string.h>

#define ROBSTRIDE_PARAM_RUN_MODE             ROBSTRIDE_PARAM_INDEX_RUN_MODE

static const robstride_motor_limits_t s_motor_limits[] = {
    [ROBSTRIDE_MODEL_RS02] = {
        .position_min_rad = -12.566371f,
        .position_max_rad = 12.566371f,
        .velocity_min_rad_s = -44.0f,
        .velocity_max_rad_s = 44.0f,
        .torque_min_nm = -17.0f,
        .torque_max_nm = 17.0f,
        .kp_min = 0.0f,
        .kp_max = 500.0f,
        .kd_min = 0.0f,
        .kd_max = 5.0f,
        .current_min_a = -16.0f,
        .current_max_a = 16.0f,
        .mode_velocity_min_rad_s = -33.0f,
        .mode_velocity_max_rad_s = 33.0f,
    },
    [ROBSTRIDE_MODEL_RS03] = {
        .position_min_rad = -12.57f,
        .position_max_rad = 12.57f,
        .velocity_min_rad_s = -20.0f,
        .velocity_max_rad_s = 20.0f,
        .torque_min_nm = -60.0f,
        .torque_max_nm = 60.0f,
        .kp_min = 0.0f,
        .kp_max = 5000.0f,
        .kd_min = 0.0f,
        .kd_max = 100.0f,
        .current_min_a = -43.0f,
        .current_max_a = 43.0f,
        .mode_velocity_min_rad_s = -20.0f,
        .mode_velocity_max_rad_s = 20.0f,
    },
};

static bool value_in_range(float value, float min_value, float max_value)
{
    return isfinite(value) && (value >= min_value) && (value <= max_value);
}

static bool valid_run_mode(robstride_run_mode_t mode)
{
    return mode == ROBSTRIDE_RUN_MODE_OPERATION;
}

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
    frame->id = robstride_private_make_id(type, data_field, target_id);
    frame->dlc = ROBSTRIDE_PRIVATE_DLC;
    return true;
}

uint16_t robstride_float_to_uint(float value, float min_value, float max_value, uint8_t bits)
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

float robstride_uint_to_float(uint16_t value, float min_value, float max_value, uint8_t bits)
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

const robstride_motor_limits_t *robstride_get_motor_limits(robstride_motor_model_t model)
{
    if ((unsigned int)model
        >= (sizeof(s_motor_limits) / sizeof(s_motor_limits[0]))) {
        return NULL;
    }

    return &s_motor_limits[model];
}

uint32_t robstride_private_make_id(uint8_t type, uint16_t data, uint8_t target_id)
{
    return ((((uint32_t)type) & 0x1FUL) << 24)
         | (((uint32_t)data) << 8)
         | target_id;
}

bool robstride_private_parse_id(uint32_t id, robstride_private_id_fields_t *fields)
{
    if ((fields == NULL) || ((id & ~ROBSTRIDE_PRIVATE_CAN_ID_MASK) != 0U)) {
        return false;
    }

    fields->type = (uint8_t)((id >> 24) & 0x1FU);
    fields->data = (uint16_t)((id >> 8) & 0xFFFFU);
    fields->target_id = (uint8_t)id;
    return true;
}

bool robstride_private_encode_enable(uint8_t bus, uint8_t motor_id, uint8_t host_id, can_if_frame_t *frame)
{
    return prepare_private_frame(bus, ROBSTRIDE_PRIVATE_TYPE_ENABLE, host_id, motor_id, frame);
}

bool robstride_private_encode_stop(uint8_t bus, uint8_t motor_id, uint8_t host_id, bool clear_fault, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, ROBSTRIDE_PRIVATE_TYPE_STOP, host_id, motor_id, frame)) {
        return false;
    }

    frame->data[0] = clear_fault ? 0x01U : 0x00U;
    return true;
}

bool robstride_private_encode_motion_control(uint8_t bus, uint8_t motor_id,
                                        robstride_motor_model_t model,
                                        const robstride_motion_cmd_t *cmd,
                                        can_if_frame_t *frame)
{
    const robstride_motor_limits_t *limits = robstride_get_motor_limits(model);
    uint16_t p;
    uint16_t v;
    uint16_t t;
    uint16_t kp;
    uint16_t kd;

    if ((cmd == NULL) || (limits == NULL)
        || !value_in_range(cmd->position_rad, limits->position_min_rad,
                           limits->position_max_rad)
        || !value_in_range(cmd->velocity_rad_s, limits->velocity_min_rad_s,
                           limits->velocity_max_rad_s)
        || !value_in_range(cmd->torque_nm, limits->torque_min_nm,
                           limits->torque_max_nm)
        || !value_in_range(cmd->kp, limits->kp_min, limits->kp_max)
        || !value_in_range(cmd->kd, limits->kd_min, limits->kd_max)) {
        return false;
    }

    p = robstride_float_to_uint(cmd->position_rad, limits->position_min_rad,
                           limits->position_max_rad, 16U);
    v = robstride_float_to_uint(cmd->velocity_rad_s, limits->velocity_min_rad_s,
                           limits->velocity_max_rad_s, 16U);
    t = robstride_float_to_uint(cmd->torque_nm, limits->torque_min_nm,
                           limits->torque_max_nm, 16U);
    kp = robstride_float_to_uint(cmd->kp, limits->kp_min, limits->kp_max, 16U);
    kd = robstride_float_to_uint(cmd->kd, limits->kd_min, limits->kd_max, 16U);

    if (!prepare_private_frame(bus, ROBSTRIDE_PRIVATE_TYPE_MOTION_CONTROL, t, motor_id, frame)) {
        return false;
    }

    write_be_u16(&frame->data[0], p);
    write_be_u16(&frame->data[2], v);
    write_be_u16(&frame->data[4], kp);
    write_be_u16(&frame->data[6], kd);
    return true;
}

bool robstride_private_decode_feedback(const can_if_frame_t *frame,
                                  robstride_motor_model_t model,
                                  robstride_feedback_t *feedback)
{
    const robstride_motor_limits_t *limits = robstride_get_motor_limits(model);
    robstride_private_id_fields_t fields;
    uint16_t raw_p;
    uint16_t raw_v;
    uint16_t raw_t;
    uint16_t raw_temp;

    if ((frame == NULL) || (feedback == NULL) || (limits == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc != ROBSTRIDE_PRIVATE_DLC) ||
        !robstride_private_parse_id(frame->id, &fields) ||
        (fields.type != ROBSTRIDE_PRIVATE_TYPE_FEEDBACK)) {
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
    feedback->mode = (robstride_motor_mode_t)((fields.data >> 14) & 0x03U);
    feedback->position_rad = robstride_uint_to_float(raw_p, limits->position_min_rad,
                                                limits->position_max_rad, 16U);
    feedback->velocity_rad_s = robstride_uint_to_float(raw_v, limits->velocity_min_rad_s,
                                                  limits->velocity_max_rad_s, 16U);
    feedback->torque_nm = robstride_uint_to_float(raw_t, limits->torque_min_nm,
                                             limits->torque_max_nm, 16U);
    feedback->temperature_c = (float)raw_temp * 0.1f;
    return true;
}

bool robstride_private_encode_read_param(uint8_t bus, uint8_t motor_id, uint8_t host_id, uint16_t index, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, ROBSTRIDE_PRIVATE_TYPE_READ_PARAM, host_id, motor_id, frame)) {
        return false;
    }

    write_le_u16(&frame->data[0], index);
    return true;
}

static bool encode_write_param_raw(uint8_t bus, uint8_t motor_id,
                                   uint8_t host_id, uint16_t index,
                                   uint32_t value, can_if_frame_t *frame)
{
    if (!prepare_private_frame(bus, ROBSTRIDE_PRIVATE_TYPE_WRITE_PARAM,
                               host_id, motor_id, frame)) {
        return false;
    }

    write_le_u16(&frame->data[0], index);
    write_le_u32(&frame->data[4], value);
    return true;
}

bool robstride_private_encode_write_param_u8(uint8_t bus, uint8_t motor_id,
                                        uint8_t host_id, uint16_t index,
                                        uint8_t value, can_if_frame_t *frame)
{
    return encode_write_param_raw(bus, motor_id, host_id, index, value, frame);
}

bool robstride_private_encode_set_run_mode(uint8_t bus, uint8_t motor_id,
                                      uint8_t host_id, robstride_run_mode_t mode,
                                      can_if_frame_t *frame)
{
    return valid_run_mode(mode)
        && robstride_private_encode_write_param_u8(bus, motor_id, host_id,
                                              ROBSTRIDE_PARAM_RUN_MODE,
                                              (uint8_t)mode, frame);
}

bool robstride_private_decode_param_response(const can_if_frame_t *frame, robstride_param_response_t *response)
{
    robstride_private_id_fields_t fields;

    if ((frame == NULL) || (response == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc != ROBSTRIDE_PRIVATE_DLC) ||
        !robstride_private_parse_id(frame->id, &fields) ||
        (fields.type != ROBSTRIDE_PRIVATE_TYPE_READ_PARAM)) {
        return false;
    }

    memset(response, 0, sizeof(*response));
    response->bus = frame->bus;
    response->status = (uint8_t)((fields.data >> 8) & 0xFFU);
    response->motor_id = (uint8_t)(fields.data & 0xFFU);
    response->host_id = fields.target_id;
    response->index = read_le_u16(&frame->data[0]);
    response->raw_u32 = read_le_u32(&frame->data[4]);
    response->value_f32 = read_le_f32(&frame->data[4]);
    return true;
}

bool robstride_private_decode_fault_report(const can_if_frame_t *frame, robstride_fault_report_t *report)
{
    robstride_private_id_fields_t fields;

    if ((frame == NULL) || (report == NULL) ||
        (frame->id_format != CAN_IF_ID_EXTENDED) ||
        (frame->frame_format != CAN_IF_FRAME_CLASSIC) ||
        (frame->dlc != ROBSTRIDE_PRIVATE_DLC) ||
        !robstride_private_parse_id(frame->id, &fields) ||
        (fields.type != ROBSTRIDE_PRIVATE_TYPE_FAULT_REPORT)) {
        return false;
    }

    memset(report, 0, sizeof(*report));
    report->bus = frame->bus;
    report->motor_id = (uint8_t)(fields.data & 0xFFU);
    report->fault_value = read_le_u32(&frame->data[0]);
    report->warning_value = read_le_u32(&frame->data[4]);
    return true;
}



