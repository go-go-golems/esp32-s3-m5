#include "fan_console.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fan_relay.h"

static const char *TAG = "fan_console";

typedef enum {
    FAN_ANIM_NONE = 0,
    FAN_ANIM_BLINK,
    FAN_ANIM_TICK,
    FAN_ANIM_BEAT,
    FAN_ANIM_BURST,
} fan_anim_mode_t;

static TaskHandle_t s_anim_task;
static bool s_anim_enabled = false;
static fan_anim_mode_t s_anim_mode = FAN_ANIM_NONE;

static bool s_saved_manual_valid = false;
static bool s_saved_manual_on = false;

static uint32_t s_blink_on_ms = 500;
static uint32_t s_blink_off_ms = 500;

static uint32_t s_tick_on_ms = 50;
static uint32_t s_tick_period_ms = 1000;

static uint32_t s_burst_count = 3;
static uint32_t s_burst_on_ms = 75;
static uint32_t s_burst_off_ms = 75;
static uint32_t s_burst_pause_ms = 1000;

static void print_help(void) {
    printf("fan commands:\n");
    printf("  fan status\n");
    printf("  fan on|off|toggle\n");
    printf("  fan stop\n");
    printf("  fan blink [on_ms] [off_ms]\n");
    printf("  fan strobe [ms]\n");
    printf("  fan tick [on_ms] [period_ms]\n");
    printf("  fan beat\n");
    printf("  fan burst [count] [on_ms] [off_ms] [pause_ms]\n");
}

static const char *anim_mode_name(fan_anim_mode_t m) {
    switch (m) {
        case FAN_ANIM_NONE:
            return "none";
        case FAN_ANIM_BLINK:
            return "blink";
        case FAN_ANIM_TICK:
            return "tick";
        case FAN_ANIM_BEAT:
            return "beat";
        case FAN_ANIM_BURST:
            return "burst";
    }
    return "?";
}

static bool parse_u32(const char *s, uint32_t *out) {
    if (!s || !out) return false;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if (!end || *end != '\0') return false;
    if (v > UINT32_MAX) return false;
    *out = (uint32_t)v;
    return true;
}

static void save_manual_state_if_needed(void) {
    if (s_saved_manual_valid) return;
    s_saved_manual_on = fan_relay_is_on();
    s_saved_manual_valid = true;
}

static void restore_manual_state_if_needed(void) {
    if (!s_saved_manual_valid) return;
    fan_relay_set(s_saved_manual_on);
    s_saved_manual_valid = false;
}

static void anim_task(void *arg) {
    (void)arg;
    for (;;) {
        if (!s_anim_enabled || s_anim_mode == FAN_ANIM_NONE) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        const fan_anim_mode_t mode = s_anim_mode;
        if (mode == FAN_ANIM_BLINK) {
            const uint32_t on_ms = s_blink_on_ms ? s_blink_on_ms : 1;
            const uint32_t off_ms = s_blink_off_ms ? s_blink_off_ms : 1;
            while (s_anim_enabled && s_anim_mode == FAN_ANIM_BLINK) {
                fan_relay_set_on();
                vTaskDelay(pdMS_TO_TICKS(on_ms));
                if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BLINK) break;
                fan_relay_set_off();
                vTaskDelay(pdMS_TO_TICKS(off_ms));
            }
            continue;
        }

        if (mode == FAN_ANIM_TICK) {
            const uint32_t on_ms = s_tick_on_ms ? s_tick_on_ms : 1;
            uint32_t period_ms = s_tick_period_ms ? s_tick_period_ms : 1;
            if (period_ms <= on_ms) period_ms = on_ms + 1;
            const uint32_t off_ms = period_ms - on_ms;
            while (s_anim_enabled && s_anim_mode == FAN_ANIM_TICK) {
                fan_relay_set_on();
                vTaskDelay(pdMS_TO_TICKS(on_ms));
                if (!s_anim_enabled || s_anim_mode != FAN_ANIM_TICK) break;
                fan_relay_set_off();
                vTaskDelay(pdMS_TO_TICKS(off_ms));
            }
            continue;
        }

        if (mode == FAN_ANIM_BEAT) {
            while (s_anim_enabled && s_anim_mode == FAN_ANIM_BEAT) {
                fan_relay_set_on();
                vTaskDelay(pdMS_TO_TICKS(100));
                if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BEAT) break;
                fan_relay_set_off();
                vTaskDelay(pdMS_TO_TICKS(100));
                if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BEAT) break;
                fan_relay_set_on();
                vTaskDelay(pdMS_TO_TICKS(100));
                if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BEAT) break;
                fan_relay_set_off();
                vTaskDelay(pdMS_TO_TICKS(700));
            }
            continue;
        }

        if (mode == FAN_ANIM_BURST) {
            uint32_t count = s_burst_count ? s_burst_count : 1;
            const uint32_t on_ms = s_burst_on_ms ? s_burst_on_ms : 1;
            const uint32_t off_ms = s_burst_off_ms ? s_burst_off_ms : 1;
            const uint32_t pause_ms = s_burst_pause_ms;

            while (s_anim_enabled && s_anim_mode == FAN_ANIM_BURST) {
                for (uint32_t i = 0; i < count; i++) {
                    if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BURST) break;
                    fan_relay_set_on();
                    vTaskDelay(pdMS_TO_TICKS(on_ms));
                    if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BURST) break;
                    fan_relay_set_off();
                    vTaskDelay(pdMS_TO_TICKS(off_ms));
                }
                if (!s_anim_enabled || s_anim_mode != FAN_ANIM_BURST) break;
                if (pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
            }
            continue;
        }
    }
}

