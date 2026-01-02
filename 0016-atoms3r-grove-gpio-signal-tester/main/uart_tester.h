#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"

enum class UartMap : uint8_t {
    Normal = 0,  // RX=GPIO1, TX=GPIO2
    Swapped = 1, // RX=GPIO2, TX=GPIO1
};

typedef struct {
    int baud;
    UartMap map;
} uart_tester_config_t;

typedef struct {
    uint32_t tx_bytes_total;
    uint32_t rx_bytes_total;
    uint32_t rx_buf_used;
    uint32_t rx_buf_dropped;
} uart_tester_stats_t;

// Stop UART tester completely (stop tasks, delete UART driver, free buffers).
void uart_tester_stop(void);

// Start UART tester in TX mode (repeating payload).
// - payload: a single token (no spaces) in v1; may be empty.
// - delay_ms: inter-send delay; 0 means best-effort back-to-back (still yields).
esp_err_t uart_tester_start_tx(const uart_tester_config_t &cfg, const char *payload, int delay_ms);

// Start UART tester in RX mode (buffers incoming bytes).
esp_err_t uart_tester_start_rx(const uart_tester_config_t &cfg);

// Update TX settings while running in TX mode.
void uart_tester_tx_set(const char *payload, int delay_ms);

// Stop TX loop while staying configured (TX idle).
void uart_tester_tx_stop(void);

// Clear RX buffer and reset RX buffer drop counter.
void uart_tester_rx_clear(void);

// Drain up to max_bytes from RX buffer into out; returns bytes read.
size_t uart_tester_rx_drain(uint8_t *out, size_t max_bytes);

// Snapshot stats for LCD/status.
uart_tester_stats_t uart_tester_stats_snapshot(void);




