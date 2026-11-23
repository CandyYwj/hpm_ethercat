/*
 * gateway_app.c - SSC application callbacks for 16-axis MIT motor PDOs
 * Author: BearKing
 *
 * RxPDO / Outputs: 16 axes x 5 REAL32 = 320 bytes.
 * TxPDO / Inputs : 60-byte IMU prefix + 16 axes x 3 REAL32 = 252 bytes.
 */

#define _DIGITAL_IO_ 1

#include "ecat_def.h"
#include "applInterface.h"

#include "digital_io.h"
#undef _DIGITAL_IO_

#include "ecat_config.h"
#include "gateway_app.h"
#include "hpm_interrupt.h"
#include "imu.h"
#include <string.h>

typedef char real32_must_be_4_bytes[(sizeof(REAL32) == 4U) ? 1 : -1];
typedef char uint32_must_be_4_bytes[(sizeof(UINT32) == 4U) ? 1 : -1];
typedef char imu_pdo_must_be_60_bytes[(GATEWAY_IMU_PDO_BYTES == 60U) ? 1 : -1];
typedef char txpdo_must_fit_sm3[(GATEWAY_TXPDO_INPUT_BYTES <= MAX_PD_INPUT_SIZE) ? 1 : -1];

static mit_motor_command_t s_motor_commands[MIT_MOTOR_AXIS_COUNT];
static volatile UINT32 s_motor_command_sequence;

/* Debug: snapshot of the raw RxPDO group for axis 15 (motor 16), updated in
 * APPL_OutputMapping (ISR context) and printed from the status task. */
static volatile REAL32 s_debug_pdo_axis15[MIT_MOTOR_COMMAND_FLOATS];

/* Debug: raw bytes 300..331 of the RxPDO (axis 15 command group 300..319 plus
 * the 12-byte diagnostic pad 320..331). Lets us find the exact ESC
 * process-data delivery boundary when the host sends a distinct pad pattern. */
#define DEBUG_PDO_TAIL_OFFSET 300U
#define DEBUG_PDO_TAIL_BYTES  32U
static volatile uint8_t s_debug_pdo_tail[DEBUG_PDO_TAIL_BYTES];

static REAL32 *const s_command_objects[MIT_MOTOR_AXIS_COUNT][MIT_MOTOR_COMMAND_FLOATS] = {
    {&TargetPosition00x7010, &TargetVelocity00x7011, &FFTorque00x7012, &Kp00x7013, &Kd00x7014},
    {&TargetPosition10x7110, &TargetVelocity10x7111, &FFTorque10x7112, &Kp10x7113, &Kd10x7114},
    {&TargetPosition20x7210, &TargetVelocity20x7211, &FFTorque20x7212, &Kp20x7213, &Kd20x7214},
    {&TargetPosition30x7310, &TargetVelocity30x7311, &FFTorque30x7312, &Kp30x7313, &Kd30x7314},
    {&TargetPosition40x7410, &TargetVelocity40x7411, &FFTorque40x7412, &Kp40x7413, &Kd40x7414},
    {&TargetPosition50x7510, &TargetVelocity50x7511, &FFTorque50x7512, &Kp50x7513, &Kd50x7514},
    {&TargetPosition60x7610, &TargetVelocity60x7611, &FFTorque60x7612, &Kp60x7613, &Kd60x7614},
    {&TargetPosition70x7710, &TargetVelocity70x7711, &FFTorque70x7712, &Kp70x7713, &Kd70x7714},
    {&TargetPosition80x7810, &TargetVelocity80x7811, &FFTorque80x7812, &Kp80x7813, &Kd80x7814},
    {&TargetPosition90x7910, &TargetVelocity90x7911, &FFTorque90x7912, &Kp90x7913, &Kd90x7914},
    {&TargetPositionA0x7A10, &TargetVelocityA0x7A11, &FFTorqueA0x7A12, &KpA0x7A13, &KdA0x7A14},
    {&TargetPositionB0x7B10, &TargetVelocityB0x7B11, &FFTorqueB0x7B12, &KpB0x7B13, &KdB0x7B14},
    {&TargetPositionC0x7C10, &TargetVelocityC0x7C11, &FFTorqueC0x7C12, &KpC0x7C13, &KdC0x7C14},
    {&TargetPositionD0x7D10, &TargetVelocityD0x7D11, &FFTorqueD0x7D12, &KpD0x7D13, &KdD0x7D14},
    {&TargetPositionE0x7E10, &TargetVelocityE0x7E11, &FFTorqueE0x7E12, &KpE0x7E13, &KdE0x7E14},
    {&TargetPositionF0x7F10, &TargetVelocityF0x7F11, &FFTorqueF0x7F12, &KpF0x7F13, &KdF0x7F14},
};

