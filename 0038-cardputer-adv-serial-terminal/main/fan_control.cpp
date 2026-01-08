#include "fan_control.h"

#include <string.h>

static const fan_preset_t s_presets[] = {
    {.name = "slow", .steps_ms = {2000, 2000}, .steps_len = 2},
    {.name = "slower", .steps_ms = {5000, 5000}, .steps_len = 2},
    {.name = "pulse", .steps_ms = {250, 2750}, .steps_len = 2},
    {.name = "duty25", .steps_ms = {1000, 3000}, .steps_len = 2},
    {.name = "duty75", .steps_ms = {3000, 1000}, .steps_len = 2},
    {.name = "double", .steps_ms = {250, 250, 250, 2250}, .steps_len = 4},
    {.name = "triple", .steps_ms = {250, 250, 250, 250, 250, 1750}, .steps_len = 6},
    {.name = "sos", .steps_ms = {250, 250, 250, 250, 250, 750, 750, 250, 750, 1750}, .steps_len = 10},
};

static void ensure_task(fan_control_t *fan);
static void save_manual_if_needed(fan_control_t *fan);
static void restore_manual_if_needed(fan_control_t *fan);

static void anim_task(void *arg) {
    auto *fan = (fan_control_t *)arg;
    for (;;) {
        if (!fan || !fan->enabled || fan->mode == FAN_MODE_NONE) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        const fan_mode_t mode = fan->mode;
        if (mode == FAN_MODE_BLINK) {
            uint32_t on_ms = fan->blink_on_ms ? fan->blink_on_ms : 1;
            uint32_t off_ms = fan->blink_off_ms ? fan->blink_off_ms : 1;
            while (fan->enabled && fan->mode == FAN_MODE_BLINK) {
                relay_control_set(&fan->relay, true);
                vTaskDelay(pdMS_TO_TICKS(on_ms));
                if (!fan->enabled || fan->mode != FAN_MODE_BLINK) break;
                relay_control_set(&fan->relay, false);
                vTaskDelay(pdMS_TO_TICKS(off_ms));
            }
            continue;
        }

        if (mode == FAN_MODE_TICK) {
            uint32_t on_ms = fan->tick_on_ms ? fan->tick_on_ms : 1;
            uint32_t period_ms = fan->tick_period_ms ? fan->tick_period_ms : 1;
            if (period_ms <= on_ms) period_ms = on_ms + 1;
            uint32_t off_ms = period_ms - on_ms;
            while (fan->enabled && fan->mode == FAN_MODE_TICK) {
                relay_control_set(&fan->relay, true);
                vTaskDelay(pdMS_TO_TICKS(on_ms));
                if (!fan->enabled || fan->mode != FAN_MODE_TICK) break;
                relay_control_set(&fan->relay, false);
                vTaskDelay(pdMS_TO_TICKS(off_ms));
            }
            continue;
        }

        if (mode == FAN_MODE_BEAT) {
            while (fan->enabled && fan->mode == FAN_MODE_BEAT) {
                relay_control_set(&fan->relay, true);
                vTaskDelay(pdMS_TO_TICKS(250));
                if (!fan->enabled || fan->mode != FAN_MODE_BEAT) break;
                relay_control_set(&fan->relay, false);
                vTaskDelay(pdMS_TO_TICKS(250));
                if (!fan->enabled || fan->mode != FAN_MODE_BEAT) break;
                relay_control_set(&fan->relay, true);
                vTaskDelay(pdMS_TO_TICKS(250));
                if (!fan->enabled || fan->mode != FAN_MODE_BEAT) break;
                relay_control_set(&fan->relay, false);
                vTaskDelay(pdMS_TO_TICKS(2250));
            }
            continue;
        }

        if (mode == FAN_MODE_BURST) {
            uint32_t count = fan->burst_count ? fan->burst_count : 1;
            uint32_t on_ms = fan->burst_on_ms ? fan->burst_on_ms : 1;
            uint32_t off_ms = fan->burst_off_ms ? fan->burst_off_ms : 1;
            uint32_t pause_ms = fan->burst_pause_ms;

            while (fan->enabled && fan->mode == FAN_MODE_BURST) {
                for (uint32_t i = 0; i < count; i++) {
                    if (!fan->enabled || fan->mode != FAN_MODE_BURST) break;
                    relay_control_set(&fan->relay, true);
                    vTaskDelay(pdMS_TO_TICKS(on_ms));
                    if (!fan->enabled || fan->mode != FAN_MODE_BURST) break;
                    relay_control_set(&fan->relay, false);
                    vTaskDelay(pdMS_TO_TICKS(off_ms));
                }
                if (!fan->enabled || fan->mode != FAN_MODE_BURST) break;
                if (pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
            }
            continue;
        }

        if (mode == FAN_MODE_PRESET) {
            const fan_preset_t *preset = fan->active_preset;
            if (!preset || preset->steps_len <= 0) {
                fan->mode = FAN_MODE_NONE;
                continue;
            }
            while (fan->enabled && fan->mode == FAN_MODE_PRESET) {
                for (int i = 0; i < preset->steps_len; i++) {
                    if (!fan->enabled || fan->mode != FAN_MODE_PRESET) break;
                    const bool on = (i % 2) == 0;
                    relay_control_set(&fan->relay, on);
                    uint32_t ms = preset->steps_ms[i];
                    if (ms == 0) ms = 1;
                    vTaskDelay(pdMS_TO_TICKS(ms));
                }
            }
            continue;
        }
    }
}

void fan_control_init_defaults(fan_control_t *fan) {
    if (!fan) return;
    memset(fan, 0, sizeof(*fan));
    fan->blink_on_ms = 2000;
    fan->blink_off_ms = 2000;
    fan->tick_on_ms = 200;
    fan->tick_period_ms = 2000;
    fan->burst_count = 3;
    fan->burst_on_ms = 200;
    fan->burst_off_ms = 200;
    fan->burst_pause_ms = 2000;
}

bool fan_control_init_relay(fan_control_t *fan, gpio_num_t gpio, bool active_high, bool default_on) {
    if (!fan) return false;
    fan_control_init_defaults(fan);
    return relay_control_init(&fan->relay, gpio, active_high, default_on);
}

bool fan_control_is_ready(const fan_control_t *fan) {
    if (!fan) return false;
    return relay_control_is_ready(&fan->relay);
}

bool fan_control_is_on(const fan_control_t *fan) {
    if (!fan) return false;
    return relay_control_is_on(&fan->relay);
}

fan_mode_t fan_control_mode(const fan_control_t *fan) {
    if (!fan) return FAN_MODE_NONE;
    return fan->mode;
}

const char *fan_control_mode_name(fan_mode_t mode) {
    switch (mode) {
        case FAN_MODE_NONE:
            return "none";
        case FAN_MODE_BLINK:
            return "blink";
        case FAN_MODE_TICK:
            return "tick";
        case FAN_MODE_BEAT:
            return "beat";
        case FAN_MODE_BURST:
            return "burst";
        case FAN_MODE_PRESET:
            return "preset";
    }
    return "?";
}

const char *fan_control_active_preset_name(const fan_control_t *fan) {
    if (!fan || !fan->active_preset) return nullptr;
    return fan->active_preset->name;
}

static void ensure_task(fan_control_t *fan) {
    if (!fan) return;
    if (fan->task) return;
    xTaskCreate(&anim_task, "fan_anim", 3072, fan, 2, &fan->task);
}

static void save_manual_if_needed(fan_control_t *fan) {
    if (!fan) return;
    if (fan->saved_manual_valid) return;
    fan->saved_manual_on = relay_control_is_on(&fan->relay);
    fan->saved_manual_valid = true;
}

static void restore_manual_if_needed(fan_control_t *fan) {
    if (!fan) return;
    if (!fan->saved_manual_valid) return;
    relay_control_set(&fan->relay, fan->saved_manual_on);
    fan->saved_manual_valid = false;
}

void fan_control_stop(fan_control_t *fan) {
    if (!fan) return;
    fan->enabled = false;
    fan->mode = FAN_MODE_NONE;
    fan->active_preset = nullptr;
    if (fan->task) xTaskNotifyGive(fan->task);
    restore_manual_if_needed(fan);
}

void fan_control_set_manual(fan_control_t *fan, bool on) {
    if (!fan) return;
    fan_control_stop(fan);
    relay_control_set(&fan->relay, on);
}

void fan_control_toggle_manual(fan_control_t *fan) {
    if (!fan) return;
    fan_control_stop(fan);
    relay_control_toggle(&fan->relay);
}

static void start_mode(fan_control_t *fan, fan_mode_t mode) {
    if (!fan) return;
    if (!fan_control_is_ready(fan)) return;
    save_manual_if_needed(fan);
    fan->mode = mode;
    fan->enabled = true;
    ensure_task(fan);
    if (fan->task) xTaskNotifyGive(fan->task);
}

void fan_control_start_blink(fan_control_t *fan, uint32_t on_ms, uint32_t off_ms) {
    if (!fan) return;
    fan->blink_on_ms = on_ms;
    fan->blink_off_ms = off_ms;
    start_mode(fan, FAN_MODE_BLINK);
}

void fan_control_start_tick(fan_control_t *fan, uint32_t on_ms, uint32_t period_ms) {
    if (!fan) return;
    fan->tick_on_ms = on_ms;
    fan->tick_period_ms = period_ms;
    start_mode(fan, FAN_MODE_TICK);
}

void fan_control_start_beat(fan_control_t *fan) {
    start_mode(fan, FAN_MODE_BEAT);
}

void fan_control_start_burst(fan_control_t *fan, uint32_t count, uint32_t on_ms, uint32_t off_ms, uint32_t pause_ms) {
    if (!fan) return;
    fan->burst_count = count;
    fan->burst_on_ms = on_ms;
    fan->burst_off_ms = off_ms;
    fan->burst_pause_ms = pause_ms;
    start_mode(fan, FAN_MODE_BURST);
}

bool fan_control_start_preset(fan_control_t *fan, const char *name) {
    if (!fan || !name) return false;
    for (size_t i = 0; i < (sizeof(s_presets) / sizeof(s_presets[0])); i++) {
        if (strcmp(s_presets[i].name, name) == 0) {
            fan->active_preset = &s_presets[i];
            start_mode(fan, FAN_MODE_PRESET);
            return true;
        }
    }
    return false;
}

const fan_preset_t *fan_control_presets(size_t *out_count) {
    if (out_count) *out_count = sizeof(s_presets) / sizeof(s_presets[0]);
    return s_presets;
}
