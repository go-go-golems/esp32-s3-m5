/*
 * Tutorial 0016: manual line-based REPL (no esp_console).
 *
 * Goals:
 * - deterministic (no linenoise/history/tab completion)
 * - runs over USB Serial/JTAG directly (driver read/write)
 * - command parsing maps to CtrlEvent via ctrl_send()
 *
 * Accepted line endings: CR, LF, CRLF.
 */

#include "manual_repl.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/usb_serial_jtag.h"

#include "control_plane.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

static bool streq(const char *a, const char *b) {
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static bool try_parse_int(const char *s, int *out) {
    if (!s || !*s) return false;
    char *end = nullptr;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static void repl_write(const void *data, size_t len) {
    if (!data || len == 0) return;
    if (usb_serial_jtag_is_driver_installed()) {
        (void)usb_serial_jtag_write_bytes(data, len, 0);
        return;
    }
    // Fallback: best-effort stdio (may be routed elsewhere depending on sdkconfig).
    (void)fwrite(data, 1, len, stdout);
    (void)fflush(stdout);
}

static void repl_puts(const char *s) {
    if (!s) return;
    repl_write(s, strlen(s));
}

static void print_help(void) {
    repl_puts(
        "Commands:\r\n"
        "  help\r\n"
        "  mode tx|rx|idle|uart_tx|uart_rx\r\n"
        "  pin 1|2\r\n"
        "  tx high|low|stop\r\n"
        "  tx square <hz>\r\n"
        "  tx pulse <width_us> <period_ms>\r\n"
        "  rx edges rising|falling|both\r\n"
        "  rx pull none|up|down\r\n"
        "  rx reset\r\n"
        "  uart baud <baud>\r\n"
        "  uart map normal|swapped\r\n"
        "  uart tx <token> <delay_ms>\r\n"
        "  uart tx stop\r\n"
        "  uart rx get [max_bytes]\r\n"
        "  uart rx clear\r\n"
        "  status\r\n");
}

static void handle_tokens(int argc, char **argv) {
    if (argc <= 0 || !argv || !argv[0]) {
        return;
    }
    if (streq(argv[0], "help")) {
        print_help();
        return;
    }
    if (streq(argv[0], "mode")) {
        if (argc < 2) {
            repl_puts("usage: mode tx|rx|idle|uart_tx|uart_rx\r\n");
            return;
        }
        int m = -1;
        if (streq(argv[1], "idle")) m = 0;
        else if (streq(argv[1], "tx")) m = 1;
        else if (streq(argv[1], "rx")) m = 2;
        else if (streq(argv[1], "uart_tx")) m = 3;
        else if (streq(argv[1], "uart_rx")) m = 4;
        else {
            repl_puts("invalid mode\r\n");
            return;
        }
        ctrl_send({.type = CtrlType::SetMode, .arg0 = m, .arg1 = 0});
        return;
    }
    if (streq(argv[0], "pin")) {
        if (argc < 2) {
            repl_puts("usage: pin 1|2\r\n");
            return;
        }
        int p = 0;
        if (!try_parse_int(argv[1], &p) || (p != 1 && p != 2)) {
            repl_puts("invalid pin\r\n");
            return;
        }
        ctrl_send({.type = CtrlType::SetPin, .arg0 = p, .arg1 = 0});
        return;
    }
    if (streq(argv[0], "status")) {
        ctrl_send({.type = CtrlType::Status, .arg0 = 0, .arg1 = 0});
        return;
    }
    if (streq(argv[0], "tx")) {
        if (argc < 2) {
            repl_puts("usage: tx high|low|stop|square <hz>|pulse <width_us> <period_ms>\r\n");
            return;
        }
        if (streq(argv[1], "high")) {
            ctrl_send({.type = CtrlType::TxHigh, .arg0 = 0, .arg1 = 0});
            return;
        }
        if (streq(argv[1], "low")) {
            ctrl_send({.type = CtrlType::TxLow, .arg0 = 0, .arg1 = 0});
            return;
        }
        if (streq(argv[1], "stop")) {
            ctrl_send({.type = CtrlType::TxStop, .arg0 = 0, .arg1 = 0});
            return;
        }
        if (streq(argv[1], "square")) {
            if (argc < 3) {
                repl_puts("usage: tx square <hz>\r\n");
                return;
            }
            int hz = 0;
            if (!try_parse_int(argv[2], &hz) || hz <= 0 || hz > 20000) {
                repl_puts("invalid hz (allowed: 1..20000)\r\n");
                return;
            }
            ctrl_send({.type = CtrlType::TxSquare, .arg0 = hz, .arg1 = 0});
            return;
        }
        if (streq(argv[1], "pulse")) {
            if (argc < 4) {
                repl_puts("usage: tx pulse <width_us> <period_ms>\r\n");
                return;
            }
            int width_us = 0;
            int period_ms = 0;
            if (!try_parse_int(argv[2], &width_us) || width_us <= 0) {
                repl_puts("invalid width_us\r\n");
                return;
            }
            if (!try_parse_int(argv[3], &period_ms) || period_ms <= 0) {
                repl_puts("invalid period_ms\r\n");
                return;
            }
            ctrl_send({.type = CtrlType::TxPulse, .arg0 = width_us, .arg1 = period_ms});
            return;
        }
        repl_puts("unknown tx subcommand\r\n");
        return;
    }
    if (streq(argv[0], "rx")) {
        if (argc < 2) {
            repl_puts("usage: rx edges rising|falling|both | rx pull none|up|down | rx reset\r\n");
            return;
        }
        if (streq(argv[1], "reset")) {
            ctrl_send({.type = CtrlType::RxReset, .arg0 = 0, .arg1 = 0});
            return;
        }
        if (streq(argv[1], "edges")) {
            if (argc < 3) {
                repl_puts("usage: rx edges rising|falling|both\r\n");
                return;
            }
            int m = -1;
            if (streq(argv[2], "rising")) m = 0;
            else if (streq(argv[2], "falling")) m = 1;
            else if (streq(argv[2], "both")) m = 2;
            else {
                repl_puts("invalid edges mode\r\n");
                return;
            }
            ctrl_send({.type = CtrlType::RxEdges, .arg0 = m, .arg1 = 0});
            return;
        }
        if (streq(argv[1], "pull")) {
            if (argc < 3) {
                repl_puts("usage: rx pull none|up|down\r\n");
                return;
            }
            int p = -1;
            if (streq(argv[2], "none")) p = 0;
            else if (streq(argv[2], "up")) p = 1;
            else if (streq(argv[2], "down")) p = 2;
            else {
                repl_puts("invalid pull\r\n");
                return;
            }
            ctrl_send({.type = CtrlType::RxPull, .arg0 = p, .arg1 = 0});
            return;
        }
        repl_puts("unknown rx subcommand\r\n");
        return;
    }

    if (streq(argv[0], "uart")) {
        if (argc < 2) {
            repl_puts("usage: uart baud|map|tx|rx ...\r\n");
            return;
        }
        if (streq(argv[1], "baud")) {
            if (argc < 3) {
                repl_puts("usage: uart baud <baud>\r\n");
                return;
            }
            int baud = 0;
            if (!try_parse_int(argv[2], &baud) || baud <= 0) {
                repl_puts("invalid baud\r\n");
                return;
            }
            CtrlEvent ev = {};
            ev.type = CtrlType::UartBaud;
            ev.arg0 = baud;
            ctrl_send(ev);
            return;
        }
        if (streq(argv[1], "map")) {
            if (argc < 3) {
                repl_puts("usage: uart map normal|swapped\r\n");
                return;
            }
            int map = -1;
            if (streq(argv[2], "normal")) map = 0;
            else if (streq(argv[2], "swapped")) map = 1;
            else {
                repl_puts("invalid map\r\n");
                return;
            }
            CtrlEvent ev = {};
            ev.type = CtrlType::UartMap;
            ev.arg0 = map;
            ctrl_send(ev);
            return;
        }
        if (streq(argv[1], "tx")) {
            if (argc < 3) {
                repl_puts("usage: uart tx <token> <delay_ms> | uart tx stop\r\n");
                return;
            }
            if (streq(argv[2], "stop")) {
                CtrlEvent ev = {};
                ev.type = CtrlType::UartTxStop;
                ctrl_send(ev);
                return;
            }
            if (argc < 4) {
                repl_puts("usage: uart tx <token> <delay_ms>\r\n");
                return;
            }
            int delay_ms = 0;
            if (!try_parse_int(argv[3], &delay_ms) || delay_ms < 0) {
                repl_puts("invalid delay_ms\r\n");
                return;
            }
            CtrlEvent ev = {};
            ev.type = CtrlType::UartTxStart;
            ev.arg0 = delay_ms;
            // payload token (single token)
            strncpy(ev.str0, argv[2], sizeof(ev.str0) - 1);
            ev.str0[sizeof(ev.str0) - 1] = '\0';
            ctrl_send(ev);
            return;
        }
        if (streq(argv[1], "rx")) {
            if (argc < 3) {
                repl_puts("usage: uart rx get [max_bytes] | uart rx clear\r\n");
                return;
            }
            if (streq(argv[2], "clear")) {
                CtrlEvent ev = {};
                ev.type = CtrlType::UartRxClear;
                ctrl_send(ev);
                return;
            }
            if (streq(argv[2], "get")) {
                int max_bytes = 0;
                if (argc >= 4) {
                    if (!try_parse_int(argv[3], &max_bytes) || max_bytes < 0) {
                        repl_puts("invalid max_bytes\r\n");
                        return;
                    }
                }
                CtrlEvent ev = {};
                ev.type = CtrlType::UartRxGet;
                ev.arg0 = max_bytes;
                ctrl_send(ev);
                return;
            }
            repl_puts("unknown uart rx subcommand\r\n");
            return;
        }

        repl_puts("unknown uart subcommand\r\n");
        return;
    }

    repl_puts("unknown command (try: help)\r\n");
}

static void handle_line(char *line) {
    if (!line) return;

    // Trim leading whitespace.
    while (*line == ' ' || *line == '\t') line++;
    if (!*line) return;

    // Tokenize in-place.
    char *argv[8] = {};
    int argc = 0;
    char *save = nullptr;
    for (char *tok = strtok_r(line, " \t", &save);
         tok && argc < (int)(sizeof(argv) / sizeof(argv[0]));
         tok = strtok_r(nullptr, " \t", &save)) {
        argv[argc++] = tok;
    }
    handle_tokens(argc, argv);
}

static void repl_task(void *arg) {
    (void)arg;

    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        cfg.rx_buffer_size = 1024;
        cfg.tx_buffer_size = 1024;
        esp_err_t err = usb_serial_jtag_driver_install(&cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "USB-Serial-JTAG driver install failed: %s", esp_err_to_name(err));
        }
    }

    ESP_LOGI(TAG, "manual repl started (usb_serial_jtag installed=%d)", (int)usb_serial_jtag_is_driver_installed());

    // Line buffer.
    char line[128] = {};
    size_t n = 0;

    // CRLF suppression: if we saw CR, ignore the next LF.
    bool ignore_next_lf = false;

    while (true) {
        repl_puts(CONFIG_TUTORIAL_0016_CONSOLE_PROMPT);
        n = 0;
        ignore_next_lf = false;

        while (true) {
            uint8_t c = 0;
            int got = 0;
            if (usb_serial_jtag_is_driver_installed()) {
                got = (int)usb_serial_jtag_read_bytes((void *)&c, 1, pdMS_TO_TICKS(100));
            } else {
                // No driver: avoid busy loop.
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
            if (got <= 0) {
                continue;
            }

            if (ignore_next_lf && c == '\n') {
                ignore_next_lf = false;
                continue;
            }
            ignore_next_lf = false;

            if (c == '\r' || c == '\n') {
                if (c == '\r') {
                    ignore_next_lf = true;
                }
                repl_puts("\r\n");
                line[n] = '\0';
                break;
            }

            // Backspace support (basic).
            if (c == 0x08 || c == 0x7f) {
                if (n > 0) {
                    n--;
                    // erase last char visually: "\b \b"
                    repl_puts("\b \b");
                }
                continue;
            }

            // Normalize tabs to a single space.
            if (c == '\t') {
                c = ' ';
            }

            // Accept printable ASCII only (keep deterministic).
            if (c < 32 || c >= 127) {
                continue;
            }

            if (n + 1 < sizeof(line)) {
                line[n++] = (char)c;
                repl_write(&c, 1);  // echo
            } else {
                // Buffer full: drop input until newline.
            }
        }

        // Execute.
        handle_line(line);
    }
}

void manual_repl_start(void) {
    BaseType_t ok = xTaskCreatePinnedToCore(repl_task, "manual_repl", 4096, nullptr, 1, nullptr, tskNO_AFFINITY);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "failed to create manual repl task");
    }
}


