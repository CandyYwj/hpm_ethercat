/*
 * motor_manager.c - Decoupled motor CAN scheduler.
 * Author: BearKing
 */

#include "motor_manager.h"
#include "can_if.h"
#include "gateway_app.h"
#include "hpm_clock_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_soc.h"
#include "motor_config.h"
#include "motor_fault.h"
#include "motor_safety.h"
#include "robstride_proto.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MOTOR_MANAGER_RX_BUDGET_PER_POLL      32U
#define MOTOR_MANAGER_POLL_DEADLINE_US         1000U
/* A timed-out axis stays stopped until motor_manager_init() explicitly resets it. */
#define MOTOR_MANAGER_FEEDBACK_TIMEOUT_MS     250U
#define MOTOR_MANAGER_BUS_OFF_RETRY_MS         100U
/* A dead motor would otherwise swallow one unacknowledged stop frame per
 * control period and push the whole CAN controller into bus-off. */
#define MOTOR_MANAGER_STOP_MAX_ATTEMPTS            5U
/* An axis without feedback is probed slowly for a bounded window: RobStride
 * motors answer only after they receive a frame, so silence is not a proof of
 * absence, but transmitting forever would bus-off an empty bus.
 */
#define MOTOR_MANAGER_PROBE_WINDOW_MS              3000U
#define MOTOR_MANAGER_PROBE_INTERVAL_MS             500U

typedef enum {
    MOTOR_PROBE_WAITING = 0,   /* Type 17 reads are being sent */
    MOTOR_PROBE_PRESENT = 1,   /* matching 0x7005 reply received */
    MOTOR_PROBE_FAILED = 2,    /* no reply inside the window, axis latched */
} motor_probe_state_t;

typedef struct {
    bool online;
    bool enable_sent;
    bool run_mode_sent;   /* Type 18 write of run_mode=operation done */
    uint32_t run_mode_ms;
    uint32_t enable_ms;
    bool command_stop_pending;
    bool stop_pending;
    bool has_feedback_once;   /* only talk to motors that have answered before */
    uint8_t command_stop_attempts;
    uint8_t probe_state;
    uint8_t probe_attempts;
    uint32_t probe_deadline_ms;
    uint32_t probe_next_ms;
    uint8_t fault_bits;
    float temperature_c;
    motor_fault_state_t fault;
    uint32_t feedback_gap_max_ms;
    uint32_t stale_feedback_count;
    uint32_t last_frame_id;
    uint32_t last_frame_ms;
    uint8_t last_frame_data[CAN_IF_CLASSIC_MAX_DLC];
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t feedback_count;
    motor_feedback_guard_t feedback_guard;
} motor_axis_state;

typedef struct {
    uint32_t mchtmr_freq_hz;
    uint32_t poll_max_us;
    uint32_t poll_overrun_count;
    uint32_t can_send_max_us;
    uint32_t can_send_fail_count;
    uint32_t can_send_attempt_count;
} motor_perf_stats;

typedef struct {
    TickType_t previous_tick;
    uint32_t previous_rx_total;
    uint32_t previous_tx_total;
    uint32_t previous_feedback_total;
    uint32_t previous_fault_report_count;
} motor_status_state;

typedef struct {
    robstride_fault_report_t last_fault_report;
    uint32_t fault_report_count;
    uint32_t timeout_stop_count;
    uint32_t feedback_timeout_count;
    uint32_t motor_fault_count;
    uint32_t id_mismatch_count;
    uint32_t invalid_frame_count;
    uint32_t rx_overflow_count;
    uint32_t bus_off_count;
    uint32_t stale_frame_count;
    uint32_t pdo_restart_count;
    bool has_fault_report;
} motor_event_state;

typedef struct {
    motor_axis_state axes[MOTOR_MANAGER_AXIS_COUNT];
    motor_perf_stats performance;
    motor_status_state status;
    motor_event_state events;
    uint32_t start_time_ms;
    uint32_t last_control_ms;
    uint32_t last_command_ms;
    uint32_t last_command_sequence;
    bool has_received_command;
    bool has_sent_timeout_stop;
    bool command_timeout_stop_active;
    uint32_t bus_off_seen_mask;
    uint32_t last_bus_service_ms;
    uint32_t previous_drop_count[CAN_IF_BUS_COUNT];
} motor_runtime_state;

static motor_runtime_state s_motor;


#if MOTOR_MANAGER_ENABLE_FAULT_INJECTION
/* Bench fault injection for Task 10: bit0 = CAN TX rejects every frame,
 * bit1 = discard all received frames (feedback loss). */
#define MOTOR_INJECT_TX_FAIL        (1UL << 0)
#define MOTOR_INJECT_FEEDBACK_LOSS  (1UL << 1)
static uint32_t s_inject_mask;
#endif

static uint32_t current_time_ms(void)
{
    return (uint32_t)(((uint64_t)xTaskGetTickCount() * 1000ULL) / configTICK_RATE_HZ);
}

static uint64_t read_perf_counter_ticks(void)
{
    return mchtmr_get_count(HPM_MCHTMR);
}

