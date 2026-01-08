/*
 * ESP32-S3 tutorial 0038:
 * Cardputer-ADV serial terminal: ADV keyboard (TCA8418 over I2C) -> on-screen text UI,
 * plus TX/RX to a configurable backend (USB-Serial-JTAG or UART).
 *
 * Copied/adapted from:
 * - 0015-cardputer-serial-terminal (terminal buffer + rendering + serial backend)
 * - 0036-cardputer-adv-led-matrix-console (TCA8418 event handling + key mapping)
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <deque>
#include <string>
#include <string_view>
#include <vector>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include <M5Unified.hpp>

#include "app_state.h"
#include "console_repl.h"
#include "fan_cmd.h"

static const char *TAG = "cardputer_adv_serial_terminal_0038";
static const char *KBD_TAG = "adv_kbd";

// Backwards compatibility: if the project was configured before Kconfig options existed,
// the CONFIG_ macros may be missing until you run `idf.py reconfigure` or `idf.py fullclean`.
#ifndef CONFIG_TUTORIAL_0038_PRESENT_WAIT_DMA
#define CONFIG_TUTORIAL_0038_PRESENT_WAIT_DMA 1
#endif
#ifndef CONFIG_TUTORIAL_0038_CANVAS_USE_PSRAM
#define CONFIG_TUTORIAL_0038_CANVAS_USE_PSRAM 0
#endif
#ifndef CONFIG_TUTORIAL_0038_MAX_LINES
#define CONFIG_TUTORIAL_0038_MAX_LINES 256
#endif

// Serial backend defaults (choice)
#if !defined(CONFIG_TUTORIAL_0038_BACKEND_USB_SERIAL_JTAG) && !defined(CONFIG_TUTORIAL_0038_BACKEND_UART)
#define CONFIG_TUTORIAL_0038_BACKEND_UART 1
#endif
#ifndef CONFIG_TUTORIAL_0038_UART_NUM
#define CONFIG_TUTORIAL_0038_UART_NUM 1
#endif
#ifndef CONFIG_TUTORIAL_0038_UART_BAUD
#define CONFIG_TUTORIAL_0038_UART_BAUD 115200
#endif
#ifndef CONFIG_TUTORIAL_0038_UART_TX_GPIO
#define CONFIG_TUTORIAL_0038_UART_TX_GPIO 2
#endif
#ifndef CONFIG_TUTORIAL_0038_UART_RX_GPIO
#define CONFIG_TUTORIAL_0038_UART_RX_GPIO 1
#endif
#ifndef CONFIG_TUTORIAL_0038_LOCAL_ECHO
#define CONFIG_TUTORIAL_0038_LOCAL_ECHO 1
#endif
#ifndef CONFIG_TUTORIAL_0038_SHOW_RX
#define CONFIG_TUTORIAL_0038_SHOW_RX 1
#endif
#ifndef CONFIG_TUTORIAL_0038_NEWLINE_CRLF
#define CONFIG_TUTORIAL_0038_NEWLINE_CRLF 1
#endif

// Backspace byte selection (choice)
#if !defined(CONFIG_TUTORIAL_0038_BS_SEND_BS) && !defined(CONFIG_TUTORIAL_0038_BS_SEND_DEL)
#define CONFIG_TUTORIAL_0038_BS_SEND_DEL 1
#endif

#ifndef CONFIG_TUTORIAL_0038_LOG_KEY_EVENTS
#define CONFIG_TUTORIAL_0038_LOG_KEY_EVENTS 0
#endif

#ifndef CONFIG_TUTORIAL_0038_RELAY_ENABLE
#define CONFIG_TUTORIAL_0038_RELAY_ENABLE 0
#endif
#ifndef CONFIG_TUTORIAL_0038_RELAY_GPIO
#define CONFIG_TUTORIAL_0038_RELAY_GPIO 3
#endif
#ifndef CONFIG_TUTORIAL_0038_RELAY_ACTIVE_HIGH
#define CONFIG_TUTORIAL_0038_RELAY_ACTIVE_HIGH 0
#endif

static std::string trim_copy(std::string_view in) {
    size_t start = 0;
    while (start < in.size() && (in[start] == ' ' || in[start] == '\t')) start++;
    size_t end = in.size();
    while (end > start && (in[end - 1] == ' ' || in[end - 1] == '\t')) end--;
    return std::string(in.substr(start, end - start));
}

static std::vector<std::string_view> split_ws(std::string_view in) {
    std::vector<std::string_view> out;
    size_t i = 0;
    while (i < in.size()) {
        while (i < in.size() && (in[i] == ' ' || in[i] == '\t')) i++;
        if (i >= in.size()) break;
        const size_t start = i;
        while (i < in.size() && in[i] != ' ' && in[i] != '\t') i++;
        out.emplace_back(in.substr(start, i - start));
    }
    return out;
}

// ---------------- Serial backend (copied from 0015) ----------------

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
#if CONFIG_TUTORIAL_0038_BACKEND_UART
    b.kind = BACKEND_UART;
    b.uart_num = (uart_port_t)CONFIG_TUTORIAL_0038_UART_NUM;

    uart_config_t cfg = {};
    cfg.baud_rate = CONFIG_TUTORIAL_0038_UART_BAUD;
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
                                CONFIG_TUTORIAL_0038_UART_TX_GPIO,
                                CONFIG_TUTORIAL_0038_UART_RX_GPIO,
                                UART_PIN_NO_CHANGE,
                                UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "serial backend: UART%u baud=%d tx=%d rx=%d",
             (unsigned)b.uart_num,
             CONFIG_TUTORIAL_0038_UART_BAUD,
             CONFIG_TUTORIAL_0038_UART_TX_GPIO,
             CONFIG_TUTORIAL_0038_UART_RX_GPIO);
#else
    b.kind = BACKEND_USB_SERIAL_JTAG;
    b.uart_num = UART_NUM_0;
    b.usb_driver_ok = usb_serial_jtag_is_driver_installed();
    if (!b.usb_driver_ok) {
        usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        cfg.rx_buffer_size = 1024;
        cfg.tx_buffer_size = 1024;
        esp_err_t err = usb_serial_jtag_driver_install(&cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "USB-Serial-JTAG driver install failed: %s", esp_err_to_name(err));
        }
        b.usb_driver_ok = usb_serial_jtag_is_driver_installed();
    }
    ESP_LOGI(TAG, "serial backend: USB-Serial-JTAG (installed=%d)", (int)b.usb_driver_ok);
#endif
}

static int backend_write(serial_backend_t &b, const uint8_t *data, size_t len) {
    if (!data || len == 0) return 0;
    if (b.kind == BACKEND_UART) {
        return uart_write_bytes(b.uart_num, (const char *)data, (size_t)len);
    }
    if (!usb_serial_jtag_is_driver_installed()) return 0;
    return (int)usb_serial_jtag_write_bytes((const void *)data, len, 0);
}

static int backend_read(serial_backend_t &b, uint8_t *data, size_t max_len) {
    if (!data || max_len == 0) return 0;
    if (b.kind == BACKEND_UART) {
        int n = uart_read_bytes(b.uart_num, data, (uint32_t)max_len, 0);
        return n < 0 ? 0 : n;
    }
    if (!usb_serial_jtag_is_driver_installed()) return 0;
    return (int)usb_serial_jtag_read_bytes((void *)data, max_len, 0);
}

static size_t token_to_tx_bytes(const char *tok, uint8_t out[8]) {
    if (!tok || !out) return 0;

    if (strcmp(tok, "space") == 0) {
        out[0] = (uint8_t)' ';
        return 1;
    }
    if (strcmp(tok, "tab") == 0) {
        out[0] = (uint8_t)'\t';
        return 1;
    }
    if (strcmp(tok, "enter") == 0) {
#if CONFIG_TUTORIAL_0038_NEWLINE_CRLF
        out[0] = (uint8_t)'\r';
        out[1] = (uint8_t)'\n';
        return 2;
#else
        out[0] = (uint8_t)'\n';
        return 1;
#endif
    }
    if (strcmp(tok, "del") == 0) {
#if CONFIG_TUTORIAL_0038_BS_SEND_BS
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

// ---------------- Terminal buffer (copied from 0015) ----------------

static void buf_trim(std::deque<std::string> &lines, size_t max_lines, size_t &dropped_lines) {
    while (lines.size() > max_lines) {
        lines.pop_front();
        dropped_lines++;
    }
    if (lines.empty()) lines.push_back(std::string());
}

typedef struct {
    std::deque<std::string> *lines;
    size_t *dropped_lines;
    size_t max_lines;
} ui_out_ctx_t;

static void ui_out(void *ctx, const char *line) {
    if (!ctx || !line) return;
    auto *st = (ui_out_ctx_t *)ctx;
    if (!st->lines || !st->dropped_lines) return;
    st->lines->push_back(std::string(line));
    buf_trim(*st->lines, st->max_lines, *st->dropped_lines);
}

static void buf_insert_char(std::deque<std::string> &lines, char c) {
    if (lines.empty()) lines.push_back(std::string());
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
    if (lines.size() > 1) {
        lines.pop_back();
    }
}

static void buf_newline(std::deque<std::string> &lines, size_t max_lines, size_t &dropped_lines) {
    if (lines.empty()) lines.push_back(std::string());
    lines.push_back(std::string());
    buf_trim(lines, max_lines, dropped_lines);
}

typedef struct {
    bool pending_cr;
} term_rx_state_t;

static void term_apply_byte(std::deque<std::string> &lines,
                            size_t max_lines,
                            size_t &dropped_lines,
                            term_rx_state_t &st,
                            uint8_t b) {
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
}

// ---------------- Cardputer-ADV keyboard (TCA8418) ----------------

#include "Adafruit_TCA8418.h"

#define KBD_INT GPIO_NUM_11

static Adafruit_TCA8418 s_tca{};
static bool s_kbd_ready = false;
static bool s_kbd_shift = false;
static bool s_kbd_capslock = false;
static bool s_kbd_ctrl = false;
static bool s_kbd_alt = false;
static bool s_kbd_opt = false;
static volatile bool s_kbd_isr_flag = false;

// Key legend matches the vendor HAL's "picture" coordinate system (4 rows x 14 columns).
// Derived from M5Cardputer-UserDemo-ADV/main/hal/keyboard/keybaord.cpp.
static const char *s_key_first[4][14] = {
    {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "del"},
    {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\"},
    {"shift", "capslock", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "enter"},
    {"ctrl", "opt", "alt", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "space"},
};

static const char *s_key_second[4][14] = {
    {"~", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "del"},
    {"tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "|"},
    {"shift", "capslock", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "enter"},
    {"ctrl", "opt", "alt", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "space"},
};

typedef struct {
    bool pressed;
    uint8_t row;
    uint8_t col;
} kbd_key_t;

static bool tok_is(const char *tok, const char *name) {
    return tok && name && strcmp(tok, name) == 0;
}

static bool kbd_is_letter(const char *s) {
    if (!s) return false;
    return (s[0] >= 'a' && s[0] <= 'z') || (s[0] >= 'A' && s[0] <= 'Z');
}

static bool kbd_decode_event(uint8_t event_raw, kbd_key_t *out) {
    if (!out) return false;
    if (event_raw == 0) return false;

    // Matches 0036 behavior on-device: bit7 is "pressed" and bits6..0 are 1-based key number.
    out->pressed = (event_raw & 0x80) != 0;
    uint8_t keynum = (uint8_t)(event_raw & 0x7F);
    if (keynum == 0) return false;
    keynum--; // 0-based

    uint8_t row = (uint8_t)(keynum / 10);
    uint8_t col = (uint8_t)(keynum % 10);

    uint8_t out_col = (uint8_t)(row * 2);
    if (col > 3) out_col++;
    uint8_t out_row = (uint8_t)((col + 4) % 4);

    out->row = out_row;
    out->col = out_col;
    return (out->row < 4 && out->col < 14);
}

static void IRAM_ATTR kbd_int_isr(void *arg) {
    (void)arg;
    s_kbd_isr_flag = true;
}

static esp_err_t kbd_hw_init(void) {
    if (s_kbd_ready) return ESP_OK;

    if (!s_tca.begin()) return ESP_FAIL;
    (void)s_tca.matrix(7, 8);
    (void)s_tca.flush();

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << KBD_INT);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // match vendor demo
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    esp_err_t err = gpio_install_isr_service((int)ESP_INTR_FLAG_IRAM);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    err = gpio_isr_handler_add(KBD_INT, &kbd_int_isr, NULL);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    s_tca.enableInterrupts();
    s_kbd_ready = true;
    return ESP_OK;
}

// ---------------- App ----------------

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    ESP_LOGI(TAG, "bringing up M5Unified...");
    M5.begin();
    M5.Display.setBrightness(255);
    M5.Display.setColorDepth(16);
    auto &display = M5.Display;
    ESP_LOGI(TAG, "display ready: width=%d height=%d", (int)display.width(), (int)display.height());

    const int w = (int)display.width();
    const int h = (int)display.height();

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0038_CANVAS_USE_PSRAM
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

    // Serial backend.
    serial_backend_t backend{};
    backend_init(backend);

    fan_control_init_defaults(&g_fan);

#if CONFIG_TUTORIAL_0038_RELAY_ENABLE
    const bool fan_ok = fan_control_init_relay(&g_fan,
                                               (gpio_num_t)CONFIG_TUTORIAL_0038_RELAY_GPIO,
                                               (bool)CONFIG_TUTORIAL_0038_RELAY_ACTIVE_HIGH,
                                               false);
    if (!fan_ok) {
        ESP_LOGW(TAG, "fan relay init failed (gpio=%d)", (int)CONFIG_TUTORIAL_0038_RELAY_GPIO);
    } else {
        ESP_LOGI(TAG,
                 "fan relay ready: gpio=%d active_high=%d",
                 (int)CONFIG_TUTORIAL_0038_RELAY_GPIO,
                 (int)CONFIG_TUTORIAL_0038_RELAY_ACTIVE_HIGH);
    }
#endif

    // ADV keyboard.
    esp_err_t err = kbd_hw_init();
    if (err != ESP_OK) {
        ESP_LOGW(KBD_TAG, "keyboard init failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(KBD_TAG, "keyboard ready (TCA8418)");
    }

    // Terminal state.
    std::deque<std::string> lines;
    lines.push_back(std::string());
    size_t dropped_lines = 0;
    uint32_t edit_events = 0;
    term_rx_state_t rx_state{.pending_cr = false};

    char last_token[16] = "(none)";
    int last_row = -1;
    int last_col = -1;
    bool dirty = true;

    ESP_LOGI(TAG, "ready: local_echo=%d show_rx=%d newline_crlf=%d",
             (int)CONFIG_TUTORIAL_0038_LOCAL_ECHO,
             (int)CONFIG_TUTORIAL_0038_SHOW_RX,
             (int)CONFIG_TUTORIAL_0038_NEWLINE_CRLF);

    console_start();

    while (true) {
        if (s_kbd_ready && s_kbd_isr_flag) {
            for (;;) {
                const uint8_t evt = s_tca.getEvent();
                if (evt == 0) break;

                kbd_key_t k{};
                if (!kbd_decode_event(evt, &k)) continue;

                const char *first = s_key_first[k.row][k.col];
                const char *second = s_key_second[k.row][k.col];
                if (!first || !second) continue;

                if (CONFIG_TUTORIAL_0038_LOG_KEY_EVENTS) {
                    printf("kbd: evt=0x%02x pressed=%d pos=(%u,%u) key=%s\n",
                           (unsigned)evt,
                           k.pressed ? 1 : 0,
                           (unsigned)k.row,
                           (unsigned)k.col,
                           first ? first : "?");
                }

                // Modifiers
                if (k.row == 2 && k.col == 0) { // shift
                    s_kbd_shift = k.pressed;
                    continue;
                }
                if (k.row == 2 && k.col == 1) { // capslock
                    if (k.pressed) s_kbd_capslock = !s_kbd_capslock;
                    continue;
                }
                if (tok_is(first, "ctrl")) {
                    s_kbd_ctrl = k.pressed;
                    continue;
                }
                if (tok_is(first, "alt")) {
                    s_kbd_alt = k.pressed;
                    continue;
                }
                if (tok_is(first, "opt")) {
                    s_kbd_opt = k.pressed;
                    continue;
                }

                // Only emit characters on press.
                if (!k.pressed) continue;

                bool use_shifted = s_kbd_shift;
                if (kbd_is_letter(first)) {
                    use_shifted = s_kbd_shift ^ s_kbd_capslock;
                }
                const char *tok = use_shifted ? second : first;
                if (!tok || tok[0] == '\0') continue;

                // Terminal token normalization (match 0015 expectations).
                if (strcmp(tok, " ") == 0) tok = "space";

                (void)snprintf(last_token, sizeof(last_token), "%s", tok);
                last_row = (int)k.row;
                last_col = (int)k.col;

                // Local fan hotkeys (do not send to backend):
                // - opt+1: toggle
                // - opt+2: on
                // - opt+3: off
                // - opt+4: preset slow
                // - opt+5: preset pulse
                // - opt+6: preset double
                // - opt+7: stop
#if CONFIG_TUTORIAL_0038_RELAY_ENABLE
                if (fan_control_is_ready(&g_fan) && s_kbd_opt && strlen(tok) == 1) {
                    if (tok[0] == '1') {
                        fan_control_toggle_manual(&g_fan);
                        lines.push_back(std::string("[fan] toggle -> ").append(fan_control_is_on(&g_fan) ? "on" : "off"));
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                    if (tok[0] == '2') {
                        fan_control_set_manual(&g_fan, true);
                        lines.push_back("[fan] set on");
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                    if (tok[0] == '3') {
                        fan_control_set_manual(&g_fan, false);
                        lines.push_back("[fan] set off");
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                    if (tok[0] == '4') {
                        (void)fan_control_start_preset(&g_fan, "slow");
                        lines.push_back("[fan] preset slow");
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                    if (tok[0] == '5') {
                        (void)fan_control_start_preset(&g_fan, "pulse");
                        lines.push_back("[fan] preset pulse");
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                    if (tok[0] == '6') {
                        (void)fan_control_start_preset(&g_fan, "double");
                        lines.push_back("[fan] preset double");
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                    if (tok[0] == '7') {
                        fan_control_stop(&g_fan);
                        lines.push_back("[fan] stop");
                        buf_trim(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);
                        edit_events++;
                        dirty = true;
                        continue;
                    }
                }
#endif

                // Local command mode: if the current line starts with `fan`, execute locally on Enter.
                if (strcmp(tok, "enter") == 0) {
                    const std::string cmdline = trim_copy(lines.empty() ? "" : std::string_view(lines.back()));
                    const auto toks = split_ws(cmdline);
                    if (!toks.empty() && toks[0] == "fan") {
                        buf_newline(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);

                        std::vector<std::string> args;
                        args.reserve(toks.size());
                        for (const auto &sv : toks) args.emplace_back(sv);
                        std::vector<const char *> argv;
                        argv.reserve(args.size());
                        for (const auto &s : args) argv.push_back(s.c_str());

                        ui_out_ctx_t out_ctx{.lines = &lines, .dropped_lines = &dropped_lines, .max_lines = (size_t)CONFIG_TUTORIAL_0038_MAX_LINES};
                        (void)fan_cmd_exec(&g_fan, (int)argv.size(), argv.data(), &ui_out, &out_ctx);
                        buf_newline(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines);

                        edit_events++;
                        dirty = true;
                        continue;
                    }
                }

                uint8_t tx[8] = {0};
                const size_t tx_n = token_to_tx_bytes(tok, tx);
                if (tx_n == 0) continue;

                backend_write(backend, tx, tx_n);
                if (CONFIG_TUTORIAL_0038_LOCAL_ECHO) {
                    for (size_t i = 0; i < tx_n; i++) {
                        term_apply_byte(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines, rx_state, tx[i]);
                    }
                    edit_events++;
                    dirty = true;
                }
            }

            // try to clear the IRQ flag; if there are pending events it is not cleared
            s_tca.writeRegister8(TCA8418_REG_INT_STAT, 1);
            const int intstat = s_tca.readRegister8(TCA8418_REG_INT_STAT);
            if ((intstat & 0x01) == 0) {
                s_kbd_isr_flag = false;
            }
        }

#if CONFIG_TUTORIAL_0038_SHOW_RX
        while (true) {
            uint8_t rx_buf[64];
            int rn = backend_read(backend, rx_buf, sizeof(rx_buf));
            if (rn <= 0) break;
            for (int i = 0; i < rn; i++) {
                term_apply_byte(lines, (size_t)CONFIG_TUTORIAL_0038_MAX_LINES, dropped_lines, rx_state, rx_buf[i]);
            }
            edit_events++;
            dirty = true;
        }
#endif

        if (dirty) {
            dirty = false;
            canvas.fillScreen(TFT_BLACK);
            canvas.setTextColor(TFT_WHITE, TFT_BLACK);
            canvas.setTextWrap(false);
            canvas.setCursor(0, 0);

            canvas.printf("Cardputer-ADV Serial Terminal (0038)\n");
#if CONFIG_TUTORIAL_0038_BACKEND_UART
            canvas.printf("Backend: UART%u %d baud\n", (unsigned)backend.uart_num, CONFIG_TUTORIAL_0038_UART_BAUD);
#else
            canvas.printf("Backend: USB-Serial-JTAG\n");
#endif
            canvas.printf("Last: %s pos=(%d,%d) shift=%d caps=%d ctrl=%d opt=%d alt=%d\n",
                          last_token,
                          last_row,
                          last_col,
                          s_kbd_shift ? 1 : 0,
                          s_kbd_capslock ? 1 : 0,
                          s_kbd_ctrl ? 1 : 0,
                          s_kbd_opt ? 1 : 0,
                          s_kbd_alt ? 1 : 0);
#if CONFIG_TUTORIAL_0038_RELAY_ENABLE
            const char *fan_mode = fan_control_mode_name(fan_control_mode(&g_fan));
            const char *fan_preset = fan_control_active_preset_name(&g_fan);
            canvas.printf("Fan: gpio=%d active_high=%d state=%s mode=%s%s%s\n",
                          (int)CONFIG_TUTORIAL_0038_RELAY_GPIO,
                          (int)CONFIG_TUTORIAL_0038_RELAY_ACTIVE_HIGH,
                          fan_control_is_ready(&g_fan) ? (fan_control_is_on(&g_fan) ? "on" : "off") : "n/a",
                          fan_mode ? fan_mode : "?",
                          fan_preset ? " preset=" : "",
                          fan_preset ? fan_preset : "");
            canvas.printf("Fan: opt+1 toggle opt+2 on opt+3 off opt+4 slow opt+5 pulse opt+6 double opt+7 stop; type `fan ...` + Enter\n");
#else
            canvas.printf("Fan: disabled\n");
#endif
            canvas.printf("Lines: %u (dropped=%u)  Events: %" PRIu32 "\n",
                          (unsigned)lines.size(), (unsigned)dropped_lines, edit_events);

            const int line_h = canvas.fontHeight();
            const int header_rows = 6;
            const int max_rows = (line_h > 0) ? (canvas.height() / line_h) : 0;
            int text_rows = max_rows - header_rows;
            if (text_rows < 1) text_rows = 1;

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

            if (!lines.empty() && line_h > 0) {
                int x_cursor = canvas.textWidth(lines.back().c_str());
                if (x_cursor < 0) x_cursor = 0;
                if (x_cursor > canvas.width() - 2) x_cursor = canvas.width() - 2;
                int cursor_w = canvas.textWidth("_");
                if (cursor_w <= 0) cursor_w = 6;
                int y_underline = y_cursor + line_h - 2;
                if (y_underline < 0) y_underline = 0;
                if (y_underline > canvas.height() - 2) y_underline = canvas.height() - 2;
                canvas.fillRect(x_cursor, y_underline, cursor_w, 2, TFT_WHITE);
            }

            canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0038_PRESENT_WAIT_DMA
            display.waitDMA();
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
