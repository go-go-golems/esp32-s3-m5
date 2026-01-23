#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"

#include "sim_console.h"
#include "sim_engine.h"
#include "sim_ui.h"

static const char *TAG = "0066_ledsim";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());

    static sim_engine_t engine;
    ESP_ERROR_CHECK(sim_engine_init(
        &engine,
        (uint16_t)CONFIG_TUTORIAL_0066_LED_COUNT,
        (uint32_t)CONFIG_TUTORIAL_0066_FRAME_MS));

    sim_ui_start(&engine);
    sim_console_start(&engine);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

