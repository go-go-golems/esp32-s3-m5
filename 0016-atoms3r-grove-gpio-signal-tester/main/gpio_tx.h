#pragma once

#include <stdint.h>

#include "driver/gpio.h"

enum class TxMode : uint8_t {
    Stopped = 0,
    High = 1,
    Low = 2,
    Square = 3,
    Pulse = 4,
};

typedef struct {
    TxMode mode;
    int hz;         // for Square
    int width_us;   // for Pulse
    int period_ms;  // for Pulse
} tx_config_t;

// Apply TX output configuration for a GPIO pin (GPIO-owned mode).
void gpio_tx_apply(gpio_num_t pin, const tx_config_t &cfg);

// Stop TX generation and return pin to input (best-effort).
void gpio_tx_stop(void);



