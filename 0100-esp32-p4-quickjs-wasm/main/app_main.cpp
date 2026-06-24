// 0100 — ESP32-P4 QuickJS-WASM firmware entry point.
// Console = UART0 (CH343 USB-UART bridge; the P4 has no USB Serial/JTAG).
// Initializes WAMR (pool in PSRAM), registers the "env" host API, loads the
// embedded quickjs.wasm, runs qjs_init, and exposes `js eval` / `js status`.
#include <cstdio>

#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"

#include "js_command.h"
#include "wasm_host_api.h"
#include "wasm_runner.h"
#include "wasm_runtime_service.h"

static const char *kTag = "0100";

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0100 ESP32-P4 QuickJS-WASM (ticket ESP32-P4-QUICKJS-WASM)");

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "0100> ";
    repl_cfg.task_stack_size = 8192;
    esp_console_register_help_command();

    // Initialize the WAMR runtime + env host API + QuickJS session (qjs_init).
    if (!init_wasm_runtime()) {
        ESP_LOGE(kTag, "WAMR runtime init failed; js eval will be unavailable");
    } else if (!init_wasm_host_api()) {
        ESP_LOGE(kTag, "WAMR host API init failed; js eval will be unavailable");
    } else if (!wasm_runner_init()) {
        ESP_LOGE(kTag, "QuickJS session init failed; js eval will be unavailable");
    } else {
        ESP_LOGI(kTag, "QuickJS ready. Try: js eval \"print(1+2)\"");
    }

    register_js_commands();

    esp_console_dev_uart_config_t hw_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_cfg, &repl_cfg, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
