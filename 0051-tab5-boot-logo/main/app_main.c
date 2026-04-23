/*
 * Tab5 boot logo display demo.
 *
 * Boot sequence:
 *   1. Initialize the MIPI DSI display and render the M5 factory logo
 *   2. Start Wi-Fi (SoftAP always up; STA joins saved network)
 *   3. Start the esp_console REPL on USB Serial/JTAG
 *   4. Start the HTTP echo server
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include "esp_log.h"

#include "display_app.h"
#include "echo_state.h"
#include "http_server.h"
#include "wifi_app.h"
#include "wifi_console.h"

static const char *TAG = "tab5_boot_logo";

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    /* Display first — must be initialized before any LVGL calls.
     * Renders the M5 factory logo on the 5-inch 720P MIPI DSI display. */
    ESP_ERROR_CHECK(display_app_init());
    /* Network services run in the background; logo stays on screen. */
    ESP_ERROR_CHECK(echo_state_init());
    ESP_ERROR_CHECK(wifi_app_start());
    wifi_console_start();
    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "ready — logo displayed, Wi-Fi running, HTTP echo server up");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
