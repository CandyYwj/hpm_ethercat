/*
 * Author: BearKing
 */
#ifndef YESENSE_QUERY_H
#define YESENSE_QUERY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define YESENSE_QUERY_MAX_DATA_SIZE 32U
#define YESENSE_QUERY_MAX_FRAME_SIZE (YESENSE_QUERY_MAX_DATA_SIZE + 7U)

#define YESENSE_QUERY_CLASS_PRODUCT          0x00U
#define YESENSE_QUERY_CLASS_BAUD_RATE        0x02U
#define YESENSE_QUERY_CLASS_OUTPUT_FREQUENCY 0x03U
#define YESENSE_QUERY_CLASS_OUTPUT_CONTENT   0x04U
#define YESENSE_QUERY_CLASS_ALGORITHM_MODE   0x4DU
#define YESENSE_QUERY_SUBTYPE_PRODUCT_INFO   0x02U
#define YESENSE_QUERY_SUBTYPE_ALGORITHM_MODE 0x02U

#define YESENSE_DEVICE_INFO_PRODUCT          (1UL << 0)
#define YESENSE_DEVICE_INFO_BAUD_RATE        (1UL << 1)
#define YESENSE_DEVICE_INFO_OUTPUT_FREQUENCY (1UL << 2)
#define YESENSE_DEVICE_INFO_OUTPUT_CONTENT   (1UL << 3)
#define YESENSE_DEVICE_INFO_ALGORITHM_MODE   (1UL << 4)

typedef enum {
    yesense_query_parse_none = 0,
    yesense_query_parse_response,
    yesense_query_parse_checksum_error,
    yesense_query_parse_format_error,
} yesense_query_parse_result_t;

typedef struct {
    uint8_t data_class;
    uint16_t data_length;
    uint8_t data[YESENSE_QUERY_MAX_DATA_SIZE];
} yesense_query_response_t;

typedef struct {
    uint8_t frame[YESENSE_QUERY_MAX_FRAME_SIZE];
    uint16_t frame_length;
    uint16_t expected_length;
    uint8_t expected_data_class;
} yesense_query_parser_t;

typedef struct {
    uint32_t valid_fields;
    char product[31];
    uint32_t baud_rate;
    uint8_t baud_rate_code;
    uint8_t output_frequency_code;
    uint16_t output_content;
    uint8_t algorithm_mode;
} yesense_device_info_t;

size_t yesense_query_build(uint8_t data_class,
                           const uint8_t *data,
                           uint16_t data_length,
                           uint8_t *frame,
                           size_t capacity);
void yesense_query_parser_init(yesense_query_parser_t *parser, uint8_t expected_data_class);
yesense_query_parse_result_t yesense_query_parser_push(yesense_query_parser_t *parser,
                                                        uint8_t byte,
                                                        yesense_query_response_t *response);
bool yesense_device_info_apply(yesense_device_info_t *info,
                               const yesense_query_response_t *response);

#endif /* YESENSE_QUERY_H */
