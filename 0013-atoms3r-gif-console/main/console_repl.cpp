/*
 * Tutorial 0013: esp_console REPL startup + command registration.
 */

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "control_plane.h"
#include "gif_registry.h"

#include "console_repl.h"

static const char *TAG = "atoms3r_gif_console";

static bool try_parse_int(const char *s, int *out) {
    if (!s || !*s) return false;
    char *end = nullptr;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static int cmd_list(int, char **) {
    const int n = gif_registry_refresh();
    printf("gif assets (%d):\n", n);
    for (int i = 0; i < n; i++) {
        const char *p = gif_registry_path(i);
        printf("  %d: %s\n", i, path_basename(p));
    }
    return 0;
}

static int cmd_play(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: play <id|name>\n");
        return 1;
    }

    (void)gif_registry_refresh();

    int idx = -1;
    if (try_parse_int(argv[1], &idx)) {
        // ok
    } else {
        idx = gif_registry_find_by_name(argv[1]);
    }
    if (idx < 0 || idx >= gif_registry_count()) {
        printf("invalid gif: %s\n", argv[1]);
        return 1;
    }

    ctrl_send({.type = CtrlType::PlayIndex, .arg = idx});
    return 0;
}

static int cmd_stop(int, char **) {
    ctrl_send({.type = CtrlType::Stop, .arg = 0});
    return 0;
}

static int cmd_next(int, char **) {
    ctrl_send({.type = CtrlType::Next, .arg = 0});
    return 0;
}

static int cmd_info(int, char **) {
    ctrl_send({.type = CtrlType::Info, .arg = 0});
    return 0;
}

static int cmd_brightness(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: brightness <0..255>\n");
        return 1;
    }
    int v = 0;
    if (!try_parse_int(argv[1], &v) || v < 0 || v > 255) {
        printf("invalid brightness: %s\n", argv[1]);
        return 1;
    }
    ctrl_send({.type = CtrlType::SetBrightness, .arg = v});
    return 0;
}

static void console_register_commands(void) {
    esp_console_cmd_t cmd = {};

    cmd = {};
    cmd.command = "list";
    cmd.help = "List available GIFs (from /storage/gifs)";
    cmd.func = &cmd_list;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "play";
    cmd.help = "Play GIF by id or name: play <id|name>";
    cmd.func = &cmd_play;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "stop";
    cmd.help = "Stop playback";
    cmd.func = &cmd_stop;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "next";
    cmd.help = "Select next GIF and play";
    cmd.func = &cmd_next;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "info";
    cmd.help = "Print current playback state";
    cmd.func = &cmd_info;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "brightness";
    cmd.help = "Set backlight brightness (0..255) when I2C brightness device is enabled";
    cmd.func = &cmd_brightness;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void console_start_uart_grove(void) __attribute__((unused));
static void console_start_uart_grove(void) {
#if CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_repl_t *repl = nullptr;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = CONFIG_TUTORIAL_0013_CONSOLE_PROMPT;

    esp_console_dev_uart_config_t hw_config = {
        .channel = CONFIG_TUTORIAL_0013_CONSOLE_UART_NUM,
        .baud_rate = CONFIG_TUTORIAL_0013_CONSOLE_UART_BAUDRATE,
        .tx_gpio_num = CONFIG_TUTORIAL_0013_CONSOLE_UART_TX_GPIO,
        .rx_gpio_num = CONFIG_TUTORIAL_0013_CONSOLE_UART_RX_GPIO,
    };

    esp_err_t err = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_uart failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    console_register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over UART (channel=%d tx=%d rx=%d baud=%d)",
             CONFIG_TUTORIAL_0013_CONSOLE_UART_NUM,
             CONFIG_TUTORIAL_0013_CONSOLE_UART_TX_GPIO,
             CONFIG_TUTORIAL_0013_CONSOLE_UART_RX_GPIO,
             CONFIG_TUTORIAL_0013_CONSOLE_UART_BAUDRATE);
#else
    ESP_LOGW(TAG, "UART console support is disabled in sdkconfig (CONFIG_ESP_CONSOLE_UART_*); no console started");
#endif
}

static void console_start_usb_serial_jtag(void) __attribute__((unused));
static void console_start_usb_serial_jtag(void) {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_repl_t *repl = nullptr;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "gif> ";

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
#if CONFIG_TUTORIAL_0013_CONSOLE_BINDING_GROVE_UART
    console_start_uart_grove();
#elif CONFIG_TUTORIAL_0013_CONSOLE_BINDING_USB_SERIAL_JTAG
    console_start_usb_serial_jtag();
#else
    console_start_uart_grove();
#endif
}


