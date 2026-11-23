/*
 * Author: BearKing
 */
#include "yesense_query.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static size_t build_response(uint8_t data_class,
                             const uint8_t *data,
                             uint16_t data_length,
                             uint8_t *frame)
{
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    uint16_t operator_length = (uint16_t)(data_length << 3);
    size_t frame_length = (size_t)data_length + 7U;

    frame[0] = 0x59U;
    frame[1] = 0x53U;
    frame[2] = data_class;
    frame[3] = (uint8_t)operator_length;
    frame[4] = (uint8_t)(operator_length >> 8);
    memcpy(&frame[5], data, data_length);
    for (size_t i = 2U; i < (size_t)data_length + 5U; i++) {
        checksum_a = (uint8_t)(checksum_a + frame[i]);
        checksum_b = (uint8_t)(checksum_b + checksum_a);
    }
    frame[5U + data_length] = checksum_a;
    frame[6U + data_length] = checksum_b;
    return frame_length;
}

static yesense_query_parse_result_t feed(yesense_query_parser_t *parser,
                                          const uint8_t *data,
                                          size_t size,
                                          yesense_query_response_t *response)
{
    yesense_query_parse_result_t result = yesense_query_parse_none;

    for (size_t i = 0U; i < size; i++) {
        yesense_query_parse_result_t current = yesense_query_parser_push(parser,
                                                                         data[i],
                                                                         response);
        if (current != yesense_query_parse_none) {
            result = current;
        }
    }
    return result;
}

static void test_query_frames(void)
{
    uint8_t frame[YESENSE_QUERY_MAX_FRAME_SIZE];
    const uint8_t subtype = 0x02U;
    const uint8_t expected_product[] = {0x59U, 0x53U, 0x00U, 0x08U,
                                        0x00U, 0x02U, 0x0AU, 0x1AU};
    const uint8_t expected_baud[] = {0x59U, 0x53U, 0x02U, 0x00U,
                                     0x00U, 0x02U, 0x06U};
    const uint8_t expected_frequency[] = {0x59U, 0x53U, 0x03U, 0x00U,
                                          0x00U, 0x03U, 0x09U};
    const uint8_t expected_content[] = {0x59U, 0x53U, 0x04U, 0x00U,
                                        0x00U, 0x04U, 0x0CU};
    const uint8_t expected_mode[] = {0x59U, 0x53U, 0x4DU, 0x08U,
                                     0x00U, 0x02U, 0x57U, 0x4EU};

    assert(yesense_query_build(0x00U, &subtype, 1U, frame, sizeof(frame))
           == sizeof(expected_product));
    assert(memcmp(frame, expected_product, sizeof(expected_product)) == 0);
    assert(yesense_query_build(0x02U, NULL, 0U, frame, sizeof(frame))
           == sizeof(expected_baud));
    assert(memcmp(frame, expected_baud, sizeof(expected_baud)) == 0);
    assert(yesense_query_build(0x03U, NULL, 0U, frame, sizeof(frame))
           == sizeof(expected_frequency));
    assert(memcmp(frame, expected_frequency, sizeof(expected_frequency)) == 0);
    assert(yesense_query_build(0x04U, NULL, 0U, frame, sizeof(frame))
           == sizeof(expected_content));
    assert(memcmp(frame, expected_content, sizeof(expected_content)) == 0);
    assert(yesense_query_build(0x4DU, &subtype, 1U, frame, sizeof(frame))
           == sizeof(expected_mode));
    assert(memcmp(frame, expected_mode, sizeof(expected_mode)) == 0);
    assert(yesense_query_build(0x02U, NULL, 1U, frame, sizeof(frame)) == 0U);
}

static void test_all_device_information(void)
{
    uint8_t frame[YESENSE_QUERY_MAX_FRAME_SIZE];
    uint8_t product_data[32] = {0x02U, 30U};
    const uint8_t baud_data[] = {0x04U};
    const uint8_t frequency_data[] = {0x0BU};
    const uint8_t content_data[] = {0xD8U, 0x84U};
    const uint8_t mode_data[] = {0x02U, 0x01U};
    const char product[] = "YISF16A-SUB01-V01.02.03;";
    const struct {
        uint8_t data_class;
        const uint8_t *data;
        uint16_t data_length;
    } responses[] = {
        {0x00U, product_data, sizeof(product_data)},
        {0x02U, baud_data, sizeof(baud_data)},
        {0x03U, frequency_data, sizeof(frequency_data)},
        {0x04U, content_data, sizeof(content_data)},
        {0x4DU, mode_data, sizeof(mode_data)},
    };
    yesense_device_info_t info = {0};

    memcpy(&product_data[2], product, sizeof(product) - 1U);
    for (size_t i = 0U; i < sizeof(responses) / sizeof(responses[0]); i++) {
        yesense_query_parser_t parser;
        yesense_query_response_t response;
        size_t frame_size = build_response(responses[i].data_class,
                                           responses[i].data,
                                           responses[i].data_length,
                                           frame);

        yesense_query_parser_init(&parser, responses[i].data_class);
        assert(feed(&parser, frame, frame_size, &response) == yesense_query_parse_response);
        assert(yesense_device_info_apply(&info, &response));
    }

    assert(info.valid_fields == 0x1FU);
    assert(strcmp(info.product, product) == 0);
    assert(info.baud_rate_code == 0x04U);
    assert(info.baud_rate == 460800U);
    assert(info.output_frequency_code == 0x0BU);
    assert(info.output_content == 0x84D8U);
    assert(info.algorithm_mode == 0x01U);
}

static void test_rejects_bad_or_unexpected_frames(void)
{
    uint8_t frame[YESENSE_QUERY_MAX_FRAME_SIZE];
    const uint8_t oversized_header[] = {0x59U, 0x53U, 0x02U, 0x08U, 0x01U};
    const uint8_t data[] = {0x04U};
    yesense_query_parser_t parser;
    yesense_query_response_t response;
    size_t frame_size = build_response(0x02U, data, sizeof(data), frame);

    yesense_query_parser_init(&parser, 0x03U);
    assert(feed(&parser, frame, frame_size, &response) == yesense_query_parse_none);

    yesense_query_parser_init(&parser, 0x02U);
    frame[frame_size - 1U] ^= 0x01U;
    assert(feed(&parser, frame, frame_size, &response)
           == yesense_query_parse_checksum_error);

    frame[frame_size - 1U] ^= 0x01U;
    frame[3] |= 0x01U;
    yesense_query_parser_init(&parser, 0x02U);
    assert(feed(&parser, frame, frame_size, &response)
           == yesense_query_parse_format_error);

    yesense_query_parser_init(&parser, 0x02U);
    assert(feed(&parser, frame, 0U, &response) == yesense_query_parse_none);

    yesense_query_parser_init(&parser, 0x02U);
    assert(feed(&parser, oversized_header, sizeof(oversized_header), &response)
           == yesense_query_parse_format_error);

    response.data_class = YESENSE_QUERY_CLASS_OUTPUT_CONTENT;
    response.data_length = 1U;
    assert(!yesense_device_info_apply(&(yesense_device_info_t){0}, &response));
}

int main(void)
{
    test_query_frames();
    test_all_device_information();
    test_rejects_bad_or_unexpected_frames();
    puts("yesense_query_test: PASS");
    return 0;
}
