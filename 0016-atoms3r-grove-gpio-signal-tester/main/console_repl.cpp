/*
 * Tutorial 0016: esp_console REPL startup + command registration (USB Serial/JTAG).
 *
 * Commands map directly to CtrlEvent messages to keep a single-writer state machine in app_main.
 */

#include "console_repl.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

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

static int cmd_mode(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: mode tx|rx|idle\n");
        return 1;
    }
    int m = -1;
    if (streq(argv[1], "idle")) m = 0;
    else if (streq(argv[1], "tx")) m = 1;
    else if (streq(argv[1], "rx")) m = 2;
    else {
        printf("invalid mode: %s\n", argv[1]);
        return 1;
    }
    ctrl_send({.type = CtrlType::SetMode, .arg0 = m, .arg1 = 0});
    return 0;
}

static int cmd_pin(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: pin 1|2\n");
        return 1;
    }
    int p = 0;
    if (!try_parse_int(argv[1], &p) || (p != 1 && p != 2)) {
        printf("invalid pin: %s\n", argv[1]);
        return 1;
    }
    ctrl_send({.type = CtrlType::SetPin, .arg0 = p, .arg1 = 0});
    return 0;
}

static int cmd_status(int, char **) {
    ctrl_send({.type = CtrlType::Status, .arg0 = 0, .arg1 = 0});
    return 0;
}

static int cmd_tx(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: tx high|low|stop|square <hz>|pulse <width_us> <period_ms>\n");
        return 1;
    }
    if (streq(argv[1], "high")) {
        ctrl_send({.type = CtrlType::TxHigh, .arg0 = 0, .arg1 = 0});
        return 0;
    }
    if (streq(argv[1], "low")) {
        ctrl_send({.type = CtrlType::TxLow, .arg0 = 0, .arg1 = 0});
        return 0;
    }
    if (streq(argv[1], "stop")) {
        ctrl_send({.type = CtrlType::TxStop, .arg0 = 0, .arg1 = 0});
        return 0;
    }
    if (streq(argv[1], "square")) {
        if (argc < 3) {
            printf("usage: tx square <hz>\n");
            return 1;
        }
        int hz = 0;
        if (!try_parse_int(argv[2], &hz) || hz <= 0 || hz > 20000) {
            printf("invalid hz: %s (allowed: 1..20000)\n", argv[2]);
            return 1;
        }
        ctrl_send({.type = CtrlType::TxSquare, .arg0 = hz, .arg1 = 0});
        return 0;
    }
    if (streq(argv[1], "pulse")) {
        if (argc < 4) {
            printf("usage: tx pulse <width_us> <period_ms>\n");
            return 1;
        }
        int width_us = 0;
        int period_ms = 0;
        if (!try_parse_int(argv[2], &width_us) || width_us <= 0) {
            printf("invalid width_us: %s\n", argv[2]);
            return 1;
        }
        if (!try_parse_int(argv[3], &period_ms) || period_ms <= 0) {
            printf("invalid period_ms: %s\n", argv[3]);
            return 1;
        }
        ctrl_send({.type = CtrlType::TxPulse, .arg0 = width_us, .arg1 = period_ms});
        return 0;
    }
    printf("unknown tx subcommand: %s\n", argv[1]);
    return 1;
}

static int cmd_rx(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: rx edges rising|falling|both | rx pull none|up|down | rx reset\n");
        return 1;
    }
    if (streq(argv[1], "reset")) {
        ctrl_send({.type = CtrlType::RxReset, .arg0 = 0, .arg1 = 0});
        return 0;
    }
    if (streq(argv[1], "edges")) {
        if (argc < 3) {
            printf("usage: rx edges rising|falling|both\n");
            return 1;
        }
        int m = -1;
        if (streq(argv[2], "rising")) m = 0;
        else if (streq(argv[2], "falling")) m = 1;
        else if (streq(argv[2], "both")) m = 2;
        else {
            printf("invalid edges mode: %s\n", argv[2]);
            return 1;
        }
        ctrl_send({.type = CtrlType::RxEdges, .arg0 = m, .arg1 = 0});
        return 0;
    }
    if (streq(argv[1], "pull")) {
        if (argc < 3) {
            printf("usage: rx pull none|up|down\n");
            return 1;
        }
        int p = -1;
        if (streq(argv[2], "none")) p = 0;
        else if (streq(argv[2], "up")) p = 1;
        else if (streq(argv[2], "down")) p = 2;
        else {
            printf("invalid pull: %s\n", argv[2]);
            return 1;
        }
        ctrl_send({.type = CtrlType::RxPull, .arg0 = p, .arg1 = 0});
        return 0;
    }
    printf("unknown rx subcommand: %s\n", argv[1]);
    return 1;
}

static void console_register_commands(void) {
    esp_console_cmd_t cmd = {};

    cmd = {};
    cmd.command = "mode";
    cmd.help = "Set tester mode: mode tx|rx|idle";
    cmd.func = &cmd_mode;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "pin";
    cmd.help = "Select active pin: pin 1|2 (GPIO1/GPIO2)";
    cmd.func = &cmd_pin;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "status";
    cmd.help = "Print current mode/pin/tx/rx stats";
    cmd.func = &cmd_status;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "tx";
    cmd.help = "TX control: tx high|low|stop|square <hz>|pulse <width_us> <period_ms>";
    cmd.func = &cmd_tx;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "rx";
    cmd.help = "RX control: rx edges rising|falling|both | rx pull none|up|down | rx reset";
    cmd.func = &cmd_rx;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void console_start_usb_serial_jtag(void) {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_repl_t *repl = nullptr;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = CONFIG_TUTORIAL_0016_CONSOLE_PROMPT;

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    console_register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over USB Serial/JTAG");
#else
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no console started");
#endif
}

void console_start(void) {
    console_start_usb_serial_jtag();
}