static uint32_t perf_ticks_to_microseconds(uint64_t ticks)
{
    if (s_motor.performance.mchtmr_freq_hz == 0U) {
        return 0U;
    }

    return (uint32_t)((ticks * 1000000ULL) / s_motor.performance.mchtmr_freq_hz);
}

static void update_maximum_u32(uint32_t *maximum, uint32_t value)
{
    if (value > *maximum) {
        *maximum = value;
    }
}

static bool validate_axis_configuration(void)
{
    uint32_t initialized_bus_mask = can_if_get_initialized_mask();
    uint8_t axis;

    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);
        uint8_t previous_axis;

        if (axis_config == NULL) {
            return false;
        }
        if (axis_config->motor_id != (axis + 1U)) {
            return false;
        }
        if (!axis_config->enabled) {
            continue;
        }
        if ((axis_config->bus >= CAN_IF_BUS_COUNT)
            || ((initialized_bus_mask & (1UL << axis_config->bus)) == 0U)
            || (axis_config->motor_id == 0U)
            || (robstride_get_motor_limits(axis_config->model) == NULL)) {
            return false;
        }

        for (previous_axis = 0U; previous_axis < axis; previous_axis++) {
            const motor_axis_config_t *previous_config = motor_config_get_axis(previous_axis);

            if ((previous_config != NULL)
                && previous_config->enabled
                && (previous_config->bus == axis_config->bus)
                && (previous_config->motor_id == axis_config->motor_id)) {
                return false;
            }
        }
    }

    return true;
}

/* Motion and mode frames are only allowed after the probe confirmed the motor
 * at (bus, motor_id, host_id). A missing motor never receives an enable frame.
 */
static bool axis_may_transmit(uint8_t axis_index, uint32_t now_ms)
{
    (void)now_ms;
    return s_motor.axes[axis_index].probe_state == MOTOR_PROBE_PRESENT;
}

static bool send_frame_for_axis(uint8_t axis_index, const can_if_frame_t *frame)
{
    uint64_t start_ticks = read_perf_counter_ticks();
    bool sent = can_if_send(frame);
    uint32_t elapsed_us = perf_ticks_to_microseconds(read_perf_counter_ticks() - start_ticks);

    s_motor.performance.can_send_attempt_count++;
    update_maximum_u32(&s_motor.performance.can_send_max_us, elapsed_us);

#if MOTOR_MANAGER_ENABLE_FAULT_INJECTION
    if ((s_inject_mask & MOTOR_INJECT_TX_FAIL) != 0U) {
        sent = false;
    }
#endif
    if (sent) {
        s_motor.axes[axis_index].tx_count++;
        return true;
    }

    s_motor.performance.can_send_fail_count++;
    return false;
}

/* Non-motion presence probe: Type 17 read of run_mode (0x7005) until the motor
 * answers or the window expires. Only a matching reply lets the axis enter the
 * enable sequence; a silent axis is latched and never auto-enabled.
 */
static void service_probe(uint32_t now_ms)
{
    uint8_t axis;

    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);
        motor_axis_state *state = &s_motor.axes[axis];
        can_if_frame_t frame;

        if ((axis_config == NULL) || !axis_config->enabled
            || (state->probe_state != MOTOR_PROBE_WAITING)) {
            continue;
        }

        if ((int32_t)(now_ms - state->probe_deadline_ms) >= 0) {
            state->probe_state = MOTOR_PROBE_FAILED;
            state->enable_sent = false;
            if (motor_fault_latch(&state->fault, MOTOR_FAULT_NO_FEEDBACK, now_ms)) {
                s_motor.events.feedback_timeout_count++;
            }
            continue;
        }

        if ((now_ms - state->probe_next_ms) < MOTOR_MANAGER_PROBE_INTERVAL_MS) {
            continue;
        }
        state->probe_next_ms = now_ms;

        if (robstride_private_encode_read_param(axis_config->bus,
                                                axis_config->motor_id,
                                                axis_config->host_id,
                                                ROBSTRIDE_PARAM_INDEX_RUN_MODE,
                                                &frame)) {
            (void)send_frame_for_axis(axis, &frame);
            state->probe_attempts++;
        }
    }
}

static void update_feedback(uint8_t axis_index,
                            const motor_axis_config_t *axis_config,
                            uint8_t fault_bits,
                            float position_rad,
                            float velocity_rad_s,
                            float torque_nm,
                            float temperature_c)
{
    if (axis_config->reverse) {
        position_rad = -position_rad;
        velocity_rad_s = -velocity_rad_s;
        torque_nm = -torque_nm;
    }

    gateway_app_set_motor_feedback((UINT16)axis_index,
                                   (REAL32)position_rad,
                                   (REAL32)velocity_rad_s,
                                   (REAL32)torque_nm);

    {
        uint32_t now_ms = current_time_ms();
        motor_axis_state *state = &s_motor.axes[axis_index];

        if (state->feedback_guard.has_feedback) {
            uint32_t gap_ms = now_ms - state->feedback_guard.last_feedback_ms;
            update_maximum_u32(&state->feedback_gap_max_ms, gap_ms);
        }
        motor_feedback_guard_note_feedback(&state->feedback_guard, now_ms);
        state->has_feedback_once = true;
    }
    s_motor.axes[axis_index].online =
        motor_feedback_guard_motion_allowed(&s_motor.axes[axis_index].feedback_guard);
    s_motor.axes[axis_index].fault_bits = fault_bits;
    s_motor.axes[axis_index].temperature_c = temperature_c;
    s_motor.axes[axis_index].rx_count++;
    s_motor.axes[axis_index].feedback_count++;
}

