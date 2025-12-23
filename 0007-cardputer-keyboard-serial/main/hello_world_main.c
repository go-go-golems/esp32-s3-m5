/*
 * ESP32-S3 tutorial 0007:
 * Cardputer keyboard (GPIO matrix scan) -> realtime serial echo.
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_rom_sys.h"

static const char *TAG = "cardputer_keyboard";

// Backwards compatibility: if the project was configured before new Kconfig options existed,
// the CONFIG_ macros may be missing until you run `idf.py reconfigure` or `idf.py fullclean`.
#ifndef CONFIG_TUTORIAL_0007_SCAN_SETTLE_US
#define CONFIG_TUTORIAL_0007_SCAN_SETTLE_US 0
#endif
#ifndef CONFIG_TUTORIAL_0007_KB_ALT_IN0_GPIO
#define CONFIG_TUTORIAL_0007_KB_ALT_IN0_GPIO 1
#endif
#ifndef CONFIG_TUTORIAL_0007_KB_ALT_IN1_GPIO
#define CONFIG_TUTORIAL_0007_KB_ALT_IN1_GPIO 2
#endif
#ifndef CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS
#define CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS 0
#endif

typedef struct {
    const char *first;
    const char *second;
} key_value_t;

// Key legend matches the vendor HAL's "picture" coordinate system (4 rows x 14 columns).
// Derived from M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h.
static const key_value_t s_key_map[4][14] = {
    {{"`", "~"}, {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"}, {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"}, {"del", "del"}},
    {{"tab", "tab"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"}, {"\\", "|"}},
    {{"fn", "fn"}, {"shift", "shift"}, {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"}, {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"}, {"'", "\""}, {"enter", "enter"}},
    {{"ctrl", "ctrl"}, {"opt", "opt"}, {"alt", "alt"}, {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"}, {"space", "space"}},
};

static inline void kb_set_output(uint8_t out_bits3) {
    out_bits3 &= 0x07;
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0007_KB_OUT0_GPIO, (out_bits3 & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0007_KB_OUT1_GPIO, (out_bits3 & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0007_KB_OUT2_GPIO, (out_bits3 & 0x04) ? 1 : 0);
}

static inline uint8_t kb_get_input_mask(void) {
    // Input pins are pulled up; a pressed key reads low -> we invert to build a 7-bit pressed mask.
    const int in_pins[7] = {
        CONFIG_TUTORIAL_0007_KB_IN0_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN1_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN6_GPIO,
    };

    uint8_t mask = 0;
    for (int i = 0; i < 7; i++) {
        int level = gpio_get_level((gpio_num_t)in_pins[i]);
        if (level == 0) {
            mask |= (uint8_t)(1U << i);
        }
    }
    return mask;
}

static inline uint8_t kb_get_input_mask_alt_in01(void) {
#if CONFIG_TUTORIAL_0007_AUTODETECT_IN01
    // Alternate wiring hypothesis from vendor keyboard.h (some revisions): IN0/IN1 are GPIO1/GPIO2.
    const int in_pins[7] = {
        CONFIG_TUTORIAL_0007_KB_ALT_IN0_GPIO,
        CONFIG_TUTORIAL_0007_KB_ALT_IN1_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN6_GPIO,
    };
    uint8_t mask = 0;
    for (int i = 0; i < 7; i++) {
        int level = gpio_get_level((gpio_num_t)in_pins[i]);
        if (level == 0) {
            mask |= (uint8_t)(1U << i);
        }
    }
    return mask;
#else
    return 0;
#endif
}

typedef struct {
    int x; // 0..13
    int y; // 0..3
} key_pos_t;

static bool s_use_alt_in01 = false;

static int kb_scan_pressed(key_pos_t *out_keys, int out_cap) {
    int count = 0;

    for (int scan_state = 0; scan_state < 8; scan_state++) {
        kb_set_output((uint8_t)scan_state);
        if (CONFIG_TUTORIAL_0007_SCAN_SETTLE_US > 0) {
            esp_rom_delay_us((uint32_t)CONFIG_TUTORIAL_0007_SCAN_SETTLE_US);
        }

        uint8_t in_mask = 0;
        if (s_use_alt_in01) {
            in_mask = kb_get_input_mask_alt_in01();
        } else {
            in_mask = kb_get_input_mask();
        }

#if CONFIG_TUTORIAL_0007_AUTODETECT_IN01
        // If the primary wiring shows no activity, also check the alternate IN0/IN1 pins.
        // If we see activity there, switch permanently and log once.
        if (!s_use_alt_in01 && in_mask == 0) {
            uint8_t alt = kb_get_input_mask_alt_in01();
            if (alt != 0) {
                s_use_alt_in01 = true;
                in_mask = alt;
                ESP_LOGW(TAG, "keyboard autodetect: switching IN0/IN1 to alt pins [%d,%d] (was [%d,%d])",
                         CONFIG_TUTORIAL_0007_KB_ALT_IN0_GPIO,
                         CONFIG_TUTORIAL_0007_KB_ALT_IN1_GPIO,
                         CONFIG_TUTORIAL_0007_KB_IN0_GPIO,
                         CONFIG_TUTORIAL_0007_KB_IN1_GPIO);
            }
        }
#endif

#if CONFIG_TUTORIAL_0007_DEBUG_RAW_SCAN
        if (in_mask != 0) {
            ESP_LOGI(TAG, "scan_state=%d out=0x%x in_mask=0x%02x pinset=%s",
                     scan_state, scan_state, (unsigned)in_mask,
                     s_use_alt_in01 ? "alt_in01" : "primary");
        }
#endif
        if (in_mask == 0) {
            continue;
        }

        // For each asserted input bit, map to x/y based on vendor chart.
        for (int j = 0; j < 7; j++) {
            if ((in_mask & (1U << j)) == 0) {
                continue;
            }

            int x = (scan_state > 3) ? (2 * j) : (2 * j + 1);
            int y_base = (scan_state > 3) ? (scan_state - 4) : scan_state; // 0..3
            int y = (-y_base) + 3; // flip to match "picture"

            if (x < 0 || x > 13 || y < 0 || y > 3) {
                continue;
            }

            if (count < out_cap) {
                out_keys[count].x = x;
                out_keys[count].y = y;
                count++;
            }
        }
    }

    kb_set_output(0);
    return count;
}

static bool pos_is_pressed(const key_pos_t *keys, int n, int x, int y) {
    for (int i = 0; i < n; i++) {
        if (keys[i].x == x && keys[i].y == y) {
            return true;
        }
    }
    return false;
}

static void serial_write_str(const char *s) {
    if (s == NULL) {
        return;
    }
    fputs(s, stdout);
    fflush(stdout);
}

static void serial_write_ch(char c) {
    fputc((int)c, stdout);
    fflush(stdout);
}

static void keyboard_echo_task(void *arg) {
    (void)arg;

    // Make stdout unbuffered so characters appear "realtime" in the serial monitor.
    setvbuf(stdout, NULL, _IONBF, 0);

    // Configure GPIO.
    const int out_pins[3] = {
        CONFIG_TUTORIAL_0007_KB_OUT0_GPIO,
        CONFIG_TUTORIAL_0007_KB_OUT1_GPIO,
        CONFIG_TUTORIAL_0007_KB_OUT2_GPIO,
    };
    const int in_pins[7] = {
        CONFIG_TUTORIAL_0007_KB_IN0_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN1_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0007_KB_IN6_GPIO,
    };

    for (int i = 0; i < 3; i++) {
        gpio_reset_pin((gpio_num_t)out_pins[i]);
        gpio_set_direction((gpio_num_t)out_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)out_pins[i], GPIO_PULLUP_PULLDOWN);
        gpio_set_level((gpio_num_t)out_pins[i], 0);
    }
    for (int i = 0; i < 7; i++) {
        gpio_reset_pin((gpio_num_t)in_pins[i]);
        gpio_set_direction((gpio_num_t)in_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)in_pins[i], GPIO_PULLUP_ONLY);
    }

#if CONFIG_TUTORIAL_0007_AUTODETECT_IN01
    // Configure alt IN0/IN1 pins too, so we can autodetect at runtime.
    const int alt_in_pins[2] = {CONFIG_TUTORIAL_0007_KB_ALT_IN0_GPIO, CONFIG_TUTORIAL_0007_KB_ALT_IN1_GPIO};
    for (int i = 0; i < 2; i++) {
        gpio_reset_pin((gpio_num_t)alt_in_pins[i]);
        gpio_set_direction((gpio_num_t)alt_in_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)alt_in_pins[i], GPIO_PULLUP_ONLY);
    }
    ESP_LOGI(TAG, "keyboard autodetect enabled: primary IN0/IN1=[%d,%d], alt IN0/IN1=[%d,%d]",
             CONFIG_TUTORIAL_0007_KB_IN0_GPIO, CONFIG_TUTORIAL_0007_KB_IN1_GPIO,
             CONFIG_TUTORIAL_0007_KB_ALT_IN0_GPIO, CONFIG_TUTORIAL_0007_KB_ALT_IN1_GPIO);
#endif

    kb_set_output(0);

    ESP_LOGI(TAG, "keyboard scan pins: OUT=[%d,%d,%d] IN=[%d,%d,%d,%d,%d,%d,%d]",
             out_pins[0], out_pins[1], out_pins[2],
             in_pins[0], in_pins[1], in_pins[2], in_pins[3], in_pins[4], in_pins[5], in_pins[6]);
    ESP_LOGI(TAG, "type on the Cardputer keyboard; characters echo INLINE after the prompt (no newline until Enter).");
    ESP_LOGI(TAG, "tip: enable menuconfig -> \"Debug: log a line for every key press event\" for newline-per-key feedback.");
    serial_write_str("\r\n> ");

    bool prev_pressed[4][14] = {0};
    uint32_t last_emit_ms[4][14] = {0};

    char line_buf[256];
    size_t line_len = 0;

    while (1) {
        key_pos_t keys[24];
        int n = kb_scan_pressed(keys, (int)(sizeof(keys) / sizeof(keys[0])));

        bool shift = false;
        bool ctrl = false;
        bool alt = false;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 14; x++) {
                if (!pos_is_pressed(keys, n, x, y)) {
                    continue;
                }
                const char *label = s_key_map[y][x].first;
                if (label && strcmp(label, "shift") == 0) {
                    shift = true;
                } else if (label && strcmp(label, "ctrl") == 0) {
                    ctrl = true;
                } else if (label && strcmp(label, "alt") == 0) {
                    alt = true;
                }
            }
        }

        uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 14; x++) {
                bool pressed = pos_is_pressed(keys, n, x, y);
                bool was_pressed = prev_pressed[y][x];

                if (pressed && !was_pressed) {
                    uint32_t last = last_emit_ms[y][x];
                    if (CONFIG_TUTORIAL_0007_DEBOUNCE_MS > 0 && (now_ms - last) < (uint32_t)CONFIG_TUTORIAL_0007_DEBOUNCE_MS) {
                        // Ignore bouncy edges.
                    } else {
                        last_emit_ms[y][x] = now_ms;

                        const char *s = shift ? s_key_map[y][x].second : s_key_map[y][x].first;
                        if (s == NULL) {
                            // N/A
                        } else if (strcmp(s, "shift") == 0 || strcmp(s, "ctrl") == 0 || strcmp(s, "alt") == 0 ||
                                   strcmp(s, "fn") == 0 || strcmp(s, "opt") == 0) {
                            // Modifiers: ignore on press (their effect is handled via shift flag).
                        } else if (strcmp(s, "space") == 0) {
                            if (line_len < sizeof(line_buf) - 1) {
                                line_buf[line_len++] = ' ';
                            }
                            serial_write_ch(' ');
                        } else if (strcmp(s, "tab") == 0) {
                            if (line_len < sizeof(line_buf) - 1) {
                                line_buf[line_len++] = '\t';
                            }
                            serial_write_ch('\t');
                        } else if (strcmp(s, "enter") == 0) {
                            serial_write_str("\r\n");
                            line_buf[line_len] = '\0';
                            ESP_LOGI(TAG, "line (%s%s): \"%s\"",
                                     ctrl ? "ctrl+" : "",
                                     alt ? "alt+" : "",
                                     line_buf);
                            line_len = 0;
                            serial_write_str("> ");
                        } else if (strcmp(s, "del") == 0) {
                            if (line_len > 0) {
                                line_len--;
                                // Backspace on terminal: move left, erase, move left.
                                serial_write_str("\b \b");
                            }
                        } else if (strlen(s) == 1) {
                            char c = s[0];
                            if (line_len < sizeof(line_buf) - 1) {
                                line_buf[line_len++] = c;
                            }
                            serial_write_ch(c);
                        } else {
                            // Unknown/non-printable token.
                            ESP_LOGI(TAG, "key pressed @(%d,%d): %s (ctrl=%d alt=%d shift=%d)", x, y, s, ctrl, alt, shift);
                        }

#if CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS
                        // Extra, unmistakable feedback (one log line per key event).
                        if (s != NULL) {
                            ESP_LOGI(TAG, "key event @(%d,%d): %s (ctrl=%d alt=%d shift=%d len=%u)",
                                     x, y, s, ctrl, alt, shift, (unsigned)line_len);
                        }
#endif
                    }
                }

                prev_pressed[y][x] = pressed;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0007_SCAN_PERIOD_MS));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    xTaskCreate(keyboard_echo_task, "keyboard_echo", 4096, NULL, 10, NULL);
}
