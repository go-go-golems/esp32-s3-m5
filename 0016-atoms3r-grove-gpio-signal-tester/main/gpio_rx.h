#pragma once

#include <stdint.h>

#include "driver/gpio.h"

enum class RxEdgeMode : uint8_t {
    Rising = 0,
    Falling = 1,
    Both = 2,
};

enum class RxPullMode : uint8_t {
    None = 0,
    Up = 1,
    Down = 2,
};

typedef struct {
    uint32_t edges;
    uint32_t rises;
    uint32_t falls;
    uint32_t last_tick; // FreeRTOS tick count (from ISR)
    int last_level;     // gpio level at last interrupt
} rx_stats_t;

// Configure RX edge counter for a GPIO pin (GPIO-owned mode).
void gpio_rx_configure(gpio_num_t pin, RxEdgeMode edges, RxPullMode pull);

// Disable RX edge counting and detach ISR.
void gpio_rx_disable(void);

void gpio_rx_reset(void);
rx_stats_t gpio_rx_snapshot(void);



