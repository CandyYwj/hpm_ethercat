/*
 * Author: BearKing
 */
#ifndef YESENSE_PARSER_H
#define YESENSE_PARSER_H

#include "yesense_types.h"
#include <stdint.h>

#define YESENSE_MAX_PAYLOAD_SIZE 255U
#define YESENSE_MAX_FRAME_SIZE   (YESENSE_MAX_PAYLOAD_SIZE + 7U)

typedef enum {
    yesense_parse_none = 0,
    yesense_parse_frame,
    yesense_parse_checksum_error,
    yesense_parse_format_error,
} yesense_parse_result_t;

typedef struct {
    uint8_t frame[YESENSE_MAX_FRAME_SIZE];
    uint16_t frame_length;
    uint16_t expected_length;
} yesense_parser_t;

void yesense_parser_init(yesense_parser_t *parser);
yesense_parse_result_t yesense_parser_push(yesense_parser_t *parser,
                                           uint8_t byte,
                                           imu_sample_t *sample);

#endif /* YESENSE_PARSER_H */
