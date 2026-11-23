/*
 * motor_manager.h - EtherCAT command to motor CAN scheduler.
 * Author: BearKing
 */
#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include "motor_config.h"
#include <stdbool.h>
#include <stdint.h>

bool motor_manager_init(void);
void motor_manager_poll(void);
void motor_manager_stop_all(void);
void motor_manager_print_status(void);
void motor_manager_print_telemetry(void);

/* Explicit fault reset: clears the axis latch, leaves the motor disabled and
 * re-arms the presence probe. It never re-enables and never resumes motion. */
bool motor_manager_reset_axis_fault(uint8_t axis_index);

#if MOTOR_MANAGER_ENABLE_FAULT_INJECTION
/* Task 10 bench hooks: bit0 forces every CAN TX to fail, bit1 discards all
 * received frames. Never compiled into a production image. */
void motor_manager_inject_faults(uint32_t mask);
#endif

#endif
