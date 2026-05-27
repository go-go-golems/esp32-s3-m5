/*
 * Tab5 WiFi/HTTP benchmark firmware.
 *
 * Boot sequence:
 *   1. Start Wi-Fi (SoftAP always up; STA joins saved network)
 *   2. Start the esp_console REPL on USB Serial/JTAG
 *   3. Start the HTTP server with benchmark endpoints
 *
 * No display initialization — this firmware is headless.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include "esp_log.h"

#include "bench_server.h"
#include "wifi_app.h"
#include "wifi_console.h"

static const char *TAG = "tab5_bench";

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    /* Network services run in the background. */
    ESP_ERROR_CHECK(wifi_app_start());
    wifi_console_start();
    ESP_ERROR_CHECK(bench_server_start());

    ESP_LOGI(TAG, "ready — benchmark server active");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