static void handle_robstride_feedback(const robstride_feedback_t *feedback)
{
    int axis_index = motor_config_find_axis_index(feedback->bus,
                                                  feedback->motor_id);
    const motor_axis_config_t *axis_config;

    if (axis_index < 0) {
        return;
    }

    axis_config = motor_config_get_axis((uint8_t)axis_index);
    if ((axis_config == NULL) || (feedback->host_id != axis_config->host_id)) {
        return;
    }

    if (!isfinite(feedback->position_rad)
        || !isfinite(feedback->velocity_rad_s)
        || !isfinite(feedback->torque_nm)
        || !isfinite(feedback->temperature_c)) {
        if (motor_fault_latch(&s_motor.axes[axis_index].fault,
                              MOTOR_FAULT_INVALID_DATA, current_time_ms())) {
            s_motor.events.invalid_frame_count++;
        }
        s_motor.axes[axis_index].stop_pending = true;
        s_motor.axes[axis_index].enable_sent = false;
        return;
    }

    update_feedback((uint8_t)axis_index,
                    axis_config,
                    feedback->fault_bits,
                    feedback->position_rad,
                    feedback->velocity_rad_s,
                    feedback->torque_nm,
                    feedback->temperature_c);

    if (feedback->fault_bits != 0U) {
        if (motor_fault_latch(&s_motor.axes[axis_index].fault,
                              MOTOR_FAULT_MOTOR_REPORTED, current_time_ms())) {
            s_motor.events.motor_fault_count++;
        }
        s_motor.axes[axis_index].stop_pending = true;
        s_motor.axes[axis_index].enable_sent = false;
    }
}

/* A feedback frame identical to the last one inside the same millisecond is
 * a duplicate delivery, not fresh data. */
static bool is_duplicate_feedback(uint8_t axis_index,
                                  const can_if_frame_t *frame)
{
    const motor_axis_state *state = &s_motor.axes[axis_index];

    return (state->last_frame_id == frame->id)
        && (state->last_frame_ms == current_time_ms())
        && (memcmp(state->last_frame_data, frame->data,
                   CAN_IF_CLASSIC_MAX_DLC) == 0);
}

static void remember_feedback(uint8_t axis_index, const can_if_frame_t *frame)
{
    motor_axis_state *state = &s_motor.axes[axis_index];

    state->last_frame_id = frame->id;
    state->last_frame_ms = current_time_ms();
    memcpy(state->last_frame_data, frame->data, CAN_IF_CLASSIC_MAX_DLC);
}

static void process_received_frames(void)
{
    uint8_t processed_frame_count;

#if MOTOR_MANAGER_ENABLE_FAULT_INJECTION
    if ((s_inject_mask & MOTOR_INJECT_FEEDBACK_LOSS) != 0U) {
        return;
    }
#endif

    for (processed_frame_count = 0U;
         processed_frame_count < MOTOR_MANAGER_RX_BUDGET_PER_POLL;
         processed_frame_count++) {
        can_if_frame_t received_frame;
        robstride_feedback_t robstride_feedback;
        robstride_fault_report_t fault_report;

        if (!can_if_receive(&received_frame)) {
            break;
        }

        {
            robstride_private_id_fields_t fields;

            if ((received_frame.id_format == CAN_IF_ID_EXTENDED)
                && robstride_private_parse_id(received_frame.id, &fields)
                && (fields.type == ROBSTRIDE_PRIVATE_TYPE_FEEDBACK)) {
                int axis_index = motor_config_find_axis_index(
                    received_frame.bus, (uint8_t)fields.data);
                const motor_axis_config_t *axis_config = (axis_index < 0)
                    ? NULL : motor_config_get_axis((uint8_t)axis_index);

                if (axis_config == NULL) {
                    /* Unknown motor id, or the right id on the wrong bus. */
                    s_motor.events.id_mismatch_count++;
                } else if (fields.target_id != axis_config->host_id) {
                    (void)motor_fault_latch(&s_motor.axes[axis_index].fault,
                                            MOTOR_FAULT_ID_MISMATCH,
                                            current_time_ms());
                    s_motor.axes[axis_index].stop_pending = true;
                    s_motor.axes[axis_index].enable_sent = false;
                } else if (!robstride_private_decode_feedback(&received_frame,
                                                             axis_config->model,
                                                             &robstride_feedback)) {
                    /* Bad DLC or a frame that cannot be decoded. */
                    s_motor.events.invalid_frame_count++;
                    (void)motor_fault_latch(&s_motor.axes[axis_index].fault,
                                            MOTOR_FAULT_INVALID_DATA,
                                            current_time_ms());
                    s_motor.axes[axis_index].stop_pending = true;
                    s_motor.axes[axis_index].enable_sent = false;
                } else if (is_duplicate_feedback((uint8_t)axis_index, &received_frame)) {
                    s_motor.axes[axis_index].stale_feedback_count++;
                    s_motor.events.stale_frame_count++;
                } else {
                    remember_feedback((uint8_t)axis_index, &received_frame);
                    handle_robstride_feedback(&robstride_feedback);
                }
                continue;
            }
        }

        {
            robstride_param_response_t param_response;

            if (robstride_private_decode_param_response(&received_frame,
                                                   &param_response)) {
                int axis_index = motor_config_find_axis_index(param_response.bus,
                                                              param_response.motor_id);
                const motor_axis_config_t *axis_config = (axis_index < 0)
                    ? NULL : motor_config_get_axis((uint8_t)axis_index);

                if ((axis_config != NULL)
                    && (param_response.host_id == axis_config->host_id)
                    && (param_response.index == ROBSTRIDE_PARAM_INDEX_RUN_MODE)) {
                    if ((param_response.status == 0U)
                        && (s_motor.axes[axis_index].probe_state
                            == MOTOR_PROBE_WAITING)) {
                        s_motor.axes[axis_index].probe_state = MOTOR_PROBE_PRESENT;
                        s_motor.axes[axis_index].probe_attempts = 0U;
                    }
                }
                continue;
            }
        }

        if (robstride_private_decode_fault_report(&received_frame, &fault_report)) {
            s_motor.events.last_fault_report = fault_report;
            s_motor.events.fault_report_count++;
            s_motor.events.has_fault_report = true;
            continue;
        }
    }
}

