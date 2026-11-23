/*
 * motor_fault.h - Per-axis fault cause latch.
 * Author: BearKing
 *
 * Pure logic: no CAN, no FreeRTOS, no allocation. Every fault has one cause so
 * the log says why an axis stopped instead of only that it stopped.
 *
 * Rules:
 *  - A cause is counted every time it is seen; the latch keeps the FIRST cause
 *    that actually latched the axis.
 *  - A latched axis never resumes from ordinary traffic; only
 *    motor_fault_reset() clears it, and that does not enable the motor.
 */
#ifndef MOTOR_FAULT_H
#define MOTOR_FAULT_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MOTOR_FAULT_CAUSE_COUNT   8U

typedef enum {
    MOTOR_FAULT_NONE = 0U,
    MOTOR_FAULT_NO_FEEDBACK,      /* no Type 2 frame within the timeout */
    MOTOR_FAULT_ID_MISMATCH,      /* unknown motor id, wrong host id or wrong bus */
    MOTOR_FAULT_MOTOR_REPORTED,   /* Type 2 fault bits non-zero */
    MOTOR_FAULT_INVALID_DATA,     /* bad DLC, NaN/Inf or decode failure */
    MOTOR_FAULT_RX_OVERFLOW,      /* MCAN FIFO lost / software RX ring full */
    MOTOR_FAULT_BUS_OFF,          /* MCAN bus-off on this axis' bus */
    MOTOR_FAULT_MODE_SWITCH,      /* five-mode switch failed */
} motor_fault_cause_t;

typedef struct {
    bool latched;
    uint8_t cause;                                  /* first latched cause */
    uint32_t latched_at_ms;
    uint32_t reset_count;
    uint32_t count[MOTOR_FAULT_CAUSE_COUNT];        /* occurrences per cause */
} motor_fault_state_t;

static inline void motor_fault_init(motor_fault_state_t *state)
{
    memset(state, 0, sizeof(*state));
}

static inline bool motor_fault_is_valid_cause(uint8_t cause)
{
    return (cause > (uint8_t)MOTOR_FAULT_NONE)
        && (cause < (uint8_t)MOTOR_FAULT_CAUSE_COUNT);
}

/* Count an occurrence without latching (e.g. a rejected frame). */
static inline void motor_fault_note(motor_fault_state_t *state, uint8_t cause)
{
    if (motor_fault_is_valid_cause(cause)) {
        state->count[cause]++;
    }
}

/* Count and latch. Returns true only the first time the axis latches. */
static inline bool motor_fault_latch(motor_fault_state_t *state, uint8_t cause,
                                     uint32_t now_ms)
{
    if (!motor_fault_is_valid_cause(cause)) {
        return false;
    }

    state->count[cause]++;
    if (state->latched) {
        return false;
    }

    state->latched = true;
    state->cause = cause;
    state->latched_at_ms = now_ms;
    return true;
}

static inline bool motor_fault_is_latched(const motor_fault_state_t *state)
{
    return state->latched;
}

static inline uint8_t motor_fault_cause(const motor_fault_state_t *state)
{
    return state->latched ? state->cause : (uint8_t)MOTOR_FAULT_NONE;
}

/* Motion frames, and therefore enable, are blocked while latched. */
static inline bool motor_fault_motion_allowed(const motor_fault_state_t *state)
{
    return !state->latched;
}

/* Explicit reset only: clears the latch, never re-enables the motor. */
static inline bool motor_fault_reset(motor_fault_state_t *state)
{
    bool had_fault = state->latched;

    state->latched = false;
    state->cause = 0U;
    state->latched_at_ms = 0U;
    if (had_fault) {
        state->reset_count++;
    }
    return had_fault;
}

#endif
