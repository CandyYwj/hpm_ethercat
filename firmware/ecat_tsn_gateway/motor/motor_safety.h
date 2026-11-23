/* Internal motor-manager feedback timeout state. */
#ifndef MOTOR_SAFETY_H
#define MOTOR_SAFETY_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    bool has_feedback;
    bool feedback_timeout_latched;
    bool stop_pending;
    uint32_t last_feedback_ms;
    uint32_t stop_attempt_count;
    uint32_t stop_failure_count;
} motor_feedback_guard_t;

static inline void motor_feedback_guard_init(motor_feedback_guard_t *guard)
{
    memset(guard, 0, sizeof(*guard));
}

static inline void motor_feedback_guard_note_feedback(motor_feedback_guard_t *guard,
                                                      uint32_t now_ms)
{
    guard->has_feedback = true;
    guard->last_feedback_ms = now_ms;
}

static inline bool motor_feedback_guard_check_timeout(motor_feedback_guard_t *guard,
                                                      uint32_t now_ms,
                                                      uint32_t timeout_ms)
{
    if (!guard->has_feedback || guard->feedback_timeout_latched
        || ((now_ms - guard->last_feedback_ms) <= timeout_ms)) {
        return false;
    }

    guard->feedback_timeout_latched = true;
    guard->stop_pending = true;
    return true;
}

static inline bool motor_feedback_guard_stop_pending(
    const motor_feedback_guard_t *guard)
{
    return guard->stop_pending;
}

static inline void motor_feedback_guard_record_stop_result(
    motor_feedback_guard_t *guard, bool sent)
{
    if (!guard->stop_pending) {
        return;
    }

    guard->stop_attempt_count++;
    if (sent) {
        guard->stop_pending = false;
    } else {
        guard->stop_failure_count++;
    }
}

static inline bool motor_feedback_guard_motion_allowed(
    const motor_feedback_guard_t *guard)
{
    return !guard->feedback_timeout_latched;
}

#endif
