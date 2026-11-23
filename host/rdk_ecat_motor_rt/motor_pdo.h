/*
 * motor_pdo.h - Native process-data layout for the 16-axis EtherCAT gateway.
 * Author: BearKing
 *
 * RDK and HPM firmware exchange IEEE-754 binary32 values in little-endian
 * EtherCAT PDOs. These structures are native-layout views of that fixed,
 * same-architecture contract; the size assertions are part of the contract.
 */
#ifndef GATEWAY_RT_MOTOR_PDO_H
#define GATEWAY_RT_MOTOR_PDO_H

#include <stdint.h>

#define GATEWAY_RT_AXIS_COUNT               16U
#define GATEWAY_RT_OUTPUT_SIZE_BYTES        320U
#define GATEWAY_RT_IMU_INPUT_SIZE_BYTES      60U
#define GATEWAY_RT_MOTOR_INPUT_SIZE_BYTES   192U
#define GATEWAY_RT_INPUT_SIZE_BYTES          252U

/* IMU valid_fields bits required by the 252-byte gateway input PDO. */
#define GATEWAY_RT_IMU_REQUIRED_FIELDS \
    ((UINT32_C(1) << 0) | (UINT32_C(1) << 1) | (UINT32_C(1) << 2) | \
     (UINT32_C(1) << 6) | (UINT32_C(1) << 7))

#define GATEWAY_RT_VENDOR_ID                 0x0048504DU
#define GATEWAY_RT_PRODUCT_CODE              0x00000001U

typedef struct {
    float position_rad;
    float velocity_rad_s;
    float feedforward_torque_nm;
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
} gateway_rt_output_pdo;

typedef struct {
    gateway_rt_imu_feedback imu;
    /* axis[n] corresponds to global CAN motor ID n + 1. */
    gateway_rt_motor_feedback axis[GATEWAY_RT_AXIS_COUNT];
} gateway_rt_input_pdo;

_Static_assert(sizeof(gateway_rt_motor_command) == 20U, "unexpected motor command size");
_Static_assert(sizeof(gateway_rt_motor_feedback) == 12U, "unexpected motor feedback size");
_Static_assert(sizeof(gateway_rt_imu_feedback) == GATEWAY_RT_IMU_INPUT_SIZE_BYTES,
               "unexpected IMU PDO size");
_Static_assert(sizeof(gateway_rt_output_pdo) == GATEWAY_RT_OUTPUT_SIZE_BYTES,
               "unexpected output PDO size");
_Static_assert(sizeof(gateway_rt_input_pdo) == GATEWAY_RT_INPUT_SIZE_BYTES,
               "unexpected input PDO size");

#endif
