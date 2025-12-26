/*
 * Tutorial 0017: Button input (GPIO ISR -> queue -> WebSocket JSON).
 */

#include "button_input.h"

#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "http_server.h"

static const char *TAG = "atoms3r_web_ui_0017";

typedef struct {
    uint32_t ts_ms;
} button_event_t;

static QueueHandle_t s_btn_q = nullptr;
static bool s_started = false;

static void IRAM_ATTR button_isr(void *arg) {
    (void)arg;
    if (!s_btn_q) return;
    button_event_t ev = {.ts_ms = (uint32_t)(esp_timer_get_time() / 1000)};
    BaseType_t hp_task_woken = pdFALSE;
    xQueueSendFromISR(s_btn_q, &ev, &hp_task_woken);
    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}

static void button_task(void *arg) {
    (void)arg;
    uint32_t last_ms = 0;
    while (true) {
        button_event_t ev = {};
        if (xQueueReceive(s_btn_q, &ev, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        const uint32_t dt = ev.ts_ms - last_ms;
        if (dt < (uint32_t)CONFIG_TUTORIAL_0017_BUTTON_DEBOUNCE_MS) {
            continue;
        }
        last_ms = ev.ts_ms;

        char msg[128];
        const int n = snprintf(msg, sizeof(msg),
                               "{\"type\":\"button\",\"ts_ms\":%" PRIu32 "}",
                               ev.ts_ms);
        if (n > 0 && n < (int)sizeof(msg)) {
            (void)http_server_ws_broadcast_text(msg);
        }
    }
}

esp_err_t button_input_start(void) {
    if (s_started) return ESP_OK;

    s_btn_q = xQueueCreate(16, sizeof(button_event_t));
    if (!s_btn_q) return ESP_ERR_NO_MEM;

    const gpio_num_t pin = (gpio_num_t)CONFIG_TUTORIAL_0017_BUTTON_GPIO;
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << (int)pin;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
#if CONFIG_TUTORIAL_0017_BUTTON_ACTIVE_LOW
    io.intr_type = GPIO_INTR_NEGEDGE;
#else
    io.intr_type = GPIO_INTR_POSEDGE;
#endif
    ESP_ERROR_CHECK(gpio_config(&io));

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(pin, button_isr, nullptr));

    ESP_LOGI(TAG, "button init: gpio=%d active_low=%d debounce_ms=%d",
             (int)pin,
             (int)(0
#if CONFIG_TUTORIAL_0017_BUTTON_ACTIVE_LOW
                   + 1
#endif
                   ),
             CONFIG_TUTORIAL_0017_BUTTON_DEBOUNCE_MS);

    xTaskCreate(button_task, "btn_ws", 3072, nullptr, 10, nullptr);

    s_started = true;
    return ESP_OK;
}


