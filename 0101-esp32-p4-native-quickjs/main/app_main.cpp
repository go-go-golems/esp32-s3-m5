// 0101 — ESP32-P4 native QuickJS console firmware.
// Console = UART0 (CH343 USB-UART bridge; the P4 has no USB Serial/JTAG).
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "js_command.h"
#include "qjs_service.h"

namespace {
constexpr const char *kTag = "0101";
constexpr size_t kQuickJsMemoryLimit = 2 * 1024 * 1024;
constexpr size_t kQuickJsStackLimit = 64 * 1024;

qjs_service_t *start_quickjs_service()
{
    qjs_service_config_t cfg = {};
    cfg.task_name = "qjs0101";
    cfg.task_stack_words = 32768;
    cfg.task_priority = 8;
    cfg.task_core_id = -1;
    cfg.queue_len = 8;
    cfg.memory_limit_bytes = kQuickJsMemoryLimit;
    cfg.stack_limit_bytes = kQuickJsStackLimit;
    cfg.can_block = false;

    qjs_service_t *svc = nullptr;
    esp_err_t err = qjs_service_start(&cfg, &svc);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "qjs_service_start failed: %s", esp_err_to_name(err));
        return nullptr;
    }
    return svc;
}
}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0101 ESP32-P4 native QuickJS console (ticket ESP32-P4-NATIVE-QUICKJS)");

    qjs_service_t *svc = start_quickjs_service();
    if (svc) {
        ESP_LOGI(kTag, "QuickJS ready. Try: js eval \"print(1+2)\" or js bench");
    } else {
        ESP_LOGE(kTag, "QuickJS service unavailable; console will still start");
    }

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "0101> ";
    repl_cfg.task_stack_size = 8192;
    esp_console_register_help_command();
    register_js_commands(svc);

    esp_console_dev_uart_config_t hw_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_cfg, &repl_cfg, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
