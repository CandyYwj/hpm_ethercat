/*
 * Author: BearKing
 */
#include "yesense_query.h"

#include <string.h>

#define YESENSE_HEADER_FIRST      0x59U
#define YESENSE_HEADER_SECOND     0x53U
#define YESENSE_QUERY_OPERATOR    0U
#define YESENSE_OPERATOR_MASK     0x07U
#define YESENSE_DATA_LENGTH_SHIFT 3U

static void checksum_update(uint8_t byte, uint8_t *checksum_a, uint8_t *checksum_b)
{
    *checksum_a = (uint8_t)(*checksum_a + byte);
    *checksum_b = (uint8_t)(*checksum_b + *checksum_a);
}

static void parser_restart_from_byte(yesense_query_parser_t *parser, uint8_t byte)
{
    parser->expected_length = 0U;
    if (byte == YESENSE_HEADER_FIRST) {
        parser->frame[0] = byte;
        parser->frame_length = 1U;
    } else {
        parser->frame_length = 0U;
    }
}

static uint32_t baud_rate_from_code(uint8_t code)
{
    switch (code) {
    case 0x01U:
        return 9600U;
    case 0x02U:
        return 38400U;
    case 0x03U:
        return 115200U;
    case 0x04U:
        return 460800U;
    case 0x05U:
        return 921600U;
    case 0x06U:
        return 19200U;
    case 0x07U:
        return 57600U;
    case 0x08U:
        return 76800U;
    case 0x09U:
        return 230400U;
    default:
        return 0U;
    }
}

size_t yesense_query_build(uint8_t data_class,
                           const uint8_t *data,
                           uint16_t data_length,
                           uint8_t *frame,
                           size_t capacity)
{
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    uint16_t operator_length;
    size_t frame_length = (size_t)data_length + 7U;

    if ((frame == NULL)
        || (data_length > YESENSE_QUERY_MAX_DATA_SIZE)
        || ((data_length != 0U) && (data == NULL))
        || (capacity < frame_length)) {
        return 0U;
    }

    operator_length = (uint16_t)((data_length << YESENSE_DATA_LENGTH_SHIFT)
                                 | YESENSE_QUERY_OPERATOR);
    frame[0] = YESENSE_HEADER_FIRST;
    frame[1] = YESENSE_HEADER_SECOND;
    frame[2] = data_class;
    frame[3] = (uint8_t)operator_length;
    frame[4] = (uint8_t)(operator_length >> 8);
    if (data_length != 0U) {
        memcpy(&frame[5], data, data_length);
    }

    for (size_t i = 2U; i < (size_t)data_length + 5U; i++) {
        checksum_update(frame[i], &checksum_a, &checksum_b);
    }
    frame[5U + data_length] = checksum_a;
    frame[6U + data_length] = checksum_b;
    return frame_length;
}

void yesense_query_parser_init(yesense_query_parser_t *parser, uint8_t expected_data_class)
{
    if (parser != NULL) {
        parser->frame_length = 0U;
        parser->expected_length = 0U;
        parser->expected_data_class = expected_data_class;
    }
}