#if MOTOR_MANAGER_ENABLE_MOTION_CONTROL
static void snapshot_command(uint8_t axis, robstride_motion_cmd_t *command)
{
    mit_motor_command_t source_command;

    memset(command, 0, sizeof(*command));
    if (!gateway_app_get_motor_command(axis, &source_command)) {
        return;
    }

    command->position_rad = (float)source_command.target_position;
    command->velocity_rad_s = (float)source_command.target_velocity;
    command->torque_nm = (float)source_command.feedforward_torque;
    command->kp = (float)source_command.kp;
    command->kd = (float)source_command.kd;
}
#endif

static bool encode_enable_for_axis(const motor_axis_config_t *axis_config,
                                   can_if_frame_t *frame)
{
    return robstride_private_encode_enable(axis_config->bus,
                                      axis_config->motor_id,
                                      axis_config->host_id,
                                      frame);
}

static bool encode_stop_for_axis(const motor_axis_config_t *axis_config,
                                 can_if_frame_t *frame)
{
    return robstride_private_encode_stop(axis_config->bus,
                                    axis_config->motor_id,
                                    axis_config->host_id,
                                    false,
                                    frame);
}

static bool encode_motion_for_axis(const motor_axis_config_t *axis_config,
                                   const robstride_motion_cmd_t *command,
                                   can_if_frame_t *frame)
{
    return robstride_private_encode_motion_control(axis_config->bus,
                                              axis_config->motor_id,
                                              axis_config->model,
                                              command,
                                              frame);
}

/* Bus-level health. A bus-off latches every axis on that bus and the recovery
 * attempt is rate limited; RX overflow is counted so the cause stays visible.
 * Recovery never resumes motion: the axes stay latched until an explicit reset.
 */
static void service_can_bus_health(uint32_t now_ms)
{
    uint32_t initialized_mask = can_if_get_initialized_mask();
    uint8_t bus;

    for (bus = 0U; bus < CAN_IF_BUS_COUNT; bus++) {
        can_if_bus_state_t bus_state;
        uint32_t drops;
        uint8_t axis;

        if ((initialized_mask & (1UL << bus)) == 0U) {
            continue;
        }

        drops = can_if_get_drop_count(bus);
        if (drops != s_motor.previous_drop_count[bus]) {
            s_motor.previous_drop_count[bus] = drops;
            s_motor.events.rx_overflow_count++;
            for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
                const motor_axis_config_t *axis_config = motor_config_get_axis(axis);

                if ((axis_config != NULL) && axis_config->enabled
                    && (axis_config->bus == bus)) {
                    motor_fault_note(&s_motor.axes[axis].fault,
                                     MOTOR_FAULT_RX_OVERFLOW);
                }
            }
        }

        bus_state = can_if_get_bus_state(bus);
        if (bus_state == CAN_IF_BUS_OFF) {
            if ((s_motor.bus_off_seen_mask & (1UL << bus)) == 0U) {
                s_motor.bus_off_seen_mask |= (1UL << bus);
                s_motor.events.bus_off_count++;
                for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
                    const motor_axis_config_t *axis_config = motor_config_get_axis(axis);

                    if ((axis_config != NULL) && axis_config->enabled
                        && (axis_config->bus == bus)) {
                        (void)motor_fault_latch(&s_motor.axes[axis].fault,
                                                MOTOR_FAULT_BUS_OFF, now_ms);
                        s_motor.axes[axis].stop_pending = true;
                        s_motor.axes[axis].enable_sent = false;
                    }
                }
            }
            if ((now_ms - s_motor.last_bus_service_ms)
                >= MOTOR_MANAGER_BUS_OFF_RETRY_MS) {
                s_motor.last_bus_service_ms = now_ms;
                (void)can_if_service_bus_off(bus);
            }
        } else if (bus_state == CAN_IF_BUS_OK) {
            /* Recovered: clear the edge flag, keep the axes latched. */
            s_motor.bus_off_seen_mask &= ~(1UL << bus);
        }
    }
}