static void anim_ensure_task(void) {
    if (s_anim_task) return;
    xTaskCreate(&anim_task, "fan_anim", 3072, NULL, 2, &s_anim_task);
}

static void anim_stop(void) {
    s_anim_enabled = false;
    s_anim_mode = FAN_ANIM_NONE;
    if (s_anim_task) xTaskNotifyGive(s_anim_task);
    restore_manual_state_if_needed();
}

static void anim_start(fan_anim_mode_t mode) {
    if (!fan_relay_is_ready()) {
        printf("fan relay not initialized\n");
        return;
    }
    save_manual_state_if_needed();
    s_anim_mode = mode;
    s_anim_enabled = true;
    anim_ensure_task();
    xTaskNotifyGive(s_anim_task);
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {0};
    cmd.command = "fan";
    cmd.help = "Fan relay control (GPIO3): fan help";
    cmd.func = NULL;
    cmd.func = &cmd_fan;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

int cmd_fan(int argc, char **argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        printf("fan: gpio=%d ready=%d active_high=%d on=%d\n",
               (int)fan_relay_gpio(),
               fan_relay_is_ready() ? 1 : 0,
               fan_relay_active_high() ? 1 : 0,
               fan_relay_is_on() ? 1 : 0);
        printf("anim: enabled=%d mode=%s saved_manual=%d\n",
               s_anim_enabled ? 1 : 0,
               anim_mode_name(s_anim_mode),
               s_saved_manual_valid ? 1 : 0);
        if (s_anim_mode == FAN_ANIM_BLINK) {
            printf("blink: on_ms=%" PRIu32 " off_ms=%" PRIu32 "\n", s_blink_on_ms, s_blink_off_ms);
        } else if (s_anim_mode == FAN_ANIM_TICK) {
            printf("tick: on_ms=%" PRIu32 " period_ms=%" PRIu32 "\n", s_tick_on_ms, s_tick_period_ms);
        } else if (s_anim_mode == FAN_ANIM_BURST) {
            printf("burst: count=%" PRIu32 " on_ms=%" PRIu32 " off_ms=%" PRIu32 " pause_ms=%" PRIu32 "\n",
                   s_burst_count,
                   s_burst_on_ms,
                   s_burst_off_ms,
                   s_burst_pause_ms);
        }
        return 0;
    }

    if (strcmp(argv[1], "on") == 0) {
        anim_stop();
        fan_relay_set_on();
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "off") == 0) {
        anim_stop();
        fan_relay_set_off();
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "toggle") == 0) {
        anim_stop();
        fan_relay_set(!fan_relay_is_on());
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "stop") == 0) {
        anim_stop();
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "blink") == 0) {
        uint32_t on_ms = s_blink_on_ms;
        uint32_t off_ms = s_blink_off_ms;
        if (argc >= 3 && !parse_u32(argv[2], &on_ms)) {
            printf("invalid on_ms: %s\n", argv[2]);
            return 1;
        }
        if (argc >= 4 && !parse_u32(argv[3], &off_ms)) {
            printf("invalid off_ms: %s\n", argv[3]);
            return 1;
        }
        s_blink_on_ms = on_ms;
        s_blink_off_ms = off_ms;
        anim_start(FAN_ANIM_BLINK);
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "strobe") == 0) {
        uint32_t ms = 100;
        if (argc >= 3 && !parse_u32(argv[2], &ms)) {
            printf("invalid ms: %s\n", argv[2]);
            return 1;
        }
        s_blink_on_ms = ms;
        s_blink_off_ms = ms;
        anim_start(FAN_ANIM_BLINK);
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "tick") == 0) {
        uint32_t on_ms = s_tick_on_ms;
        uint32_t period_ms = s_tick_period_ms;
        if (argc >= 3 && !parse_u32(argv[2], &on_ms)) {
            printf("invalid on_ms: %s\n", argv[2]);
            return 1;
        }
        if (argc >= 4 && !parse_u32(argv[3], &period_ms)) {
            printf("invalid period_ms: %s\n", argv[3]);
            return 1;
        }
        if (period_ms <= on_ms) {
            printf("period_ms must be > on_ms\n");
            return 1;
        }
        s_tick_on_ms = on_ms;
        s_tick_period_ms = period_ms;
        anim_start(FAN_ANIM_TICK);
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "beat") == 0) {
        anim_start(FAN_ANIM_BEAT);
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "burst") == 0) {
        uint32_t count = s_burst_count;
        uint32_t on_ms = s_burst_on_ms;
        uint32_t off_ms = s_burst_off_ms;
        uint32_t pause_ms = s_burst_pause_ms;

        if (argc >= 3 && !parse_u32(argv[2], &count)) {
            printf("invalid count: %s\n", argv[2]);
            return 1;
        }
        if (argc >= 4 && !parse_u32(argv[3], &on_ms)) {
            printf("invalid on_ms: %s\n", argv[3]);
            return 1;
        }
        if (argc >= 5 && !parse_u32(argv[4], &off_ms)) {
            printf("invalid off_ms: %s\n", argv[4]);
            return 1;
        }
        if (argc >= 6 && !parse_u32(argv[5], &pause_ms)) {
            printf("invalid pause_ms: %s\n", argv[5]);
            return 1;
        }

        s_burst_count = count;
        s_burst_on_ms = on_ms;
        s_burst_off_ms = off_ms;
        s_burst_pause_ms = pause_ms;
        anim_start(FAN_ANIM_BURST);
        printf("ok\n");
        return 0;
    }

    printf("unknown subcommand: %s (try: fan help)\n", argv[1]);
    return 1;
}

void fan_console_start(void) {
#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no REPL started");
    return;
#else
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "fan> ";
    repl_config.task_stack_size = 4096;

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over USB Serial/JTAG (try: help, fan help)");
#endif
}

