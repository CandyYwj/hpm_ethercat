/*
 * yesense_types.h - Hardware-independent YESENSE sample data.
 * Author: BearKing
 */
#ifndef YESENSE_TYPES_H
#define YESENSE_TYPES_H

#include <stdint.h>

#define IMU_FIELD_TEMPERATURE          (1UL << 0)
#define IMU_FIELD_ACCELERATION         (1UL << 1)
#define IMU_FIELD_ANGULAR_RATE         (1UL << 2)
#define IMU_FIELD_MAGNETIC_NORMALIZED  (1UL << 3)
#define IMU_FIELD_MAGNETIC_RAW         (1UL << 4)
#define IMU_FIELD_EULER                (1UL << 5)
#define IMU_FIELD_QUATERNION           (1UL << 6)
#define IMU_FIELD_SAMPLE_TIMESTAMP     (1UL << 7)
#define IMU_FIELD_READY_TIMESTAMP      (1UL << 8)

typedef struct {
    uint16_t transaction_id;
    uint32_t valid_fields;
    float temperature_c;
    float acceleration_mps2[3];
    float angular_rate_dps[3];
    float magnetic_normalized[3];
    float magnetic_mgauss[3];
    float euler_deg[3];
    float quaternion[4];
    uint32_t sample_timestamp_us;
    uint32_t data_ready_timestamp_us;
    uint32_t received_tick;
} imu_sample_t;

#endif /* YESENSE_TYPES_H */