static void service_feedback_safety(uint32_t now_ms)
{
    uint8_t axis;

    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);
        can_if_frame_t stop_frame;

        if ((axis_config == NULL) || !axis_config->enabled) {
            continue;
        }

        if (motor_feedback_guard_check_timeout(
                &s_motor.axes[axis].feedback_guard,
                now_ms,
                MOTOR_MANAGER_FEEDBACK_TIMEOUT_MS)) {
            s_motor.axes[axis].online = false;
            s_motor.axes[axis].enable_sent = false;
            s_motor.axes[axis].command_stop_pending = false;
            s_motor.axes[axis].stop_pending = true;
            (void)motor_fault_latch(&s_motor.axes[axis].fault,
                                    MOTOR_FAULT_NO_FEEDBACK, now_ms);
            s_motor.events.feedback_timeout_count++;
        }

        /* Enable was sent but no Type 2 ever answered: do not keep sending
         * motion frames into a silent motor. */
        if (s_motor.axes[axis].enable_sent
            && !s_motor.axes[axis].feedback_guard.has_feedback
            && ((now_ms - s_motor.axes[axis].enable_ms)
                > MOTOR_MANAGER_FEEDBACK_TIMEOUT_MS)) {
            s_motor.axes[axis].probe_state = MOTOR_PROBE_FAILED;
            s_motor.axes[axis].enable_sent = false;
            s_motor.axes[axis].stop_pending = true;
            if (motor_fault_latch(&s_motor.axes[axis].fault,
                                  MOTOR_FAULT_NO_FEEDBACK, now_ms)) {
                s_motor.events.feedback_timeout_count++;
            }
        }

        if (motor_feedback_guard_stop_pending(
                &s_motor.axes[axis].feedback_guard)) {
            bool sent = encode_stop_for_axis(axis_config, &stop_frame)
                     && send_frame_for_axis(axis, &stop_frame);

            motor_feedback_guard_record_stop_result(
                &s_motor.axes[axis].feedback_guard, sent);
        }
    }
}

static void send_motion_controls(void)
{
#if MOTOR_MANAGER_ENABLE_MOTION_CONTROL
    uint8_t axis;

    for (axis = 0; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);
        motor_axis_state *state = &s_motor.axes[axis];
        can_if_frame_t tx_frame;
        robstride_motion_cmd_t command;

        if ((axis_config == NULL) || !axis_config->enabled) {
            continue;
        }

        if (!axis_may_transmit(axis, current_time_ms())) {
            continue;
        }

        if (!motor_feedback_guard_motion_allowed(&state->feedback_guard)
            || (state->fault_bits != 0U)
            || !motor_fault_motion_allowed(&state->fault)) {
            continue;
        }

#if MOTOR_MANAGER_ENABLE_AUTO_ENABLE
        /* The motor ignores motion frames until its run mode is operation.
         * Set it once, then enable, then command. */
        if (!state->run_mode_sent) {
            can_if_frame_t run_mode_frame;

            if (robstride_private_encode_set_run_mode(axis_config->bus,
                                                      axis_config->motor_id,
                                                      axis_config->host_id,
                                                      ROBSTRIDE_RUN_MODE_OPERATION,
                                                      &run_mode_frame)
                && send_frame_for_axis(axis, &run_mode_frame)) {
                state->run_mode_sent = true;
                state->run_mode_ms = current_time_ms();
            }
            continue;
        }
        if (!state->enable_sent) {
            /* Give the motor a moment to switch into operation mode before the
             * enable frame, and before position commands start. */
            if ((current_time_ms() - state->run_mode_ms) < 50U) {
                continue;
            }
            if (!encode_enable_for_axis(axis_config, &tx_frame)
                || !send_frame_for_axis(axis, &tx_frame)) {
                continue;
            }
            state->enable_sent = true;
            state->enable_ms = current_time_ms();
            continue;
        }
        if ((current_time_ms() - state->enable_ms) < 50U) {
            continue;
        }
#endif

        snapshot_command(axis, &command);

        if (axis_config->reverse) {
            command.position_rad = -command.position_rad;
            command.velocity_rad_s = -command.velocity_rad_s;
            command.torque_nm = -command.torque_nm;
        }

        if (encode_motion_for_axis(axis_config, &command, &tx_frame)) {
            (void)send_frame_for_axis(axis, &tx_frame);
        }
    }
#endif
}


