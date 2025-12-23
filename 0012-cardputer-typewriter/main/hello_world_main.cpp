/*
 * ESP32-S3 tutorial 0012:
 * Cardputer "typewriter": keyboard input -> on-screen text UI using M5GFX (LovyanGFX).
 *
 * This chapter intentionally uses:
 * - M5GFX autodetect for Cardputer wiring (ST7789 on SPI3, offsets, rotation, backlight PWM)
 * - a sprite/canvas backbuffer for full-screen redraws
 * - an optional waitDMA() after presents to avoid tearing from overlapping transfers
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_system.h"

#include "M5GFX.h"

static const char *TAG = "cardputer_typewriter_0012";

// Backwards compatibility: if the project was configured before Kconfig options existed,
// the CONFIG_ macros may be missing until you run `idf.py reconfigure` or `idf.py fullclean`.
#ifndef CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA
#define CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA 1
#endif
#ifndef CONFIG_TUTORIAL_0012_CANVAS_USE_PSRAM
#define CONFIG_TUTORIAL_0012_CANVAS_USE_PSRAM 0
#endif
#ifndef CONFIG_TUTORIAL_0012_SCAN_PERIOD_MS
#define CONFIG_TUTORIAL_0012_SCAN_PERIOD_MS 10
#endif
#ifndef CONFIG_TUTORIAL_0012_DEBOUNCE_MS
#define CONFIG_TUTORIAL_0012_DEBOUNCE_MS 30
#endif
#ifndef CONFIG_TUTORIAL_0012_SCAN_SETTLE_US
#define CONFIG_TUTORIAL_0012_SCAN_SETTLE_US 0
#endif
#ifndef CONFIG_TUTORIAL_0012_KB_ALT_IN0_GPIO
#define CONFIG_TUTORIAL_0012_KB_ALT_IN0_GPIO 1
#endif
#ifndef CONFIG_TUTORIAL_0012_KB_ALT_IN1_GPIO
#define CONFIG_TUTORIAL_0012_KB_ALT_IN1_GPIO 2
#endif
#ifndef CONFIG_TUTORIAL_0012_LOG_KEY_EVENTS
#define CONFIG_TUTORIAL_0012_LOG_KEY_EVENTS 0
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
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0012_KB_OUT0_GPIO, (out_bits3 & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0012_KB_OUT1_GPIO, (out_bits3 & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0012_KB_OUT2_GPIO, (out_bits3 & 0x04) ? 1 : 0);
}

static inline uint8_t kb_get_input_mask(const int in_pins[7]) {
    // Input pins are pulled up; a pressed key reads low -> we invert to build a 7-bit pressed mask.
    uint8_t mask = 0;
    for (int i = 0; i < 7; i++) {
        int level = gpio_get_level((gpio_num_t)in_pins[i]);
        if (level == 0) {
            mask |= (uint8_t)(1U << i);
        }
    }
    return mask;
}

typedef struct {
    int x; // 0..13
    int y; // 0..3
} key_pos_t;

static bool s_use_alt_in01 = false;

static int kb_scan_pressed(key_pos_t *out_keys, int out_cap, const int in_pins_primary[7], const int in_pins_alt[7]) {
    int count = 0;

    for (int scan_state = 0; scan_state < 8; scan_state++) {
        kb_set_output((uint8_t)scan_state);
        if (CONFIG_TUTORIAL_0012_SCAN_SETTLE_US > 0) {
            esp_rom_delay_us((uint32_t)CONFIG_TUTORIAL_0012_SCAN_SETTLE_US);
        }

        uint8_t in_mask = 0;
        if (s_use_alt_in01) {
            in_mask = kb_get_input_mask(in_pins_alt);
        } else {
            in_mask = kb_get_input_mask(in_pins_primary);
        }

#if CONFIG_TUTORIAL_0012_AUTODETECT_IN01
        // If the primary wiring shows no activity, also check the alternate IN0/IN1 pins.
        // If we see activity there, switch permanently and log once.
        if (!s_use_alt_in01 && in_mask == 0) {
            uint8_t alt = kb_get_input_mask(in_pins_alt);
            if (alt != 0) {
                s_use_alt_in01 = true;
                in_mask = alt;
                ESP_LOGW(TAG, "keyboard autodetect: switching IN0/IN1 to alt pins [%d,%d] (was [%d,%d])",
                         CONFIG_TUTORIAL_0012_KB_ALT_IN0_GPIO,
                         CONFIG_TUTORIAL_0012_KB_ALT_IN1_GPIO,
                         CONFIG_TUTORIAL_0012_KB_IN0_GPIO,
                         CONFIG_TUTORIAL_0012_KB_IN1_GPIO);
            }
        }
#endif

#if CONFIG_TUTORIAL_0012_DEBUG_RAW_SCAN
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

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "bringing up M5GFX display via autodetect...");
    display.init();
    display.setColorDepth(16);

    ESP_LOGI(TAG, "display ready: width=%d height=%d", (int)display.width(), (int)display.height());

    const int w = (int)display.width();
    const int h = (int)display.height();

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0012_CANVAS_USE_PSRAM
    canvas.setPsram(true);
#else
    canvas.setPsram(false);
#endif
    canvas.setColorDepth(16);
    void *buf = canvas.createSprite(w, h);
    if (!buf) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        return;
    }

    ESP_LOGI(TAG, "canvas ok: %u bytes; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             (unsigned)canvas.bufferLength(),
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setCursor(0, 0);
    canvas.printf("Cardputer Typewriter (0012)\n\n");
    canvas.printf("Keyboard scan: bringing up...\n");
    canvas.printf("Refs: 0007 (keyboard), 0011 (display)\n");
    canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA
    display.waitDMA();
#endif

    // Configure GPIO (keyboard scan).
    const int out_pins[3] = {
        CONFIG_TUTORIAL_0012_KB_OUT0_GPIO,
        CONFIG_TUTORIAL_0012_KB_OUT1_GPIO,
        CONFIG_TUTORIAL_0012_KB_OUT2_GPIO,
    };
    const int in_pins_primary[7] = {
        CONFIG_TUTORIAL_0012_KB_IN0_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN1_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN6_GPIO,
    };
    const int in_pins_alt[7] = {
        CONFIG_TUTORIAL_0012_KB_ALT_IN0_GPIO,
        CONFIG_TUTORIAL_0012_KB_ALT_IN1_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0012_KB_IN6_GPIO,
    };

    for (int i = 0; i < 3; i++) {
        gpio_reset_pin((gpio_num_t)out_pins[i]);
        gpio_set_direction((gpio_num_t)out_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)out_pins[i], GPIO_PULLUP_PULLDOWN);
        gpio_set_level((gpio_num_t)out_pins[i], 0);
    }
    for (int i = 0; i < 7; i++) {
        gpio_reset_pin((gpio_num_t)in_pins_primary[i]);
        gpio_set_direction((gpio_num_t)in_pins_primary[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)in_pins_primary[i], GPIO_PULLUP_ONLY);
    }
#if CONFIG_TUTORIAL_0012_AUTODETECT_IN01
    // Configure alt IN0/IN1 pins too, so we can autodetect at runtime.
    const int alt_in_pins_2[2] = {CONFIG_TUTORIAL_0012_KB_ALT_IN0_GPIO, CONFIG_TUTORIAL_0012_KB_ALT_IN1_GPIO};
    for (int i = 0; i < 2; i++) {
        gpio_reset_pin((gpio_num_t)alt_in_pins_2[i]);
        gpio_set_direction((gpio_num_t)alt_in_pins_2[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)alt_in_pins_2[i], GPIO_PULLUP_ONLY);
    }
    ESP_LOGI(TAG, "keyboard autodetect enabled: primary IN0/IN1=[%d,%d], alt IN0/IN1=[%d,%d]",
             CONFIG_TUTORIAL_0012_KB_IN0_GPIO, CONFIG_TUTORIAL_0012_KB_IN1_GPIO,
             CONFIG_TUTORIAL_0012_KB_ALT_IN0_GPIO, CONFIG_TUTORIAL_0012_KB_ALT_IN1_GPIO);
#endif

    kb_set_output(0);

    ESP_LOGI(TAG, "keyboard scan pins: OUT=[%d,%d,%d] IN=[%d,%d,%d,%d,%d,%d,%d]",
             out_pins[0], out_pins[1], out_pins[2],
             in_pins_primary[0], in_pins_primary[1], in_pins_primary[2], in_pins_primary[3], in_pins_primary[4], in_pins_primary[5], in_pins_primary[6]);
    ESP_LOGI(TAG, "press keys on the Cardputer keyboard; we'll log key events and show the last token on-screen.");

    bool prev_pressed[4][14] = {0};
    uint32_t last_emit_ms[4][14] = {0};

    char last_token[32] = "(none)";
    int last_x = -1;
    int last_y = -1;
    bool dirty = true;

    while (true) {
        key_pos_t keys[24];
        int n = kb_scan_pressed(keys, (int)(sizeof(keys) / sizeof(keys[0])), in_pins_primary, in_pins_alt);

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
                    if (CONFIG_TUTORIAL_0012_DEBOUNCE_MS > 0 && (now_ms - last) < (uint32_t)CONFIG_TUTORIAL_0012_DEBOUNCE_MS) {
                        // Ignore bouncy edges.
                    } else {
                        last_emit_ms[y][x] = now_ms;

                        const char *s = shift ? s_key_map[y][x].second : s_key_map[y][x].first;
                        if (s != NULL) {
                            // Modifiers: ignore as tokens (their effect is handled via shift flag).
                            if (strcmp(s, "shift") == 0 || strcmp(s, "ctrl") == 0 || strcmp(s, "alt") == 0 ||
                                strcmp(s, "fn") == 0 || strcmp(s, "opt") == 0) {
                                // N/A
                            } else {
                                (void)snprintf(last_token, sizeof(last_token), "%s", s);
                                last_x = x;
                                last_y = y;
                                dirty = true;
                            }
                        }

#if CONFIG_TUTORIAL_0012_LOG_KEY_EVENTS
                        if (s != NULL) {
                            ESP_LOGI(TAG, "key event @(%d,%d): %s (ctrl=%d alt=%d shift=%d)",
                                     x, y, s, ctrl, alt, shift);
                        }
#else
                        if (s != NULL && (strcmp(s, "shift") != 0 && strcmp(s, "ctrl") != 0 && strcmp(s, "alt") != 0)) {
                            ESP_LOGI(TAG, "key @(%d,%d): %s (ctrl=%d alt=%d shift=%d)", x, y, s, ctrl, alt, shift);
                        }
#endif
                    }
                }

                prev_pressed[y][x] = pressed;
            }
        }

        if (dirty) {
            dirty = false;
            canvas.fillScreen(TFT_BLACK);
            canvas.setTextColor(TFT_WHITE, TFT_BLACK);
            canvas.setCursor(0, 0);
            canvas.printf("Cardputer Typewriter (0012)\n\n");
            canvas.printf("Last token: %s\n", last_token);
            canvas.printf("Pos: (%d,%d) mods: shift=%d ctrl=%d alt=%d\n",
                          last_x, last_y, (int)shift, (int)ctrl, (int)alt);
            canvas.printf("\nNext: route tokens into a text buffer + render\n");
            canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA
            display.waitDMA();
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0012_SCAN_PERIOD_MS));
    }
}



