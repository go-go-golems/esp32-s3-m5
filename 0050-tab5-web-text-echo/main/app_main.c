/*
 * Tab5 web text echo demo.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_err.h"
#include "esp_log.h"

#include "echo_state.h"
#include "http_server.h"
#include "wifi_app.h"
#include "wifi_console.h"

static const char *TAG = "tab5_text_echo_app";

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    ESP_ERROR_CHECK(echo_state_init());
    ESP_ERROR_CHECK(wifi_app_start());
    wifi_console_start();
    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "ready");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
