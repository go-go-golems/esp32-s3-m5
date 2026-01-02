#pragma once

#include <stdint.h>

#include "driver/gpio.h"

#include "gpio_rx.h"
#include "gpio_tx.h"
#include "uart_tester.h"

enum class TesterMode : uint8_t {
    Idle = 0,
    Tx = 1,
    Rx = 2,
    UartTx = 3,
    UartRx = 4,
};

typedef struct {
    TesterMode mode;
    int active_pin; // 1 or 2
    tx_config_t tx;
    RxEdgeMode rx_edges;
    RxPullMode rx_pull;

    // UART peripheral tester config.
    int uart_baud;
    UartMap uart_map;
    bool uart_tx_enabled;
    int uart_tx_delay_ms;
    char uart_tx_token[64];
} tester_state_t;

void tester_state_init(void);
void tester_state_apply_event(const struct CtrlEvent &ev);
tester_state_t tester_state_snapshot(void);



