// 0103 — AtomS3R M12 native QuickJS console firmware with PSRAM.
// Console = USB Serial/JTAG. Use the Espressif by-id path, not the ESP32-P4 CH343 port.
#include "esp_console.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"

#include "js_command.h"
#include "qjs_service.h"
#include "storage_namespace.h"
#include "system_namespace.h"
#include "wifi_app.h"
#include "wifi_command.h"
#include "wifi_namespace.h"

namespace {
constexpr const char *kTag = "0103";
constexpr size_t kQuickJsMemoryLimit = 1 * 1024 * 1024;
constexpr size_t kQuickJsStackLimit = 64 * 1024;

qjs_service_t *start_quickjs_service()
{
    qjs_service_config_t cfg = {};
    cfg.task_name = "qjs0103";
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

void log_memory_baseline(const char *phase)
{
    ESP_LOGI(kTag,
             "%s: psram_initialized=%d psram_size=%u internal_free=%u 8bit_free=%u psram_free=%u",
             phase,
             esp_psram_is_initialized(),
             (unsigned)esp_psram_get_size(),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0103 AtomS3R M12 native QuickJS console (ticket ATOMS3R-M12-NATIVE-QUICKJS)");
    ESP_LOGI(kTag, "serial target: /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00");
    log_memory_baseline("before_qjs");

    esp_err_t storage_err = storage_namespace_start(false);
    if (storage_err != ESP_OK) {
        ESP_LOGW(kTag, "storage mount skipped/failed: %s (run `storage mount format` for a blank dev partition)", esp_err_to_name(storage_err));
    }
    log_memory_baseline("after_storage");

    esp_err_t wifi_err = wifi_app_start();
    if (wifi_err != ESP_OK) {
        ESP_LOGW(kTag, "wifi_app_start failed: %s", esp_err_to_name(wifi_err));
    }
    log_memory_baseline("after_wifi");

    qjs_service_t *svc = start_quickjs_service();
    log_memory_baseline("after_qjs");
    if (svc) {
        esp_err_t sys_err = install_system_namespace(svc);
        if (sys_err != ESP_OK) {
            ESP_LOGW(kTag, "install_system_namespace failed: %s", esp_err_to_name(sys_err));
        }
        esp_err_t storage_ns_err = install_storage_namespace(svc);
        if (storage_ns_err != ESP_OK) {
            ESP_LOGW(kTag, "install_storage_namespace failed: %s", esp_err_to_name(storage_ns_err));
        }
        esp_err_t wifi_ns_err = install_wifi_namespace(svc);
        if (wifi_ns_err != ESP_OK) {
            ESP_LOGW(kTag, "install_wifi_namespace failed: %s", esp_err_to_name(wifi_ns_err));
        }
        ESP_LOGI(kTag, "QuickJS ready. Try: js status | js eval \"system.board\" | js eval \"wifi.status().state\" | js bench");
    } else {
        ESP_LOGE(kTag, "QuickJS service unavailable; console will still start");
    }

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "0103> ";
    repl_cfg.task_stack_size = 8192;
    esp_console_register_help_command();
    register_storage_commands();
    register_wifi_commands();
    register_js_commands(svc);

    esp_console_dev_usb_serial_jtag_config_t hw_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
