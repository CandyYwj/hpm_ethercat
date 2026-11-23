/*
 * Author: BearKing
 */
#include "yesense_parser.h"

#include <stddef.h>
#include <string.h>

#define YESENSE_HEADER_FIRST   0x59U
#define YESENSE_HEADER_SECOND  0x53U

#define YESENSE_ID_TEMPERATURE       0x01U
#define YESENSE_ID_ACCELERATION      0x10U
#define YESENSE_ID_ANGULAR_RATE      0x20U
#define YESENSE_ID_MAGNETIC          0x30U
#define YESENSE_ID_MAGNETIC_RAW      0x31U
#define YESENSE_ID_EULER             0x40U
#define YESENSE_ID_QUATERNION        0x41U
#define YESENSE_ID_SAMPLE_TIMESTAMP  0x51U
#define YESENSE_ID_READY_TIMESTAMP   0x52U

static uint16_t read_le_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t read_le_i16(const uint8_t *data)
{
    return (int16_t)read_le_u16(data);
}

static uint32_t read_le_u32(const uint8_t *data)
{
    return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static int32_t read_le_i32(const uint8_t *data)
{
    return (int32_t)read_le_u32(data);
}

static void decode_vector3(const uint8_t *data, float scale, float output[3])
{
    output[0] = (float)read_le_i32(&data[0]) * scale;
    output[1] = (float)read_le_i32(&data[4]) * scale;
    output[2] = (float)read_le_i32(&data[8]) * scale;
}

static int decode_field(uint8_t id, const uint8_t *data, uint8_t length, imu_sample_t *sample)
{
    switch (id) {
    case YESENSE_ID_TEMPERATURE:
        if (length != 2U) {
            return -1;
        }
        sample->temperature_c = (float)read_le_i16(data) * 0.01f;
        sample->valid_fields |= IMU_FIELD_TEMPERATURE;
        return 1;

    case YESENSE_ID_ACCELERATION:
        if (length != 12U) {
            return -1;
        }
        decode_vector3(data, 0.000001f, sample->acceleration_mps2);
        sample->valid_fields |= IMU_FIELD_ACCELERATION;
        return 1;

    case YESENSE_ID_ANGULAR_RATE:
        if (length != 12U) {
            return -1;
        }
        decode_vector3(data, 0.000001f, sample->angular_rate_dps);
        sample->valid_fields |= IMU_FIELD_ANGULAR_RATE;
        return 1;

    case YESENSE_ID_MAGNETIC:
        if (length != 12U) {
            return -1;
        }
        decode_vector3(data, 0.000001f, sample->magnetic_normalized);
        sample->valid_fields |= IMU_FIELD_MAGNETIC_NORMALIZED;
        return 1;

    case YESENSE_ID_MAGNETIC_RAW:
        if (length != 12U) {
            return -1;
        }
        decode_vector3(data, 0.001f, sample->magnetic_mgauss);
        sample->valid_fields |= IMU_FIELD_MAGNETIC_RAW;
        return 1;

    case YESENSE_ID_EULER:
        if (length != 12U) {
            return -1;
        }
        decode_vector3(data, 0.000001f, sample->euler_deg);
        sample->valid_fields |= IMU_FIELD_EULER;
        return 1;

    case YESENSE_ID_QUATERNION:
        if (length != 16U) {
            return -1;
        }
        sample->quaternion[0] = (float)read_le_i32(&data[0]) * 0.000001f;
        sample->quaternion[1] = (float)read_le_i32(&data[4]) * 0.000001f;
        sample->quaternion[2] = (float)read_le_i32(&data[8]) * 0.000001f;
        sample->quaternion[3] = (float)read_le_i32(&data[12]) * 0.000001f;
        sample->valid_fields |= IMU_FIELD_QUATERNION;
        return 1;

    case YESENSE_ID_SAMPLE_TIMESTAMP:
        if (length != 4U) {
            return -1;
        }
        sample->sample_timestamp_us = read_le_u32(data);
        sample->valid_fields |= IMU_FIELD_SAMPLE_TIMESTAMP;
        return 1;

    case YESENSE_ID_READY_TIMESTAMP:
        if (length != 4U) {
            return -1;
        }
        sample->data_ready_timestamp_us = read_le_u32(data);
        sample->valid_fields |= IMU_FIELD_READY_TIMESTAMP;
        return 1;

    default:
        return 0;
    }
}

static yesense_parse_result_t decode_frame(const uint8_t *frame, imu_sample_t *sample)
{
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    uint16_t payload_end;
    uint16_t position;
    uint8_t payload_length = frame[4];

    for (position = 2U; position < (uint16_t)(5U + payload_length); position++) {
        checksum_a = (uint8_t)(checksum_a + frame[position]);
        checksum_b = (uint8_t)(checksum_b + checksum_a);
    }

    if ((frame[5U + payload_length] != checksum_a)
        || (frame[6U + payload_length] != checksum_b)) {
        return yesense_parse_checksum_error;
    }

    memset(sample, 0, sizeof(*sample));
    sample->transaction_id = read_le_u16(&frame[2]);
    position = 5U;
    payload_end = (uint16_t)(5U + payload_length);

    while (position < payload_end) {
        uint8_t id;
        uint8_t length;

        if ((uint32_t)(payload_end - position) < 2UL) {
            return yesense_parse_format_error;
        }

        id = frame[position];
        length = frame[position + 1U];
        position += 2U;
        if ((uint16_t)length > (payload_end - position)) {
            return yesense_parse_format_error;
        }
        if (decode_field(id, &frame[position], length, sample) < 0) {
            return yesense_parse_format_error;
        }
        position += length;
    }

    return yesense_parse_frame;
}

void yesense_parser_init(yesense_parser_t *parser)
{
    if (parser != NULL) {
        parser->frame_length = 0U;
        parser->expected_length = 0U;
    }
}

yesense_parse_result_t yesense_parser_push(yesense_parser_t *parser,
                                           uint8_t byte,
                                           imu_sample_t *sample)
{
    yesense_parse_result_t result;

    if ((parser == NULL) || (sample == NULL)) {
        return yesense_parse_format_error;
    }

    if (parser->frame_length == 0U) {
        if (byte == YESENSE_HEADER_FIRST) {
            parser->frame[0] = byte;
            parser->frame_length = 1U;
        }
        return yesense_parse_none;
    }

    if (parser->frame_length == 1U) {
        if (byte == YESENSE_HEADER_SECOND) {
            parser->frame[1] = byte;
            parser->frame_length = 2U;
        } else if (byte != YESENSE_HEADER_FIRST) {
            parser->frame_length = 0U;
        }
        return yesense_parse_none;
    }

    if (parser->frame_length >= YESENSE_MAX_FRAME_SIZE) {
        yesense_parser_init(parser);
        return yesense_parse_format_error;
    }

    parser->frame[parser->frame_length++] = byte;
    if (parser->frame_length == 5U) {
        parser->expected_length = (uint16_t)parser->frame[4] + 7U;
    }

    if ((parser->expected_length != 0U)
        && (parser->frame_length == parser->expected_length)) {
        result = decode_frame(parser->frame, sample);
        yesense_parser_init(parser);
        return result;
    }

    return yesense_parse_none;
}