static REAL32 *const s_feedback_objects[MIT_MOTOR_AXIS_COUNT][MIT_MOTOR_FEEDBACK_FLOATS] = {
    {&ActualPosition00x6000, &ActualVelocity00x6001, &ActualTorque00x6002},
    {&ActualPosition10x6100, &ActualVelocity10x6101, &ActualTorque10x6102},
    {&ActualPosition20x6200, &ActualVelocity20x6201, &ActualTorque20x6202},
    {&ActualPosition30x6300, &ActualVelocity30x6301, &ActualTorque30x6302},
    {&ActualPosition40x6400, &ActualVelocity40x6401, &ActualTorque40x6402},
    {&ActualPosition50x6500, &ActualVelocity50x6501, &ActualTorque50x6502},
    {&ActualPosition60x6600, &ActualVelocity60x6601, &ActualTorque60x6602},
    {&ActualPosition70x6700, &ActualVelocity70x6701, &ActualTorque70x6702},
    {&ActualPosition80x6800, &ActualVelocity80x6801, &ActualTorque80x6802},
    {&ActualPosition90x6900, &ActualVelocity90x6901, &ActualTorque90x6902},
    {&ActualPositionA0x6A00, &ActualVelocityA0x6A01, &ActualTorqueA0x6A02},
    {&ActualPositionB0x6B00, &ActualVelocityB0x6B01, &ActualTorqueB0x6B02},
    {&ActualPositionC0x6C00, &ActualVelocityC0x6C01, &ActualTorqueC0x6C02},
    {&ActualPositionD0x6D00, &ActualVelocityD0x6D01, &ActualTorqueD0x6D02},
    {&ActualPositionE0x6E00, &ActualVelocityE0x6E01, &ActualTorqueE0x6E02},
    {&ActualPositionF0x6F00, &ActualVelocityF0x6F01, &ActualTorqueF0x6F02},
};

static UINT32 *const s_imu_uint32_objects[GATEWAY_IMU_PDO_UINT32S] = {
    &ImuStatus0x3000,
    &ImuValidFields0x3001,
    &ImuReceiveTick0x3002,
    &ImuSampleTimeUs0x3003,
};

static REAL32 *const s_imu_real32_objects[GATEWAY_IMU_PDO_REAL32S] = {
    &ImuAccelX0x3004,
    &ImuAccelY0x3005,
    &ImuAccelZ0x3006,
    &ImuGyroX0x3007,
    &ImuGyroY0x3008,
    &ImuGyroZ0x3009,
    &ImuQuaternion00x3010,
    &ImuQuaternion10x3011,
    &ImuQuaternion20x3012,
    &ImuQuaternion30x3013,
    &ImuTemperature0x3014,
};