bool motor_manager_init(void)
{
    uint32_t now_ms = current_time_ms();
    uint8_t axis;

    if (!validate_axis_configuration()) {
        return false;
    }

    memset(&s_motor, 0, sizeof(s_motor));
    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        motor_fault_init(&s_motor.axes[axis].fault);
        s_motor.axes[axis].has_feedback_once = false;
        s_motor.axes[axis].command_stop_attempts = 0U;
        s_motor.axes[axis].probe_state = MOTOR_PROBE_WAITING;
        s_motor.axes[axis].probe_attempts = 0U;
        s_motor.axes[axis].probe_deadline_ms = now_ms + MOTOR_MANAGER_PROBE_WINDOW_MS;
        s_motor.axes[axis].probe_next_ms = now_ms;
    }
    s_motor.performance.mchtmr_freq_hz = clock_get_frequency(clock_mchtmr0);
    s_motor.start_time_ms = now_ms;
    s_motor.last_control_ms = now_ms;
    s_motor.last_command_ms = now_ms;
    s_motor.last_command_sequence = gateway_app_get_motor_command_sequence();
    return true;
}

static void clear_command_stop_requests(void)
{
    uint8_t axis;

    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        s_motor.axes[axis].command_stop_pending = false;
    }
}

static void request_command_timeout_stop(void)
{
    uint8_t axis;

    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);

        if ((axis_config != NULL) && axis_config->enabled
            && s_motor.axes[axis].has_feedback_once
            && motor_feedback_guard_motion_allowed(
                &s_motor.axes[axis].feedback_guard)) {
            s_motor.axes[axis].command_stop_attempts = 0U;
            s_motor.axes[axis].command_stop_pending = true;
            s_motor.axes[axis].enable_sent = false;
        }
    }
}

static bool service_command_stop_requests(void)
{
    uint8_t axis;
    bool all_sent = true;

    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);
        can_if_frame_t stop_frame;

        if (!s_motor.axes[axis].command_stop_pending) {
            continue;
        }

        if ((axis_config == NULL) || !axis_config->enabled
            || !s_motor.axes[axis].has_feedback_once) {
            s_motor.axes[axis].command_stop_pending = false;
            continue;
        }

        if (encode_stop_for_axis(axis_config, &stop_frame)
            && send_frame_for_axis(axis, &stop_frame)) {
            s_motor.axes[axis].command_stop_pending = false;
            s_motor.axes[axis].command_stop_attempts = 0U;
        } else {
            s_motor.axes[axis].command_stop_attempts++;
            if (s_motor.axes[axis].command_stop_attempts
                >= MOTOR_MANAGER_STOP_MAX_ATTEMPTS) {
                /* Give up on the wire, keep the latch: the axis stays blocked
                 * until an explicit reset instead of flooding a dead bus.
                 */
                s_motor.axes[axis].command_stop_pending = false;
                s_motor.axes[axis].enable_sent = false;
                s_motor.axes[axis].command_stop_attempts = 0U;
            } else {
                all_sent = false;
            }
        }
    }

    return all_sent;
}

void motor_manager_poll(void)
{
    uint64_t poll_start_ticks = read_perf_counter_ticks();
    uint32_t command_sequence;
    uint32_t now_ms = current_time_ms();

    process_received_frames();

    command_sequence = gateway_app_get_motor_command_sequence();
    if (command_sequence != s_motor.last_command_sequence) {
        s_motor.last_command_sequence = command_sequence;
        s_motor.last_command_ms = now_ms;
        s_motor.has_received_command = true;
        s_motor.has_sent_timeout_stop = false;
        s_motor.command_timeout_stop_active = false;
        clear_command_stop_requests();
    }

    if ((now_ms - s_motor.last_control_ms) >= MOTOR_MANAGER_CONTROL_PERIOD_MS) {
        s_motor.last_control_ms = now_ms;
        service_can_bus_health(now_ms);
        service_probe(now_ms);
        service_feedback_safety(now_ms);
        if (s_motor.has_received_command
            && ((now_ms - s_motor.last_command_ms) <= MOTOR_MANAGER_COMMAND_TIMEOUT_MS)) {
            send_motion_controls();
        } else if (s_motor.has_received_command && !s_motor.has_sent_timeout_stop) {
            if (!s_motor.command_timeout_stop_active) {
                request_command_timeout_stop();
                s_motor.command_timeout_stop_active = true;
                s_motor.events.timeout_stop_count++;
            }
            s_motor.has_sent_timeout_stop = service_command_stop_requests();
            if (s_motor.has_sent_timeout_stop) {
                s_motor.command_timeout_stop_active = false;
            }
        }
    }

    {
        uint32_t elapsed_us = perf_ticks_to_microseconds(read_perf_counter_ticks()
                                                         - poll_start_ticks);

        update_maximum_u32(&s_motor.performance.poll_max_us, elapsed_us);
        if (elapsed_us > MOTOR_MANAGER_POLL_DEADLINE_US) {
            s_motor.performance.poll_overrun_count++;
        }
    }
}

void motor_manager_stop_all(void)
{
    request_command_timeout_stop();
    (void)service_command_stop_requests();
}

