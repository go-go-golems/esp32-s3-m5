#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "jtag_test";

void app_main(void)
{
    uint32_t tick = 0;

    ESP_LOGI(TAG, "boot: usb serial/jtag console test");

    while (true) {
        ESP_LOGI(TAG, "tick=%" PRIu32, tick++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
