#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "hal/gpio_types.h"

#define FAN_RELAY_DEFAULT_GPIO GPIO_NUM_3
#define FAN_RELAY_DEFAULT_ACTIVE_HIGH false

esp_err_t fan_relay_init(gpio_num_t gpio, bool active_high, bool default_on);

bool fan_relay_is_ready(void);
gpio_num_t fan_relay_gpio(void);
bool fan_relay_active_high(void);

bool fan_relay_is_on(void);
void fan_relay_set(bool on);
void fan_relay_set_on(void);
void fan_relay_set_off(void);
