#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "relay_control.h"

typedef enum {
    FAN_MODE_NONE = 0,
    FAN_MODE_BLINK,
    FAN_MODE_TICK,
    FAN_MODE_BEAT,
    FAN_MODE_BURST,
    FAN_MODE_PRESET,
} fan_mode_t;

typedef struct {
    const char *name;
    const uint32_t steps_ms[10]; // alternating on/off durations, starting with ON
    int steps_len;
} fan_preset_t;

typedef struct {
    relay_control_t relay;

    TaskHandle_t task;
    bool enabled;
    fan_mode_t mode;

    bool saved_manual_valid;
    bool saved_manual_on;

    uint32_t blink_on_ms;
    uint32_t blink_off_ms;

    uint32_t tick_on_ms;
    uint32_t tick_period_ms;

    uint32_t burst_count;
    uint32_t burst_on_ms;
    uint32_t burst_off_ms;
    uint32_t burst_pause_ms;

    const fan_preset_t *active_preset;
} fan_control_t;

void fan_control_init_defaults(fan_control_t *fan);
bool fan_control_init_relay(fan_control_t *fan, gpio_num_t gpio, bool active_high, bool default_on);

bool fan_control_is_ready(const fan_control_t *fan);
bool fan_control_is_on(const fan_control_t *fan);
fan_mode_t fan_control_mode(const fan_control_t *fan);
const char *fan_control_mode_name(fan_mode_t mode);
const char *fan_control_active_preset_name(const fan_control_t *fan);

void fan_control_stop(fan_control_t *fan);
void fan_control_set_manual(fan_control_t *fan, bool on);
void fan_control_toggle_manual(fan_control_t *fan);

void fan_control_start_blink(fan_control_t *fan, uint32_t on_ms, uint32_t off_ms);
void fan_control_start_tick(fan_control_t *fan, uint32_t on_ms, uint32_t period_ms);
void fan_control_start_beat(fan_control_t *fan);
void fan_control_start_burst(fan_control_t *fan, uint32_t count, uint32_t on_ms, uint32_t off_ms, uint32_t pause_ms);
bool fan_control_start_preset(fan_control_t *fan, const char *name);

const fan_preset_t *fan_control_presets(size_t *out_count);