void motor_manager_print_telemetry(void)
{
    uint8_t axis;
    uint32_t now_ms = current_time_ms();
    uint32_t uptime_ms = now_ms - s_motor.start_time_ms;

    /* One header makes stream resynchronization trivial after USB/UART noise. */
    printf("MT,1,%lu,%lu,%lu\r\n", (unsigned long)uptime_ms,
           (unsigned long)s_motor.last_command_sequence,
           (unsigned long)s_motor.events.pdo_restart_count);
    for (axis = 0U; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        REAL32 position = 0.0f;
        REAL32 velocity = 0.0f;
        REAL32 torque = 0.0f;
        const motor_axis_state *state = &s_motor.axes[axis];
        bool online = state->online
                   && ((now_ms - state->feedback_guard.last_feedback_ms)
                       <= MOTOR_MANAGER_FEEDBACK_TIMEOUT_MS);

        (void)gateway_app_get_motor_feedback((UINT16)axis,
                                              &position,
                                              &velocity,
                                              &torque);
        printf("MD,1,%u,%u,%u,0x%02X,%.6f,%.6f,%.6f,%.2f,%u\r\n",
               (unsigned int)(axis + 1U),
               online ? 1U : 0U,
               state->enable_sent ? 1U : 0U,
               (unsigned int)state->fault_bits,
               (double)position,
               (double)velocity,
               (double)torque,
               (double)state->temperature_c,
               (unsigned int)motor_fault_cause(&state->fault));
    }
}