static void sync_imu_objects(void)
{
    imu_sample_t sample = {0};
    imu_stats_t stats;
    UINT32 status = 0U;
    BOOL has_sample = imu_get_latest(&sample) ? TRUE : FALSE;

    imu_get_stats(&stats);
    if (has_sample == TRUE) {
        status |= GATEWAY_IMU_STATUS_SAMPLE_AVAILABLE;
    }
    if (stats.checksum_errors != 0U) {
        status |= GATEWAY_IMU_STATUS_CHECKSUM_ERROR;
    }
    if (stats.format_errors != 0U) {
        status |= GATEWAY_IMU_STATUS_FORMAT_ERROR;
    }
    if (stats.dma_overruns != 0U) {
        status |= GATEWAY_IMU_STATUS_DMA_OVERRUN;
    }
    if (stats.dma_errors != 0U) {
        status |= GATEWAY_IMU_STATUS_DMA_ERROR;
    }

    ImuStatus0x3000 = status;
    ImuValidFields0x3001 = (has_sample == TRUE) ? sample.valid_fields : 0U;
    ImuReceiveTick0x3002 = (has_sample == TRUE) ? sample.received_tick : 0U;
    ImuSampleTimeUs0x3003 = ((sample.valid_fields & IMU_FIELD_SAMPLE_TIMESTAMP) != 0U)
        ? sample.sample_timestamp_us : 0U;

    ImuAccelX0x3004 = sample.acceleration_mps2[0];
    ImuAccelY0x3005 = sample.acceleration_mps2[1];
    ImuAccelZ0x3006 = sample.acceleration_mps2[2];
    ImuGyroX0x3007 = sample.angular_rate_dps[0];
    ImuGyroY0x3008 = sample.angular_rate_dps[1];
    ImuGyroZ0x3009 = sample.angular_rate_dps[2];
    ImuQuaternion00x3010 = sample.quaternion[0];
    ImuQuaternion10x3011 = sample.quaternion[1];
    ImuQuaternion20x3012 = sample.quaternion[2];
    ImuQuaternion30x3013 = sample.quaternion[3];
    ImuTemperature0x3014 = sample.temperature_c;
}

static void sync_command_cache(UINT16 axis)
{
    s_motor_commands[axis].target_position = *s_command_objects[axis][0];
    s_motor_commands[axis].target_velocity = *s_command_objects[axis][1];
    s_motor_commands[axis].feedforward_torque = *s_command_objects[axis][2];
    s_motor_commands[axis].kp = *s_command_objects[axis][3];
    s_motor_commands[axis].kd = *s_command_objects[axis][4];
}

/* ---- State transition callbacks ---- */

void APPL_AckErrorInd(UINT16 stateTrans)
{
    (void)stateTrans;
}

