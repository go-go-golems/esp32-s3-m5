#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "mo032_ws281x";

void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());
    ESP_LOGI(TAG, "TODO: implement WS281x patterns + esp_console REPL");
}

