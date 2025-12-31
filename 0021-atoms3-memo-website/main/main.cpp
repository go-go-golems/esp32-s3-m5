/*
 * Ticket CLINTS-MEMO-WEBSITE: minimal boot strap.
 *
 * Current goal: prove out ESP-IDF build + SoftAP + HTTP server baseline.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "http_server.h"
#include "wifi.h"

static const char *TAG = "atoms3_memo_website_0021";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot");

    ESP_ERROR_CHECK(wifi_start());
    ESP_ERROR_CHECK(http_server_start());

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

