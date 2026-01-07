#include "fan_relay.h"

#include "driver/gpio.h"

static gpio_num_t s_gpio = GPIO_NUM_NC;
static bool s_active_high = true;
static bool s_ready = false;
static bool s_on = false;

esp_err_t fan_relay_init(gpio_num_t gpio, bool active_high, bool default_on) {
    if (gpio == GPIO_NUM_NC) return ESP_ERR_INVALID_ARG;

    s_gpio = gpio;
    s_active_high = active_high;

    gpio_reset_pin(gpio);
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    (void)gpio_pullup_dis(gpio);
    (void)gpio_pulldown_dis(gpio);

    s_ready = true;
    fan_relay_set(default_on);
    return ESP_OK;
}

bool fan_relay_is_ready(void) {
    return s_ready;
}

gpio_num_t fan_relay_gpio(void) {
    return s_gpio;
}

bool fan_relay_active_high(void) {
    return s_active_high;
}

bool fan_relay_is_on(void) {
    return s_on;
}

void fan_relay_set(bool on) {
    if (!s_ready || s_gpio == GPIO_NUM_NC) return;
    s_on = on;
    const int level = (on ^ (!s_active_high)) ? 1 : 0;
    gpio_set_level(s_gpio, level);
}

void fan_relay_set_on(void) {
    fan_relay_set(true);
}

void fan_relay_set_off(void) {
    fan_relay_set(false);
}

