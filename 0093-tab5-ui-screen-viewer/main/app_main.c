/*
 * Tab5 UI Screen Viewer.
 *
 * Boot sequence:
 *   1. Initialize the MIPI DSI display (black screen + SPIRAM buffer)
 *   2. Start Wi-Fi (SoftAP always up; STA joins saved network)
 *   3. Start the esp_console REPL on USB Serial/JTAG
 *   4. Start the HTTP server with image upload endpoint
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include "esp_log.h"

#include "display_app.h"
#include "http_server.h"
#include "wifi_app.h"
#include "wifi_console.h"

static const char *TAG = "tab5_screen_viewer";

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    /* Display first — must be initialized before any LVGL calls. */
    ESP_ERROR_CHECK(display_app_init());

    /* Network services run in the background. */
    ESP_ERROR_CHECK(wifi_app_start());
    wifi_console_start();
    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "ready — screen viewer active, upload images via HTTP");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
