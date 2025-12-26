#pragma once

#include <stdint.h>

#include "driver/gpio.h"

#include "gpio_rx.h"
#include "gpio_tx.h"

enum class TesterMode : uint8_t {
    Idle = 0,
    Tx = 1,
    Rx = 2,
};

typedef struct {
    TesterMode mode;
    int active_pin; // 1 or 2
    tx_config_t tx;
    RxEdgeMode rx_edges;
    RxPullMode rx_pull;
} tester_state_t;

void tester_state_init(void);
void tester_state_apply_event(const struct CtrlEvent &ev);
tester_state_t tester_state_snapshot(void);



