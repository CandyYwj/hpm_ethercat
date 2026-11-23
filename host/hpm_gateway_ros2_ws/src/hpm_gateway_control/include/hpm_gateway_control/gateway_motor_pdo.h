#ifndef HPM_GATEWAY_CONTROL_GATEWAY_MOTOR_PDO_H
#define HPM_GATEWAY_CONTROL_GATEWAY_MOTOR_PDO_H

#include <stdint.h>

#define GATEWAY_RT_AXIS_COUNT 16U
/* RxPDO is 16 axes x 20 bytes = 320 bytes, padded to 332 with a 12-byte
 * diagnostic pad (objects 0x7F66..0x7F68 in the SSC object dictionary) so the
 * gateway firmware can probe the ESC process-data delivery boundary. */
#define GATEWAY_RT_PAD_BYTES 12U
#define GATEWAY_RT_OUTPUT_SIZE_BYTES (320U + GATEWAY_RT_PAD_BYTES)
#define GATEWAY_RT_IMU_INPUT_SIZE_BYTES 60U
#define GATEWAY_RT_MOTOR_INPUT_SIZE_BYTES 192U
#define GATEWAY_RT_INPUT_SIZE_BYTES 252U

#define GATEWAY_RT_IMU_REQUIRED_FIELDS \
    ((UINT32_C(1) << 0) | (UINT32_C(1) << 1) | (UINT32_C(1) << 2) | \
     (UINT32_C(1) << 6) | (UINT32_C(1) << 7))

#define GATEWAY_RT_VENDOR_ID 0x0048504DU
#define GATEWAY_RT_PRODUCT_CODE 0x00000001U

typedef struct {
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float kp_nm_per_rad;
    float kd_nm_per_rad_s;
} gateway_rt_motor_command;

typedef struct {
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
} gateway_rt_motor_feedback;

typedef struct {
    uint32_t status;
    uint32_t valid_fields;
    uint32_t received_tick;
    uint32_t sample_timestamp_us;
    float acceleration_mps2[3];
    float angular_rate_dps[3];
    float quaternion[4];
    float temperature_c;
} gateway_rt_imu_feedback;

typedef struct {
    /* axis[n] corresponds to global CAN motor ID n + 1. */
    gateway_rt_motor_command axis[GATEWAY_RT_AXIS_COUNT];
    /* Diagnostic pad mapped to SSC objects 0x7F66..0x7F68 (bytes 320..331).
     * Filled with a distinct pattern by the master; the gateway firmware dumps
     * it to locate the ESC process-data delivery boundary. */
    uint8_t pad[GATEWAY_RT_PAD_BYTES];
} gateway_rt_output_pdo;

typedef struct {
    gateway_rt_imu_feedback imu;
    /* axis[n] corresponds to global CAN motor ID n + 1. */
    gateway_rt_motor_feedback axis[GATEWAY_RT_AXIS_COUNT];
} gateway_rt_input_pdo;

typedef struct {
    uint64_t received_monotonic_us;
    uint32_t sequence;
    uint32_t valid_mask;
    gateway_rt_output_pdo output;
} gateway_rt_command_frame;

typedef struct {
    uint64_t received_monotonic_us;
    uint32_t sequence;
    uint32_t feedback_sequence;
    gateway_rt_input_pdo input;
} gateway_rt_feedback_frame;

#ifdef __cplusplus
#define GATEWAY_RT_STATIC_ASSERT static_assert
#else
#define GATEWAY_RT_STATIC_ASSERT _Static_assert
#endif

GATEWAY_RT_STATIC_ASSERT(sizeof(gateway_rt_motor_command) == 20U,
               "unexpected motor command size");
GATEWAY_RT_STATIC_ASSERT(sizeof(gateway_rt_motor_feedback) == 12U,
               "unexpected motor feedback size");
GATEWAY_RT_STATIC_ASSERT(sizeof(gateway_rt_imu_feedback) == GATEWAY_RT_IMU_INPUT_SIZE_BYTES,
               "unexpected IMU PDO size");
GATEWAY_RT_STATIC_ASSERT(sizeof(gateway_rt_output_pdo) == GATEWAY_RT_OUTPUT_SIZE_BYTES,
               "unexpected output PDO size");
GATEWAY_RT_STATIC_ASSERT(sizeof(gateway_rt_input_pdo) == GATEWAY_RT_INPUT_SIZE_BYTES,
               "unexpected input PDO size");

#undef GATEWAY_RT_STATIC_ASSERT

#endif
