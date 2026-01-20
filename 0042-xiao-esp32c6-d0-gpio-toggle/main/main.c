#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mo030_gpio_toggle";

static inline int mo030_level_for_state(bool state)
{
    int level = state ? 1 : 0;
#if CONFIG_MO030_TOGGLE_ACTIVE_LOW
    level = !level;
#endif
    return level;
}

void app_main(void)
{
    const gpio_num_t pin = (gpio_num_t)CONFIG_MO030_TOGGLE_GPIO_NUM;

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << (uint32_t)pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config(pin=%d) failed: %s", (int)pin, esp_err_to_name(err));
        return;
    }

    const TickType_t delay_ticks = pdMS_TO_TICKS(CONFIG_MO030_TOGGLE_PERIOD_MS);
    bool state = false;

    while (true) {
        state = !state;
        const int level = mo030_level_for_state(state);
        gpio_set_level(pin, level);
        ESP_LOGI(TAG, "gpio=%d level=%d", (int)pin, level);
        vTaskDelay(delay_ticks);
    }
}
