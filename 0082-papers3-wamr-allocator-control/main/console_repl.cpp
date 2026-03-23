#include "console_repl.h"

#include "wasm_command.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

namespace papers3_wasm {

namespace {

constexpr const char *kTag = "0079_console";

}  // namespace

void StartConsoleRepl()
{
    esp_console_repl_t *repl = nullptr;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "paper> ";

    esp_err_t err = ESP_ERR_NOT_SUPPORTED;
    const char *backend = "none";

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    backend = "USB Serial/JTAG";
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl);
    backend = "USB CDC";
#elif CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    err = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    backend = "UART";
#endif

    if (err != ESP_OK) {
        ESP_LOGW(kTag, "esp_console not started (backend=%s, err=%s)", backend, esp_err_to_name(err));
        return;
    }

    ESP_ERROR_CHECK(esp_console_register_help_command());
    RegisterWasmCommands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(kTag, "esp_console started over %s", backend);
}

}  // namespace papers3_wasm