void motor_manager_print_status(void)
{
    uint8_t axis;
    uint8_t bus;
    uint8_t online_count = 0;
    uint8_t enabled_count = 0;
    uint32_t can_initialized_mask;
    uint32_t command_age_ms;
    uint32_t rx_total = 0;
    uint32_t tx_total = 0;
    uint32_t feedback_total = 0;
    uint32_t feedback_stop_failure_total = 0;
    uint32_t elapsed_ms;
    uint32_t tx_rate_hz = 0;
    uint32_t rx_rate_hz = 0;
    uint32_t feedback_rate_hz = 0;
    TickType_t now_tick;
    TickType_t elapsed_ticks;
    uint32_t now_ms;
    for (axis = 0; axis < MOTOR_MANAGER_AXIS_COUNT; axis++) {
        const motor_axis_config_t *axis_config = motor_config_get_axis(axis);

        if ((axis_config != NULL) && axis_config->enabled) {
            enabled_count++;
        }
        if (s_motor.axes[axis].online) {
            online_count++;
        }
        rx_total += s_motor.axes[axis].rx_count;
        tx_total += s_motor.axes[axis].tx_count;
        feedback_total += s_motor.axes[axis].feedback_count;
        feedback_stop_failure_total +=
            s_motor.axes[axis].feedback_guard.stop_failure_count;
    }

    now_ms = current_time_ms();
    command_age_ms = s_motor.has_received_command
                   ? (now_ms - s_motor.last_command_ms)
                   : 0U;
    can_initialized_mask = can_if_get_initialized_mask();
    now_tick = xTaskGetTickCount();
    elapsed_ticks = (s_motor.status.previous_tick == 0U)
                  ? 0U
                  : (now_tick - s_motor.status.previous_tick);
    elapsed_ms = (uint32_t)((elapsed_ticks * 1000U) / configTICK_RATE_HZ);
    if (elapsed_ticks != 0U) {
        tx_rate_hz = (uint32_t)(((uint64_t)(tx_total - s_motor.status.previous_tx_total)
                                 * configTICK_RATE_HZ) / elapsed_ticks);
        rx_rate_hz = (uint32_t)(((uint64_t)(rx_total - s_motor.status.previous_rx_total)
                                 * configTICK_RATE_HZ) / elapsed_ticks);
        feedback_rate_hz = (uint32_t)(((uint64_t)(feedback_total
                                                  - s_motor.status.previous_feedback_total)
                                       * configTICK_RATE_HZ) / elapsed_ticks);
    }
    s_motor.status.previous_tick = now_tick;
    s_motor.status.previous_tx_total = tx_total;
    s_motor.status.previous_rx_total = rx_total;
    s_motor.status.previous_feedback_total = feedback_total;

    printf("motor: uptime=%lums online=%u/%u totalAxes=%u tx=%lu rx=%lu fb=%lu direct=%u "
           "cmd=%u seq=%lu age=%lums timeoutStop=%u timeoutStops=%lu feedbackTimeouts=%lu "
           "feedbackStopFails=%lu faultReports=%lu motorFaults=%lu idMismatch=%lu invalidFrames=%lu "
           "staleFrames=%lu rxOverflow=%lu busOffs=%lu "
           "dt=%lums txHz=%lu rxHz=%lu fbHz=%lu\r\n",
           (unsigned long)(now_ms - s_motor.start_time_ms),
           online_count,
           enabled_count,
           (unsigned int)MOTOR_MANAGER_AXIS_COUNT,
           tx_total,
           rx_total,
           feedback_total,
           (unsigned int)MOTOR_MANAGER_ENABLE_MOTION_CONTROL,
           s_motor.has_received_command ? 1U : 0U,
           (unsigned long)s_motor.last_command_sequence,
           (unsigned long)command_age_ms,
           s_motor.has_sent_timeout_stop ? 1U : 0U,
           (unsigned long)s_motor.events.timeout_stop_count,
           (unsigned long)s_motor.events.feedback_timeout_count,
           (unsigned long)feedback_stop_failure_total,
           (unsigned long)s_motor.events.fault_report_count,
           (unsigned long)s_motor.events.motor_fault_count,
           (unsigned long)s_motor.events.id_mismatch_count,
           (unsigned long)s_motor.events.invalid_frame_count,
           (unsigned long)s_motor.events.stale_frame_count,
           (unsigned long)s_motor.events.rx_overflow_count,
           (unsigned long)s_motor.events.bus_off_count,
           (unsigned long)elapsed_ms,
           (unsigned long)tx_rate_hz,
           (unsigned long)rx_rate_hz,
           (unsigned long)feedback_rate_hz);

    printf("motor timing: pollMaxUs=%lu pollOverrun=%lu canSendMaxUs=%lu "
           "canSendFail=%lu canSendAttempt=%lu mchtmrHz=%lu\r\n",
           (unsigned long)s_motor.performance.poll_max_us,
           (unsigned long)s_motor.performance.poll_overrun_count,
           (unsigned long)s_motor.performance.can_send_max_us,
           (unsigned long)s_motor.performance.can_send_fail_count,
           (unsigned long)s_motor.performance.can_send_attempt_count,
           (unsigned long)s_motor.performance.mchtmr_freq_hz);

    if (s_motor.events.has_fault_report
        && (s_motor.events.fault_report_count
            != s_motor.status.previous_fault_report_count)) {
        const robstride_fault_report_t *fault_report = &s_motor.events.last_fault_report;

        printf("motor fault: count=%lu bus=%u id=%u fault=0x%08lX warning=0x%08lX\r\n",
               (unsigned long)s_motor.events.fault_report_count,
               fault_report->bus,
               fault_report->motor_id,
               (unsigned long)fault_report->fault_value,
               (unsigned long)fault_report->warning_value);
        s_motor.status.previous_fault_report_count = s_motor.events.fault_report_count;
    }

    for (bus = 0U; bus < CAN_IF_BUS_COUNT; bus++) {
        can_if_bus_stats_t can_stats;

        if ((can_initialized_mask & (1UL << bus)) == 0U) {
            continue;
        }

        if (!can_if_get_bus_stats(bus, &can_stats)) {
            continue;
        }

        printf("can%u: tx=%lu rx=%lu err=%lu drop=%lu isr=%lu isrMaxUs=%lu "
               "fifo0=%lu fifo1=%lu fifoFull=%lu fifoLost=%lu "
               "ring=%lu/%lu txFull=%lu ringFull=%lu busOff=%lu busOffRec=%lu "
               "errPassive=%lu state=%u\r\n",
               (unsigned int)bus,
               (unsigned long)can_stats.tx_count,
               (unsigned long)can_stats.rx_count,
               (unsigned long)can_stats.error_count,
               (unsigned long)can_stats.drop_count,
               (unsigned long)can_stats.isr_count,
               (unsigned long)can_stats.isr_max_us,
               (unsigned long)can_stats.rx_fifo0_count,
               (unsigned long)can_stats.rx_fifo1_count,
               (unsigned long)can_stats.rx_fifo_full_count,
               (unsigned long)can_stats.rx_fifo_lost_count,
               (unsigned long)can_stats.rx_ring_level,
               (unsigned long)can_stats.rx_ring_high_water,
               (unsigned long)can_stats.tx_fifo_full_count,
               (unsigned long)can_stats.rx_ring_full_count,
               (unsigned long)can_stats.bus_off_count,
               (unsigned long)can_stats.bus_off_recovery_count,
               (unsigned long)can_stats.error_passive_count,
               (unsigned int)can_stats.bus_state);
    }

}

bool motor_manager_reset_axis_fault(uint8_t axis_index)
{
    motor_axis_state *state;

    if (axis_index >= MOTOR_MANAGER_AXIS_COUNT) {
        return false;
    }

    state = &s_motor.axes[axis_index];
    (void)motor_fault_reset(&state->fault);
    motor_feedback_guard_init(&state->feedback_guard);
    state->has_feedback_once = false;
    state->probe_state = MOTOR_PROBE_WAITING;
    state->probe_attempts = 0U;
    state->probe_deadline_ms = current_time_ms() + MOTOR_MANAGER_PROBE_WINDOW_MS;
    state->probe_next_ms = current_time_ms();
    state->online = false;
    state->stop_pending = false;
    state->command_stop_pending = false;
    /* Clearing a fault is not an enable: motion needs a fresh command. */
    state->enable_sent = false;
    state->run_mode_sent = false;
    return true;
}

#if MOTOR_MANAGER_ENABLE_FAULT_INJECTION
void motor_manager_inject_faults(uint32_t mask)
{
    s_inject_mask = mask;
}
#endif


