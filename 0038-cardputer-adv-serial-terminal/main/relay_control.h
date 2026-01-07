#pragma once

#include <stdbool.h>

#include "hal/gpio_types.h"

typedef struct {
    gpio_num_t gpio;
    bool active_high;
    bool ready;
    bool on;
} relay_control_t;

bool relay_control_init(relay_control_t *r, gpio_num_t gpio, bool active_high, bool default_on);
bool relay_control_is_ready(const relay_control_t *r);
bool relay_control_is_on(const relay_control_t *r);
void relay_control_set(relay_control_t *r, bool on);
void relay_control_toggle(relay_control_t *r);

