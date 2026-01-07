#include "relay_control.h"

#include "driver/gpio.h"

bool relay_control_init(relay_control_t *r, gpio_num_t gpio, bool active_high, bool default_on) {
    if (!r) return false;
    if (gpio == GPIO_NUM_NC) return false;

    r->gpio = gpio;
    r->active_high = active_high;
    r->ready = true;
    r->on = false;

    gpio_reset_pin(gpio);
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    (void)gpio_pullup_dis(gpio);
    (void)gpio_pulldown_dis(gpio);

    relay_control_set(r, default_on);
    return true;
}

bool relay_control_is_ready(const relay_control_t *r) {
    return r && r->ready;
}

bool relay_control_is_on(const relay_control_t *r) {
    return r && r->on;
}

void relay_control_set(relay_control_t *r, bool on) {
    if (!r || !r->ready) return;
    r->on = on;
    const int level = (on ^ (!r->active_high)) ? 1 : 0;
    gpio_set_level(r->gpio, level);
}

void relay_control_toggle(relay_control_t *r) {
    if (!r || !r->ready) return;
    relay_control_set(r, !r->on);
}

