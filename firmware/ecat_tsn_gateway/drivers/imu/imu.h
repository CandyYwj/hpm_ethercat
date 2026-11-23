/*
 * Author: BearKing
 */
#ifndef IMU_H
#define IMU_H

#include "yesense_types.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t received_bytes;
    uint32_t valid_frames;
    uint32_t checksum_errors;
    uint32_t format_errors;
    uint32_t dma_overruns;
    uint32_t dma_errors;
    uint32_t last_frame_tick;
} imu_stats_t;

/* Initialize UART/DMA state. Must run from task context after the scheduler starts. */
bool imu_init(void);

/* Consume a bounded amount of DMA data without blocking. Called by the IMU task. */
void imu_process_rx(void);

bool imu_get_latest(imu_sample_t *sample);
void imu_get_stats(imu_stats_t *stats);
void imu_print_status(void);

#endif /* IMU_H */