yesense_query_parse_result_t yesense_query_parser_push(yesense_query_parser_t *parser,
                                                        uint8_t byte,
                                                        yesense_query_response_t *response)
{
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    uint16_t operator_length;
    uint16_t data_length;

    if ((parser == NULL) || (response == NULL)) {
        return yesense_query_parse_format_error;
    }

    if (parser->frame_length == 0U) {
        if (byte == YESENSE_HEADER_FIRST) {
            parser->frame[0] = byte;
            parser->frame_length = 1U;
        }
        return yesense_query_parse_none;
    }

    if (parser->frame_length == 1U) {
        if (byte == YESENSE_HEADER_SECOND) {
            parser->frame[1] = byte;
            parser->frame_length = 2U;
        } else if (byte != YESENSE_HEADER_FIRST) {
            parser->frame_length = 0U;
        }
        return yesense_query_parse_none;
    }

    if ((parser->frame_length == 2U) && (byte != parser->expected_data_class)) {
        parser_restart_from_byte(parser, byte);
        return yesense_query_parse_none;
    }

    if (parser->frame_length >= YESENSE_QUERY_MAX_FRAME_SIZE) {
        parser_restart_from_byte(parser, byte);
        return yesense_query_parse_format_error;
    }

    parser->frame[parser->frame_length++] = byte;
    if (parser->frame_length == 5U) {
        operator_length = (uint16_t)parser->frame[3]
                        | ((uint16_t)parser->frame[4] << 8);
        data_length = operator_length >> YESENSE_DATA_LENGTH_SHIFT;
        if (((operator_length & YESENSE_OPERATOR_MASK) != YESENSE_QUERY_OPERATOR)
            || (data_length > YESENSE_QUERY_MAX_DATA_SIZE)) {
            parser_restart_from_byte(parser, byte);
            return yesense_query_parse_format_error;
        }
        parser->expected_length = data_length + 7U;
    }

    if ((parser->expected_length == 0U)
        || (parser->frame_length != parser->expected_length)) {
        return yesense_query_parse_none;
    }

    data_length = parser->expected_length - 7U;
    for (uint16_t i = 2U; i < (uint16_t)(data_length + 5U); i++) {
        checksum_update(parser->frame[i], &checksum_a, &checksum_b);
    }
    if ((parser->frame[5U + data_length] != checksum_a)
        || (parser->frame[6U + data_length] != checksum_b)) {
        yesense_query_parser_init(parser, parser->expected_data_class);
        return yesense_query_parse_checksum_error;
    }

    response->data_class = parser->frame[2];
    response->data_length = data_length;
    if (data_length != 0U) {
        memcpy(response->data, &parser->frame[5], data_length);
    }
    yesense_query_parser_init(parser, parser->expected_data_class);
    return yesense_query_parse_response;
}

bool yesense_device_info_apply(yesense_device_info_t *info,
                               const yesense_query_response_t *response)
{
    if ((info == NULL) || (response == NULL)) {
        return false;
    }

    switch (response->data_class) {
    case YESENSE_QUERY_CLASS_PRODUCT:
        if ((response->data_length != 32U)
            || (response->data[0] != YESENSE_QUERY_SUBTYPE_PRODUCT_INFO)
            || (response->data[1] != 30U)) {
            return false;
        }
        for (size_t i = 0U; i < 30U; i++) {
            uint8_t value = response->data[i + 2U];

            if (value == 0U) {
                info->product[i] = '\0';
                break;
            }
            info->product[i] = ((value >= 0x20U) && (value <= 0x7EU))
                             ? (char)value : '?';
            info->product[i + 1U] = '\0';
        }
        info->valid_fields |= YESENSE_DEVICE_INFO_PRODUCT;
        return true;

    case YESENSE_QUERY_CLASS_BAUD_RATE:
        if (response->data_length != 1U) {
            return false;
        }
        info->baud_rate_code = response->data[0];
        info->baud_rate = baud_rate_from_code(response->data[0]);
        info->valid_fields |= YESENSE_DEVICE_INFO_BAUD_RATE;
        return true;

    case YESENSE_QUERY_CLASS_OUTPUT_FREQUENCY:
        if (response->data_length != 1U) {
            return false;
        }
        info->output_frequency_code = response->data[0];
        info->valid_fields |= YESENSE_DEVICE_INFO_OUTPUT_FREQUENCY;
        return true;

    case YESENSE_QUERY_CLASS_OUTPUT_CONTENT:
        if (response->data_length != 2U) {
            return false;
        }
        info->output_content = (uint16_t)response->data[0]
                             | ((uint16_t)response->data[1] << 8);
        info->valid_fields |= YESENSE_DEVICE_INFO_OUTPUT_CONTENT;
        return true;

    case YESENSE_QUERY_CLASS_ALGORITHM_MODE:
        if ((response->data_length != 2U)
            || (response->data[0] != YESENSE_QUERY_SUBTYPE_ALGORITHM_MODE)) {
            return false;
        }
        info->algorithm_mode = response->data[1];
        info->valid_fields |= YESENSE_DEVICE_INFO_ALGORITHM_MODE;
        return true;

    default:
        return false;
    }
}
