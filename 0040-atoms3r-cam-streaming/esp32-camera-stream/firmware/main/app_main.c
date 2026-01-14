#include "console.h"
#include "stream_client.h"
#include "wifi_sta.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "atoms3r_cam_stream";

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_err_t err = cam_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(err));
    }

    err = stream_client_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "stream client init failed: %s", esp_err_to_name(err));
    }

    cam_console_start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
