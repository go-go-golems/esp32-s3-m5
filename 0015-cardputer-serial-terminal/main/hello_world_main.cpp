/*
 * ESP32-S3 tutorial 0015:
 * Cardputer serial terminal: keyboard input -> on-screen text UI, plus TX to a configurable backend:
 * - USB-Serial-JTAG (ttyACM*), or
 * - hardware UART (configurable UART num + TX/RX pins + baud).
 *
 * This chapter intentionally reuses:
 * - keyboard scan + mapping logic from tutorial 0007 / 0012 (4x14 picture coordinates)
 * - M5GFX autodetect bring-up for Cardputer wiring
 * - M5Canvas full-screen redraw-on-dirty pattern (stable, easy to reason about)
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <deque>
#include <string>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_system.h"

#include "M5GFX.h"

static const char *TAG = "cardputer_serial_terminal_0015";

// Backwards compatibility: if the project was configured before Kconfig options existed,
// the CONFIG_ macros may be missing until you run `idf.py reconfigure` or `idf.py fullclean`.
#ifndef CONFIG_TUTORIAL_0015_PRESENT_WAIT_DMA
#define CONFIG_TUTORIAL_0015_PRESENT_WAIT_DMA 1
#endif
#ifndef CONFIG_TUTORIAL_0015_CANVAS_USE_PSRAM
#define CONFIG_TUTORIAL_0015_CANVAS_USE_PSRAM 0
#endif
#ifndef CONFIG_TUTORIAL_0015_LOG_EVERY_N_EVENTS
#define CONFIG_TUTORIAL_0015_LOG_EVERY_N_EVENTS 0
#endif
#ifndef CONFIG_TUTORIAL_0015_MAX_LINES
#define CONFIG_TUTORIAL_0015_MAX_LINES 256
#endif

// Serial backend defaults (choice)
#if !defined(CONFIG_TUTORIAL_0015_BACKEND_USB_SERIAL_JTAG) && !defined(CONFIG_TUTORIAL_0015_BACKEND_UART)
#define CONFIG_TUTORIAL_0015_BACKEND_USB_SERIAL_JTAG 1
#endif
#ifndef CONFIG_TUTORIAL_0015_UART_NUM
#define CONFIG_TUTORIAL_0015_UART_NUM 1
#endif
#ifndef CONFIG_TUTORIAL_0015_UART_BAUD
#define CONFIG_TUTORIAL_0015_UART_BAUD 115200
#endif
#ifndef CONFIG_TUTORIAL_0015_UART_TX_GPIO
#define CONFIG_TUTORIAL_0015_UART_TX_GPIO 43
#endif
#ifndef CONFIG_TUTORIAL_0015_UART_RX_GPIO
#define CONFIG_TUTORIAL_0015_UART_RX_GPIO 44
#endif
#ifndef CONFIG_TUTORIAL_0015_LOCAL_ECHO
#define CONFIG_TUTORIAL_0015_LOCAL_ECHO 1
#endif
#ifndef CONFIG_TUTORIAL_0015_SHOW_RX
#define CONFIG_TUTORIAL_0015_SHOW_RX 1
#endif
#ifndef CONFIG_TUTORIAL_0015_NEWLINE_CRLF
#define CONFIG_TUTORIAL_0015_NEWLINE_CRLF 1
#endif

// Backspace byte selection (choice)
#if !defined(CONFIG_TUTORIAL_0015_BS_SEND_BS) && !defined(CONFIG_TUTORIAL_0015_BS_SEND_DEL)
#define CONFIG_TUTORIAL_0015_BS_SEND_DEL 1
#endif

// Keyboard scan defaults
#ifndef CONFIG_TUTORIAL_0015_SCAN_PERIOD_MS
#define CONFIG_TUTORIAL_0015_SCAN_PERIOD_MS 10
#endif
#ifndef CONFIG_TUTORIAL_0015_DEBOUNCE_MS
#define CONFIG_TUTORIAL_0015_DEBOUNCE_MS 30
#endif
#ifndef CONFIG_TUTORIAL_0015_SCAN_SETTLE_US
#define CONFIG_TUTORIAL_0015_SCAN_SETTLE_US 0
#endif
#ifndef CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO
#define CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO 1
#endif
#ifndef CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO
#define CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO 2
#endif
#ifndef CONFIG_TUTORIAL_0015_LOG_KEY_EVENTS
#define CONFIG_TUTORIAL_0015_LOG_KEY_EVENTS 0
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
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0015_KB_OUT0_GPIO, (out_bits3 & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0015_KB_OUT1_GPIO, (out_bits3 & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0015_KB_OUT2_GPIO, (out_bits3 & 0x04) ? 1 : 0);
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
static bool s_allow_autodetect_in01 = true;

static int kb_scan_pressed(key_pos_t *out_keys, int out_cap, const int in_pins_primary[7], const int in_pins_alt[7]) {
    int count = 0;

    for (int scan_state = 0; scan_state < 8; scan_state++) {
        kb_set_output((uint8_t)scan_state);
        if (CONFIG_TUTORIAL_0015_SCAN_SETTLE_US > 0) {
            esp_rom_delay_us((uint32_t)CONFIG_TUTORIAL_0015_SCAN_SETTLE_US);
        }

        uint8_t in_mask = 0;
        if (s_use_alt_in01) {
            in_mask = kb_get_input_mask(in_pins_alt);
        } else {
            in_mask = kb_get_input_mask(in_pins_primary);
        }

#if CONFIG_TUTORIAL_0015_AUTODETECT_IN01
        if (s_allow_autodetect_in01) {
            // If the primary wiring shows no activity, also check the alternate IN0/IN1 pins.
            // If we see activity there, switch permanently and log once.
            if (!s_use_alt_in01 && in_mask == 0) {
                uint8_t alt = kb_get_input_mask(in_pins_alt);
                if (alt != 0) {
                    s_use_alt_in01 = true;
                    in_mask = alt;
                    ESP_LOGW(TAG, "keyboard autodetect: switching IN0/IN1 to alt pins [%d,%d] (was [%d,%d])",
                             CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO,
                             CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO,
                             CONFIG_TUTORIAL_0015_KB_IN0_GPIO,
                             CONFIG_TUTORIAL_0015_KB_IN1_GPIO);
                }
            }
        }
#endif

#if CONFIG_TUTORIAL_0015_DEBUG_RAW_SCAN
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

static void buf_trim(std::deque<std::string> &lines, size_t max_lines, size_t &dropped_lines) {
    while (lines.size() > max_lines) {
        lines.pop_front();
        dropped_lines++;
    }
    if (lines.empty()) {
        lines.push_back(std::string());
    }
}

static void buf_insert_char(std::deque<std::string> &lines, char c) {
    if (lines.empty()) {
        lines.push_back(std::string());
    }
    lines.back().push_back(c);
}

static void buf_backspace(std::deque<std::string> &lines) {
    if (lines.empty()) {
        lines.push_back(std::string());
        return;
    }
    if (!lines.back().empty()) {
        lines.back().pop_back();
        return;
    }
    // At column 0: join with previous line by removing this (empty) line.
    if (lines.size() > 1) {
        lines.pop_back();
    }
}

static void buf_newline(std::deque<std::string> &lines, size_t max_lines, size_t &dropped_lines) {
    if (lines.empty()) {
        lines.push_back(std::string());
    }
    lines.push_back(std::string());
    buf_trim(lines, max_lines, dropped_lines);
}

typedef struct {
    bool pending_cr;
} term_rx_state_t;

static void term_apply_byte(std::deque<std::string> &lines, size_t max_lines, size_t &dropped_lines, term_rx_state_t &st, uint8_t b) {
    // Normalize CRLF: treat \r as newline and skip an immediately following \n.
    if (b == '\r') {
        st.pending_cr = true;
        buf_newline(lines, max_lines, dropped_lines);
        return;
    }
    if (b == '\n') {
        if (st.pending_cr) {
            st.pending_cr = false;
            return;
        }
        buf_newline(lines, max_lines, dropped_lines);
        return;
    }
    st.pending_cr = false;

    if (b == 0x08 || b == 0x7f) {
        buf_backspace(lines);
        return;
    }
    if (b == '\t') {
        // MVP: tab as 4 spaces for rendering.
        buf_insert_char(lines, ' ');
        buf_insert_char(lines, ' ');
        buf_insert_char(lines, ' ');
        buf_insert_char(lines, ' ');
        return;
    }
    if (b >= 32 && b <= 126) {
        buf_insert_char(lines, (char)b);
        return;
    }

    // Non-printable: ignore for MVP.
}

typedef enum {
    BACKEND_USB_SERIAL_JTAG = 0,
    BACKEND_UART = 1,
} backend_kind_t;

typedef struct {
    backend_kind_t kind;
    uart_port_t uart_num;
    bool usb_driver_ok;
} serial_backend_t;

static void backend_init(serial_backend_t &b) {
#if CONFIG_TUTORIAL_0015_BACKEND_UART
    b.kind = BACKEND_UART;
    b.uart_num = (uart_port_t)CONFIG_TUTORIAL_0015_UART_NUM;

    uart_config_t cfg = {};
    cfg.baud_rate = CONFIG_TUTORIAL_0015_UART_BAUD;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    const int rx_buf = 1024;
    const int tx_buf = 0;
    ESP_ERROR_CHECK(uart_driver_install(b.uart_num, rx_buf, tx_buf, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(b.uart_num, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(b.uart_num,
                                CONFIG_TUTORIAL_0015_UART_TX_GPIO,
                                CONFIG_TUTORIAL_0015_UART_RX_GPIO,
                                UART_PIN_NO_CHANGE,
                                UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "serial backend: UART%u baud=%d tx=%d rx=%d",
             (unsigned)b.uart_num,
             CONFIG_TUTORIAL_0015_UART_BAUD,
             CONFIG_TUTORIAL_0015_UART_TX_GPIO,
             CONFIG_TUTORIAL_0015_UART_RX_GPIO);
#else
    b.kind = BACKEND_USB_SERIAL_JTAG;
    b.uart_num = UART_NUM_0;
    b.usb_driver_ok = usb_serial_jtag_is_driver_installed();
    if (!b.usb_driver_ok) {
        ESP_LOGW(TAG, "USB-Serial-JTAG driver not installed (console may be disabled); writes/reads may be no-op");
    }
    ESP_LOGI(TAG, "serial backend: USB-Serial-JTAG (installed=%d)", (int)b.usb_driver_ok);
#endif
}

static int backend_write(serial_backend_t &b, const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        return 0;
    }
    if (b.kind == BACKEND_UART) {
        return uart_write_bytes(b.uart_num, (const char *)data, (size_t)len);
    }
    if (!usb_serial_jtag_is_driver_installed()) {
        return 0;
    }
    return (int)usb_serial_jtag_write_bytes((const void *)data, len, 0);
}

static int backend_read(serial_backend_t &b, uint8_t *data, size_t max_len) {
    if (!data || max_len == 0) {
        return 0;
    }
    if (b.kind == BACKEND_UART) {
        int n = uart_read_bytes(b.uart_num, data, (uint32_t)max_len, 0);
        return n < 0 ? 0 : n;
    }
    if (!usb_serial_jtag_is_driver_installed()) {
        return 0;
    }
    return (int)usb_serial_jtag_read_bytes((void *)data, max_len, 0);
}

static size_t token_to_tx_bytes(const char *tok, uint8_t out[8]) {
    if (!tok || !out) {
        return 0;
    }
    if (strcmp(tok, "space") == 0) {
        out[0] = (uint8_t)' ';
        return 1;
    }
    if (strcmp(tok, "tab") == 0) {
        out[0] = (uint8_t)'\t';
        return 1;
    }
    if (strcmp(tok, "enter") == 0) {
#if CONFIG_TUTORIAL_0015_NEWLINE_CRLF
        out[0] = (uint8_t)'\r';
        out[1] = (uint8_t)'\n';
        return 2;
#else
        out[0] = (uint8_t)'\n';
        return 1;
#endif
    }
    if (strcmp(tok, "del") == 0) {
#if CONFIG_TUTORIAL_0015_BS_SEND_BS
        out[0] = 0x08;
#else
        out[0] = 0x7f;
#endif
        return 1;
    }
    if (strlen(tok) == 1) {
        out[0] = (uint8_t)tok[0];
        return 1;
    }
    return 0;
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    // Display + canvas bring-up (vendor autodetect).
    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "bringing up M5GFX display via autodetect...");
    display.init();
    display.setColorDepth(16);
    ESP_LOGI(TAG, "display ready: width=%d height=%d", (int)display.width(), (int)display.height());

    const int w = (int)display.width();
    const int h = (int)display.height();

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0015_CANVAS_USE_PSRAM
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

    // Configure keyboard GPIO.
    const int out_pins[3] = {
        CONFIG_TUTORIAL_0015_KB_OUT0_GPIO,
        CONFIG_TUTORIAL_0015_KB_OUT1_GPIO,
        CONFIG_TUTORIAL_0015_KB_OUT2_GPIO,
    };
    const int in_pins_primary[7] = {
        CONFIG_TUTORIAL_0015_KB_IN0_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN1_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN6_GPIO,
    };
    const int in_pins_alt[7] = {
        CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO,
        CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN2_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN3_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN4_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN5_GPIO,
        CONFIG_TUTORIAL_0015_KB_IN6_GPIO,
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
#if CONFIG_TUTORIAL_0015_AUTODETECT_IN01
    // If GROVE UART uses GPIO1/GPIO2, it conflicts with the keyboard "alt IN0/IN1" autodetect path.
    // Detect this configuration and disable autodetect at runtime so UART can own the pins.
    s_allow_autodetect_in01 = true;
#if CONFIG_TUTORIAL_0015_BACKEND_UART
    if (CONFIG_TUTORIAL_0015_UART_RX_GPIO == CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO ||
        CONFIG_TUTORIAL_0015_UART_RX_GPIO == CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO ||
        CONFIG_TUTORIAL_0015_UART_TX_GPIO == CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO ||
        CONFIG_TUTORIAL_0015_UART_TX_GPIO == CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO) {
        s_allow_autodetect_in01 = false;
        ESP_LOGW(TAG, "keyboard autodetect disabled: UART pins (%d/%d) conflict with alt IN0/IN1 (%d/%d)",
                 CONFIG_TUTORIAL_0015_UART_RX_GPIO,
                 CONFIG_TUTORIAL_0015_UART_TX_GPIO,
                 CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO,
                 CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO);
    }
#endif

    // Configure alt IN0/IN1 pins too, so we can autodetect at runtime.
    if (s_allow_autodetect_in01) {
        const int alt_in_pins_2[2] = {CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO, CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO};
        for (int i = 0; i < 2; i++) {
            gpio_reset_pin((gpio_num_t)alt_in_pins_2[i]);
            gpio_set_direction((gpio_num_t)alt_in_pins_2[i], GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)alt_in_pins_2[i], GPIO_PULLUP_ONLY);
        }
    }
    ESP_LOGI(TAG, "keyboard autodetect enabled: primary IN0/IN1=[%d,%d], alt IN0/IN1=[%d,%d]",
             CONFIG_TUTORIAL_0015_KB_IN0_GPIO, CONFIG_TUTORIAL_0015_KB_IN1_GPIO,
             CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO, CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO);
#endif
    kb_set_output(0);

    ESP_LOGI(TAG, "keyboard scan pins: OUT=[%d,%d,%d] IN=[%d,%d,%d,%d,%d,%d,%d]",
             out_pins[0], out_pins[1], out_pins[2],
             in_pins_primary[0], in_pins_primary[1], in_pins_primary[2], in_pins_primary[3], in_pins_primary[4], in_pins_primary[5], in_pins_primary[6]);

    // Serial backend.
    serial_backend_t backend = {};
    backend_init(backend);

    // Terminal state.
    std::deque<std::string> lines;
    lines.push_back(std::string());
    size_t dropped_lines = 0;
    uint32_t edit_events = 0;
    term_rx_state_t rx_state = {.pending_cr = false};

    bool prev_pressed[4][14] = {0};
    uint32_t last_emit_ms[4][14] = {0};

    char last_token[32] = "(none)";
    int last_x = -1;
    int last_y = -1;
    bool dirty = true;

    ESP_LOGI(TAG, "ready: local_echo=%d show_rx=%d newline_crlf=%d",
             (int)CONFIG_TUTORIAL_0015_LOCAL_ECHO,
             (int)CONFIG_TUTORIAL_0015_SHOW_RX,
             (int)CONFIG_TUTORIAL_0015_NEWLINE_CRLF);

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
                    if (CONFIG_TUTORIAL_0015_DEBOUNCE_MS > 0 && (now_ms - last) < (uint32_t)CONFIG_TUTORIAL_0015_DEBOUNCE_MS) {
                        // Ignore bouncy edges.
                    } else {
                        last_emit_ms[y][x] = now_ms;

                        const char *tok = shift ? s_key_map[y][x].second : s_key_map[y][x].first;
                        if (tok != NULL) {
                            // Modifiers: ignore as tokens (their effect is handled via shift flag).
                            if (strcmp(tok, "shift") == 0 || strcmp(tok, "ctrl") == 0 || strcmp(tok, "alt") == 0 ||
                                strcmp(tok, "fn") == 0 || strcmp(tok, "opt") == 0) {
                                // N/A
                            } else {
                                (void)snprintf(last_token, sizeof(last_token), "%s", tok);
                                last_x = x;
                                last_y = y;

                                uint8_t tx[8] = {0};
                                size_t tx_n = token_to_tx_bytes(tok, tx);
                                if (tx_n > 0) {
                                    backend_write(backend, tx, tx_n);
                                    if (CONFIG_TUTORIAL_0015_LOCAL_ECHO) {
                                        for (size_t i = 0; i < tx_n; i++) {
                                            term_apply_byte(lines, (size_t)CONFIG_TUTORIAL_0015_MAX_LINES, dropped_lines, rx_state, tx[i]);
                                        }
                                        edit_events++;
                                        dirty = true;
                                    }
                                }
                            }
                        }

#if CONFIG_TUTORIAL_0015_LOG_KEY_EVENTS
                        if (tok != NULL) {
                            ESP_LOGI(TAG, "key event @(%d,%d): %s (ctrl=%d alt=%d shift=%d)",
                                     x, y, tok, ctrl, alt, shift);
                        }
#else
                        if (tok != NULL && (strcmp(tok, "shift") != 0 && strcmp(tok, "ctrl") != 0 && strcmp(tok, "alt") != 0)) {
                            ESP_LOGI(TAG, "key @(%d,%d): %s (ctrl=%d alt=%d shift=%d)", x, y, tok, ctrl, alt, shift);
                        }
#endif
                    }
                }

                prev_pressed[y][x] = pressed;
            }
        }

#if CONFIG_TUTORIAL_0015_SHOW_RX
        // Non-blocking receive: pull a small chunk each tick, apply to screen buffer.
        while (true) {
            uint8_t rx_buf[64];
            int rn = backend_read(backend, rx_buf, sizeof(rx_buf));
            if (rn <= 0) {
                break;
            }
            for (int i = 0; i < rn; i++) {
                term_apply_byte(lines, (size_t)CONFIG_TUTORIAL_0015_MAX_LINES, dropped_lines, rx_state, rx_buf[i]);
            }
            edit_events++;
            dirty = true;
        }
#endif

#if CONFIG_TUTORIAL_0015_LOG_EVERY_N_EVENTS > 0
        if (edit_events > 0 && (edit_events % (uint32_t)CONFIG_TUTORIAL_0015_LOG_EVERY_N_EVENTS) == 0) {
            ESP_LOGI(TAG, "edit heartbeat: events=%" PRIu32 " lines=%u dropped=%u free_heap=%" PRIu32,
                     edit_events, (unsigned)lines.size(), (unsigned)dropped_lines, esp_get_free_heap_size());
        }
#endif

        if (dirty) {
            dirty = false;
            canvas.fillScreen(TFT_BLACK);
            canvas.setTextColor(TFT_WHITE, TFT_BLACK);
            canvas.setTextWrap(false);
            canvas.setCursor(0, 0);

            canvas.printf("Cardputer Serial Terminal (0015)\n");
#if CONFIG_TUTORIAL_0015_BACKEND_UART
            canvas.printf("Backend: UART%u %d baud\n", (unsigned)backend.uart_num, CONFIG_TUTORIAL_0015_UART_BAUD);
#else
            canvas.printf("Backend: USB-Serial-JTAG\n");
#endif
            canvas.printf("Last: %s @(%d,%d) shift=%d ctrl=%d alt=%d\n",
                          last_token, last_x, last_y, (int)shift, (int)ctrl, (int)alt);
            canvas.printf("Lines: %u (dropped=%u)  Events: %" PRIu32 "\n",
                          (unsigned)lines.size(), (unsigned)dropped_lines, edit_events);

            const int line_h = canvas.fontHeight();
            const int header_rows = 4;
            const int max_rows = (line_h > 0) ? (canvas.height() / line_h) : 0;
            int text_rows = max_rows - header_rows;
            if (text_rows < 1) {
                text_rows = 1;
            }

            const size_t visible = (lines.size() > (size_t)text_rows) ? (size_t)text_rows : lines.size();
            const size_t start = (lines.size() > visible) ? (lines.size() - visible) : 0;

            int y0 = header_rows * line_h;
            int y_cursor = y0;
            for (size_t i = start; i < lines.size(); i++) {
                canvas.setCursor(0, y0);
                canvas.print(lines[i].c_str());
                y_cursor = y0;
                y0 += line_h;
            }

            // Cursor UX (MVP): draw underscore at end of the last line.
            if (!lines.empty() && line_h > 0) {
                int x_cursor = canvas.textWidth(lines.back().c_str());
                if (x_cursor < 0) {
                    x_cursor = 0;
                }
                if (x_cursor > canvas.width() - 2) {
                    x_cursor = canvas.width() - 2;
                }
                int cursor_w = canvas.textWidth("_");
                if (cursor_w <= 0) {
                    cursor_w = 6;
                }
                int y_underline = y_cursor + line_h - 2;
                if (y_underline < 0) {
                    y_underline = 0;
                }
                if (y_underline > canvas.height() - 2) {
                    y_underline = canvas.height() - 2;
                }
                canvas.fillRect(x_cursor, y_underline, cursor_w, 2, TFT_WHITE);
            }

            canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0015_PRESENT_WAIT_DMA
            display.waitDMA();
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0015_SCAN_PERIOD_MS));
    }
}


