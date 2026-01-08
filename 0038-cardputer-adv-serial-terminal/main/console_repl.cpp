#include "console_repl.h"

#include <stdio.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "fan_cmd.h"
#include "app_state.h"

static const char *TAG = "0038_console";

static void console_out(void *ctx, const char *line) {
    (void)ctx;
    if (!line) return;
    printf("%s\n", line);
}

static int cmd_fan(int argc, char **argv) {
    const char *const *cargv = (const char *const *)argv;
    return fan_cmd_exec(&g_fan, argc, cargv, &console_out, nullptr);
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {};
    cmd.command = "fan";
    cmd.help = "Fan relay control: fan help";
    cmd.func = &cmd_fan;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void console_start(void) {
#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no REPL started");
    return;
#else
#if defined(CONFIG_TUTORIAL_0038_BACKEND_USB_SERIAL_JTAG) && CONFIG_TUTORIAL_0038_BACKEND_USB_SERIAL_JTAG
    // The on-screen terminal can optionally bind USB-Serial-JTAG for raw byte I/O.
    // Avoid fighting over the same transport.
    ESP_LOGW(TAG, "USB backend selected; skipping esp_console to avoid USB RX conflicts (select UART backend)");
    return;
#endif

    esp_console_repl_t *repl = nullptr;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "adv> ";
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

