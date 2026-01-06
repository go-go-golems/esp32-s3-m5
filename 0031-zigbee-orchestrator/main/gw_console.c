#include "gw_console.h"

#include "esp_console.h"
#include "esp_log.h"

#include "gw_console_cmds.h"

static const char *TAG = "gw_console_0031";

esp_err_t gw_console_start(void) {
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = CONFIG_GW_CONSOLE_PROMPT;

    esp_err_t err = ESP_ERR_NOT_SUPPORTED;

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    const char *backend = "USB Serial/JTAG";
#else
    const char *backend = "none";
#endif

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console unavailable (backend=%s, err=%s)", backend, esp_err_to_name(err));
        return err;
    }

    esp_console_register_help_command();
    gw_console_cmds_init();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "esp_console started over %s (try: help)", backend);
    return ESP_OK;
}
