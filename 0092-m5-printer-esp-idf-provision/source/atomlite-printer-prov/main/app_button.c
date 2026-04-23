#include "app_button.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t app_button_init(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << ATOM_LITE_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&config);
}

bool app_button_is_pressed(void)
{
    return gpio_get_level(ATOM_LITE_BUTTON_GPIO) == 0;
}

bool app_button_pressed_for(uint32_t duration_ms)
{
    static bool was_pressed = false;
    static TickType_t press_started = 0;

    const bool pressed = app_button_is_pressed();
    const TickType_t now = xTaskGetTickCount();

    if (pressed && !was_pressed) {
        was_pressed = true;
        press_started = now;
    } else if (!pressed && was_pressed) {
        was_pressed = false;
    }

    if (!was_pressed) {
        return false;
    }

    const uint32_t elapsed_ms = (uint32_t)((now - press_started) * portTICK_PERIOD_MS);
    if (elapsed_ms >= duration_ms) {
        was_pressed = false;
        return true;
    }

    return false;
}