UINT16 APPL_StartMailboxHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StopMailboxHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StartInputHandler(UINT16 *pIntMask)
{
    (void)pIntMask;
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StopInputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StartOutputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StopOutputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

#if EXPLICIT_DEVICE_ID
UINT16 APPL_GetDeviceID(void)
{
    return 0x5;
}
#endif

/* ---- PDO mapping ---- */
UINT16 APPL_GenerateMapping(UINT16 *pInputSize, UINT16 *pOutputSize)
{
    UINT16 result = ALSTATUSCODE_NOERROR;
    UINT16 InputSize = 0;
    UINT16 OutputSize = 0;

#if COE_SUPPORTED
    UINT16 PDOAssignEntryCnt = 0;
    OBJCONST TOBJECT OBJMEM *pPDO = NULL;
    UINT16 PDOSubindex0 = 0;
    UINT32 *pPDOEntry = NULL;
    UINT16 PDOEntryCnt = 0;

#if MAX_PD_OUTPUT_SIZE > 0
    for (PDOAssignEntryCnt = 0;
         PDOAssignEntryCnt < sRxPDOassign.u16SubIndex0;
         PDOAssignEntryCnt++) {
        pPDO = OBJ_GetObjectHandle(sRxPDOassign.aEntries[PDOAssignEntryCnt]);
        if (pPDO != NULL) {
            PDOSubindex0 = *((UINT16 *)pPDO->pVarPtr);
            for (PDOEntryCnt = 0; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++) {
                pPDOEntry = (UINT32 *)(((UINT16 *)pPDO->pVarPtr)
                    + (OBJ_GetEntryOffset((PDOEntryCnt + 1), pPDO) >> 4));
                OutputSize += (UINT16)((*pPDOEntry) & 0xFF);
            }
        } else {
            OutputSize = 0;
            result = ALSTATUSCODE_INVALIDOUTPUTMAPPING;
            break;
        }
    }
    OutputSize = (OutputSize + 7) >> 3;
#endif

#if MAX_PD_INPUT_SIZE > 0
    if (result == 0) {
        for (PDOAssignEntryCnt = 0;
             PDOAssignEntryCnt < sTxPDOassign.u16SubIndex0;
             PDOAssignEntryCnt++) {
            pPDO = OBJ_GetObjectHandle(sTxPDOassign.aEntries[PDOAssignEntryCnt]);
            if (pPDO != NULL) {
                PDOSubindex0 = *((UINT16 *)pPDO->pVarPtr);
                for (PDOEntryCnt = 0; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++) {
                    pPDOEntry = (UINT32 *)(((UINT16 *)pPDO->pVarPtr)
                        + (OBJ_GetEntryOffset((PDOEntryCnt + 1), pPDO) >> 4));
                    InputSize += (UINT16)((*pPDOEntry) & 0xFF);
                }
            } else {
                InputSize = 0;
                result = ALSTATUSCODE_INVALIDINPUTMAPPING;
                break;
            }
        }
    }
    InputSize = (InputSize + 7) >> 3;
#endif

#endif /* COE_SUPPORTED */

    *pInputSize  = InputSize;
    *pOutputSize = OutputSize;
    return result;
}

/* ---- PDO data transfer ---- */

void APPL_InputMapping(UINT16 *pData)
{
    uint8_t *dst = (uint8_t *)pData;
    UINT16 axis;
    UINT16 item;

    sync_imu_objects();
    for (item = 0; item < GATEWAY_IMU_PDO_UINT32S; item++) {
        memcpy(dst, s_imu_uint32_objects[item], sizeof(UINT32));
        dst += sizeof(UINT32);
    }
    for (item = 0; item < GATEWAY_IMU_PDO_REAL32S; item++) {
        memcpy(dst, s_imu_real32_objects[item], sizeof(REAL32));
        dst += sizeof(REAL32);
    }

    for (axis = 0; axis < MIT_MOTOR_AXIS_COUNT; axis++) {
        for (item = 0; item < MIT_MOTOR_FEEDBACK_FLOATS; item++) {
            memcpy(dst, s_feedback_objects[axis][item], sizeof(REAL32));
            dst += sizeof(REAL32);
        }
    }
}

void APPL_OutputMapping(UINT16 *pData)
{
    const uint8_t *src = (const uint8_t *)pData;
    UINT16 axis;
    UINT16 item;

    for (axis = 0; axis < MIT_MOTOR_AXIS_COUNT; axis++) {
        for (item = 0; item < MIT_MOTOR_COMMAND_FLOATS; item++) {
            memcpy(s_command_objects[axis][item], src, sizeof(REAL32));
            src += sizeof(REAL32);
        }
        sync_command_cache(axis);
    }

    /* Debug: snapshot raw RxPDO group for axis 15 (motor 16) to verify the
     * master command data actually arrives at the ESC output buffer. */
    {
        const uint8_t *p15 = src - (MIT_MOTOR_COMMAND_FLOATS * sizeof(REAL32));
        REAL32 tmp[MIT_MOTOR_COMMAND_FLOATS];

        memcpy(tmp, p15, sizeof(tmp));
        for (item = 0; item < MIT_MOTOR_COMMAND_FLOATS; item++) {
            s_debug_pdo_axis15[item] = tmp[item];
        }
    }

    /* Debug: raw tail bytes 300..331 (axis 15 command + 12-byte pad) to find
     * where the ESC process-data delivery actually stops. Only valid once the
     * SSC-generated RxPDO is padded to >= 332 bytes; guarded to avoid reading
     * past the 320-byte output buffer before the SSC tool regenerates it. */
    if (nPdOutputSize >= (DEBUG_PDO_TAIL_OFFSET + DEBUG_PDO_TAIL_BYTES)) {
        const uint8_t *tail = (const uint8_t *)pData + DEBUG_PDO_TAIL_OFFSET;
        UINT16 i;

        for (i = 0U; i < DEBUG_PDO_TAIL_BYTES; i++) {
            s_debug_pdo_tail[i] = tail[i];
        }
    }

    s_motor_command_sequence++;
}

void APPL_Application(void)
{
}

BOOL gateway_app_get_motor_command(UINT16 axis, mit_motor_command_t *command)
{
    uint32_t irq_state;

    if ((axis >= MIT_MOTOR_AXIS_COUNT) || (command == NULL)) {
        return FALSE;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    *command = s_motor_commands[axis];
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
    return TRUE;
}

UINT32 gateway_app_get_motor_command_sequence(void)
{
    return s_motor_command_sequence;
}

void gateway_app_get_debug_pdo_axis15(REAL32 out[MIT_MOTOR_COMMAND_FLOATS])
{
    uint32_t irq_state;
    UINT16 item;

    if (out == NULL) {
        return;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    for (item = 0; item < MIT_MOTOR_COMMAND_FLOATS; item++) {
        out[item] = s_debug_pdo_axis15[item];
    }
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
}

/* Debug: raw RxPDO bytes 300..331 (axis 15 command + diagnostic pad). */
void gateway_app_get_debug_pdo_tail(uint8_t out[DEBUG_PDO_TAIL_BYTES])
{
    uint32_t irq_state;
    UINT16 i;

    if (out == NULL) {
        return;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    for (i = 0U; i < DEBUG_PDO_TAIL_BYTES; i++) {
        out[i] = s_debug_pdo_tail[i];
    }
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
}

/* Debug: the RxPDO output size the ESC process-data path actually uses. */
UINT16 gateway_app_get_debug_pd_output_size(void)
{
    return nPdOutputSize;
}

/* Debug: read the ESC registers that drive the RxPDO data path.
 * sm2_len    = SM2 output length register (0x812)
 * fmmu0_len  = FMMU0 output length register (0x604, LogLength) */
void gateway_app_get_debug_esc_regs(UINT16 *sm2_len, UINT16 *fmmu0_len)
{
    UINT16 value = 0U;

    if (sm2_len != NULL) {
        HW_EscRead((MEM_ADDR *)&value, 0x0812U, 2U);
        *sm2_len = value;
    }
    if (fmmu0_len != NULL) {
        HW_EscRead((MEM_ADDR *)&value, 0x0604U, 2U);
        *fmmu0_len = value;
    }
}

void gateway_app_set_motor_feedback(UINT16 axis,
                                    REAL32 actual_position,
                                    REAL32 actual_velocity,
                                    REAL32 actual_torque)
{
    uint32_t irq_state;

    if (axis >= MIT_MOTOR_AXIS_COUNT) {
        return;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    *s_feedback_objects[axis][0] = actual_position;
    *s_feedback_objects[axis][1] = actual_velocity;
    *s_feedback_objects[axis][2] = actual_torque;
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
}

BOOL gateway_app_get_motor_feedback(UINT16 axis,
                                   REAL32 *actual_position,
                                   REAL32 *actual_velocity,
                                   REAL32 *actual_torque)
{
    uint32_t irq_state;

    if ((axis >= MIT_MOTOR_AXIS_COUNT)
        || (actual_position == NULL)
        || (actual_velocity == NULL)
        || (actual_torque == NULL)) {
        return FALSE;
    }

    irq_state = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    *actual_position = *s_feedback_objects[axis][0];
    *actual_velocity = *s_feedback_objects[axis][1];
    *actual_torque = *s_feedback_objects[axis][2];
    restore_global_irq(irq_state & CSR_MSTATUS_MIE_MASK);
    return TRUE;
}
