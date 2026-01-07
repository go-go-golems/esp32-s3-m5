#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"

#include "matrix_console.h"

static const char *TAG = "0036_matrix";

void app_main(void) {
    ESP_LOGI(TAG, "0036: Cardputer-ADV LED matrix console (MAX7219)");
    ESP_LOGI(TAG, "Console: start `esp_console` over USB Serial/JTAG (if enabled in sdkconfig)");
    matrix_console_start();
}

