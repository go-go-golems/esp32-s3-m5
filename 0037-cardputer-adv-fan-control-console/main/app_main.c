#include <stdbool.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"

#include "fan_console.h"
#include "fan_relay.h"

static const char *TAG = "0037_fan";

void app_main(void) {
    ESP_LOGI(TAG, "0037: Cardputer-ADV fan relay control (GPIO3)");

    const esp_err_t err = fan_relay_init(FAN_RELAY_DEFAULT_GPIO, FAN_RELAY_DEFAULT_ACTIVE_HIGH, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "fan_relay_init failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Console: start `esp_console` over USB Serial/JTAG (if enabled in sdkconfig)");
    fan_console_start();
}

