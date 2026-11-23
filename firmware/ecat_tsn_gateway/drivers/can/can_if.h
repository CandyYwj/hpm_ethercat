/*
 * can_if.h - Hardware-independent CAN interface for motor buses.
 * Author: BearKing
 */
#ifndef CAN_IF_H
#define CAN_IF_H

#include <stdbool.h>
#include <stdint.h>

#define CAN_IF_BUS_COUNT        8U
#define CAN_IF_CLASSIC_MAX_DLC  8U
#define CAN_IF_DEFAULT_BITRATE_BPS 1000000UL

typedef enum {
    CAN_IF_ID_STANDARD = 0,
    CAN_IF_ID_EXTENDED = 1,
} can_if_id_format_t;

typedef enum {
    CAN_IF_FRAME_CLASSIC = 0,
    CAN_IF_FRAME_FD = 1,
} can_if_frame_format_t;

typedef enum {
    CAN_IF_BUS_OK = 0,
    CAN_IF_BUS_ERROR_PASSIVE,
    CAN_IF_BUS_OFF,
} can_if_bus_state_t;

typedef struct {
    uint8_t bus;
    can_if_id_format_t id_format;
    can_if_frame_format_t frame_format;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[CAN_IF_CLASSIC_MAX_DLC];
} can_if_frame_t;

typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t drop_count;
    uint32_t isr_count;
    uint32_t isr_max_us;
    uint32_t rx_fifo0_count;
    uint32_t rx_fifo1_count;
    uint32_t rx_fifo_full_count;
    uint32_t rx_fifo_lost_count;
    uint32_t rx_ring_level;
    uint32_t rx_ring_high_water;
    uint32_t tx_fifo_full_count;
    uint32_t rx_ring_full_count;
    uint32_t bus_off_count;
    uint32_t bus_off_recovery_count;
    uint32_t error_passive_count;
    can_if_bus_state_t bus_state;
} can_if_bus_stats_t;

bool can_if_init(void);
bool can_if_send(const can_if_frame_t *frame);
bool can_if_receive(can_if_frame_t *frame);

uint32_t can_if_get_tx_count(uint8_t bus);
uint32_t can_if_get_rx_count(uint8_t bus);
uint32_t can_if_get_error_count(uint8_t bus);
uint32_t can_if_get_drop_count(uint8_t bus);
uint32_t can_if_get_initialized_mask(void);
bool can_if_get_bus_stats(uint8_t bus, can_if_bus_stats_t *stats);

/* Live MCAN bus state. */
can_if_bus_state_t can_if_get_bus_state(uint8_t bus);
/* Bounded, non-blocking bus-off recovery (init-mode cycle). Returns true when a
 * recovery was attempted this call. Callers must keep the motors stopped: this
 * function never restarts motion. */
bool can_if_service_bus_off(uint8_t bus);

#endif
