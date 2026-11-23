/*
 * gateway_app.h - SSC application callbacks for gateway
 * Author: BearKing
 */
#ifndef GATEWAY_APP_H
#define GATEWAY_APP_H

#include "ecat_def.h"

#define MIT_MOTOR_AXIS_COUNT        16U
#define MIT_MOTOR_COMMAND_FLOATS    5U
#define MIT_MOTOR_FEEDBACK_FLOATS   3U
#define MIT_MOTOR_COMMAND_BYTES \
    (MIT_MOTOR_AXIS_COUNT * MIT_MOTOR_COMMAND_FLOATS * sizeof(REAL32))
#define MIT_MOTOR_FEEDBACK_BYTES \
    (MIT_MOTOR_AXIS_COUNT * MIT_MOTOR_FEEDBACK_FLOATS * sizeof(REAL32))

#define GATEWAY_IMU_PDO_UINT32S     4U
#define GATEWAY_IMU_PDO_REAL32S     11U
#define GATEWAY_IMU_PDO_BYTES       ((GATEWAY_IMU_PDO_UINT32S * sizeof(UINT32)) \
                                     + (GATEWAY_IMU_PDO_REAL32S * sizeof(REAL32)))
#define GATEWAY_TXPDO_INPUT_BYTES   (GATEWAY_IMU_PDO_BYTES + MIT_MOTOR_FEEDBACK_BYTES)

#define GATEWAY_IMU_STATUS_SAMPLE_AVAILABLE  (1UL << 0)
#define GATEWAY_IMU_STATUS_CHECKSUM_ERROR     (1UL << 1)
#define GATEWAY_IMU_STATUS_FORMAT_ERROR       (1UL << 2)
#define GATEWAY_IMU_STATUS_DMA_OVERRUN        (1UL << 3)
#define GATEWAY_IMU_STATUS_DMA_ERROR          (1UL << 4)

typedef struct {
    REAL32 target_position;
    REAL32 target_velocity;
    REAL32 feedforward_torque;
    REAL32 kp;
    REAL32 kd;
} mit_motor_command_t;

/* Called by SSC stack when output data arrives (master -> slave) */
void APPL_OutputMapping(UINT16 *pData);

/* Called by SSC stack to prepare input data (slave -> master) */
void APPL_InputMapping(UINT16 *pData);

/* Called each cycle for application logic */
void APPL_Application(void);

/* Generate PDO mapping sizes */
UINT16 APPL_GenerateMapping(UINT16 *pInputSize, UINT16 *pOutputSize);

BOOL gateway_app_get_motor_command(UINT16 axis, mit_motor_command_t *command);
UINT32 gateway_app_get_motor_command_sequence(void);

/* Debug: copy the raw RxPDO command snapshot for axis 15 (motor 16). */
void gateway_app_get_debug_pdo_axis15(REAL32 out[MIT_MOTOR_COMMAND_FLOATS]);

/* Debug: raw RxPDO bytes 300..331 (axis 15 command + diagnostic pad). */
void gateway_app_get_debug_pdo_tail(uint8_t out[32]);

/* Debug: the RxPDO output size used by the ESC process-data path. */
UINT16 gateway_app_get_debug_pd_output_size(void);

/* Debug: read the ESC SM2 output length and FMMU0 output length registers. */
void gateway_app_get_debug_esc_regs(UINT16 *sm2_len, UINT16 *fmmu0_len);

void gateway_app_set_motor_feedback(UINT16 axis,
                                    REAL32 actual_position,
                                    REAL32 actual_velocity,
                                    REAL32 actual_torque);

/* Read latest motor feedback values (position, velocity, torque).
 * Returns TRUE on success, FALSE if axis out of range. */
BOOL gateway_app_get_motor_feedback(UINT16 axis,
                                   REAL32 *actual_position,
                                   REAL32 *actual_velocity,
                                   REAL32 *actual_torque);

#endif
