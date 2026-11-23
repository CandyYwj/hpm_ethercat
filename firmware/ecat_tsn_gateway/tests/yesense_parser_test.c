/*
 * Author: BearKing
 */
#include "yesense_parser.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void append_u16(uint8_t *buffer, size_t *position, uint16_t value)
{
    buffer[(*position)++] = (uint8_t)value;
    buffer[(*position)++] = (uint8_t)(value >> 8);
}

static void append_u32(uint8_t *buffer, size_t *position, uint32_t value)
{
    buffer[(*position)++] = (uint8_t)value;
    buffer[(*position)++] = (uint8_t)(value >> 8);
    buffer[(*position)++] = (uint8_t)(value >> 16);
    buffer[(*position)++] = (uint8_t)(value >> 24);
}

static void append_vector(uint8_t *buffer,
                          size_t *position,
                          uint8_t id,
                          int32_t x,
                          int32_t y,
                          int32_t z)
{
    buffer[(*position)++] = id;
    buffer[(*position)++] = 12U;
    append_u32(buffer, position, (uint32_t)x);
    append_u32(buffer, position, (uint32_t)y);
    append_u32(buffer, position, (uint32_t)z);
}

static size_t finish_frame(uint8_t *frame, size_t payload_end)
{
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;

    frame[4] = (uint8_t)(payload_end - 5U);
    for (size_t i = 2U; i < payload_end; i++) {
        checksum_a = (uint8_t)(checksum_a + frame[i]);
        checksum_b = (uint8_t)(checksum_b + checksum_a);
    }
    frame[payload_end++] = checksum_a;
    frame[payload_end++] = checksum_b;
    return payload_end;
}

static size_t build_full_frame(uint8_t *frame)
{
    size_t position = 0U;

    frame[position++] = 0x59U;
    frame[position++] = 0x53U;
    append_u16(frame, &position, 1234U);
    frame[position++] = 0U;

    frame[position++] = 0x01U;
    frame[position++] = 2U;
    append_u16(frame, &position, (uint16_t)(int16_t)-125);
    append_vector(frame, &position, 0x10U, 9810000, -1250000, 500000);
    append_vector(frame, &position, 0x20U, 1000000, -2000000, 3000000);
    append_vector(frame, &position, 0x40U, 10000000, -20000000, 30000000);

    frame[position++] = 0x41U;
    frame[position++] = 16U;
    append_u32(frame, &position, 1000000U);
    append_u32(frame, &position, 0U);
    append_u32(frame, &position, 0U);
    append_u32(frame, &position, 0U);

    frame[position++] = 0x51U;
    frame[position++] = 4U;
    append_u32(frame, &position, 0x12345678U);
    frame[position++] = 0x52U;
    frame[position++] = 4U;
    append_u32(frame, &position, 0x89ABCDEFU);

    frame[position++] = 0x99U;
    frame[position++] = 3U;
    frame[position++] = 1U;
    frame[position++] = 2U;
    frame[position++] = 3U;

    return finish_frame(frame, position);
}

static yesense_parse_result_t feed(yesense_parser_t *parser,
                                   const uint8_t *data,
                                   size_t size,
                                   imu_sample_t *sample)
{
    yesense_parse_result_t result = yesense_parse_none;

    for (size_t i = 0U; i < size; i++) {
        yesense_parse_result_t current = yesense_parser_push(parser, data[i], sample);
        if (current != yesense_parse_none) {
            result = current;
        }
    }
    return result;
}

static int float_close(float actual, float expected)
{
    float difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < 0.0001f;
}

static void test_full_frame_and_noise(void)
{
    uint8_t frame[YESENSE_MAX_FRAME_SIZE];
    const uint8_t noise[] = {0x00U, 0x59U, 0x11U, 0x59U};
    size_t frame_size = build_full_frame(frame);
    yesense_parser_t parser;
    imu_sample_t sample;

    yesense_parser_init(&parser);
    assert(feed(&parser, noise, sizeof(noise), &sample) == yesense_parse_none);
    assert(feed(&parser, frame, frame_size, &sample) == yesense_parse_frame);
    assert(sample.transaction_id == 1234U);
    assert((sample.valid_fields & IMU_FIELD_TEMPERATURE) != 0U);
    assert((sample.valid_fields & IMU_FIELD_ACCELERATION) != 0U);
    assert((sample.valid_fields & IMU_FIELD_ANGULAR_RATE) != 0U);
    assert((sample.valid_fields & IMU_FIELD_EULER) != 0U);
    assert((sample.valid_fields & IMU_FIELD_QUATERNION) != 0U);
    assert((sample.valid_fields & IMU_FIELD_SAMPLE_TIMESTAMP) != 0U);
    assert((sample.valid_fields & IMU_FIELD_READY_TIMESTAMP) != 0U);
    assert(float_close(sample.temperature_c, -1.25f));
    assert(float_close(sample.acceleration_mps2[0], 9.81f));
    assert(float_close(sample.acceleration_mps2[1], -1.25f));
    assert(float_close(sample.angular_rate_dps[2], 3.0f));
    assert(float_close(sample.euler_deg[0], 10.0f));
    assert(float_close(sample.euler_deg[1], -20.0f));
    assert(float_close(sample.euler_deg[2], 30.0f));
    assert(float_close(sample.quaternion[0], 1.0f));
    assert(sample.sample_timestamp_us == 0x12345678U);
    assert(sample.data_ready_timestamp_us == 0x89ABCDEFU);
}

static void test_checksum_error_recovers(void)
{
    uint8_t frame[YESENSE_MAX_FRAME_SIZE];
    size_t frame_size = build_full_frame(frame);
    yesense_parser_t parser;
    imu_sample_t sample;

    yesense_parser_init(&parser);
    frame[frame_size - 1U] ^= 0x01U;
    assert(feed(&parser, frame, frame_size, &sample) == yesense_parse_checksum_error);
    frame[frame_size - 1U] ^= 0x01U;
    assert(feed(&parser, frame, frame_size, &sample) == yesense_parse_frame);
}

static void test_malformed_known_field_is_rejected(void)
{
    uint8_t frame[16] = {0x59U, 0x53U, 1U, 0U, 0U, 0x01U, 1U, 0U};
    size_t frame_size = finish_frame(frame, 8U);
    yesense_parser_t parser;
    imu_sample_t sample;

    yesense_parser_init(&parser);
    assert(feed(&parser, frame, frame_size, &sample) == yesense_parse_format_error);
}

int main(void)
{
    test_full_frame_and_noise();
    test_checksum_error_recovers();
    test_malformed_known_field_is_rejected();
    puts("yesense_parser_test: PASS");
    return 0;
}
