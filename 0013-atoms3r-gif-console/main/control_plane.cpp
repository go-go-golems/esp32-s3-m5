/*
 * Tutorial 0013 (AtomS3R GIF console): control plane.
 *
 * A small queue-based event bus:
 * - console commands enqueue CtrlEvent
 * - button ISR enqueues CtrlEvent::Next
 * - app_main consumes CtrlEvent and mutates playback state
 */

#include <inttypes.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "control_plane.h"

static const char *TAG = "atoms3r_gif_console";

static QueueHandle_t s_ctrl_q = nullptr;

QueueHandle_t ctrl_init(void) {
    if (s_ctrl_q) {
        return s_ctrl_q;
    }
    s_ctrl_q = xQueueCreate(16, sizeof(CtrlEvent));
    if (!s_ctrl_q) {
        ESP_LOGE(TAG, "failed to create control queue");
    }
    return s_ctrl_q;
}

void ctrl_send(const CtrlEvent &ev) {
    if (!s_ctrl_q) return;
    (void)xQueueSend(s_ctrl_q, &ev, 0);
}

static void IRAM_ATTR button_isr(void *arg) {
    (void)arg;
    if (!s_ctrl_q) return;
    CtrlEvent ev = {.type = CtrlType::Next, .arg = 0};
    BaseType_t hp_task_woken = pdFALSE;
    xQueueSendFromISR(s_ctrl_q, &ev, &hp_task_woken);
    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}

void button_init(void) {
    const gpio_num_t pin = (gpio_num_t)CONFIG_TUTORIAL_0013_BUTTON_GPIO;
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << (int)pin;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
#if CONFIG_TUTORIAL_0013_BUTTON_ACTIVE_LOW
    io.intr_type = GPIO_INTR_NEGEDGE;
#else
    io.intr_type = GPIO_INTR_POSEDGE;
#endif

    ESP_ERROR_CHECK(gpio_config(&io));

    // NOTE: If another module already installed the ISR service, INVALID_STATE is OK.
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(pin, button_isr, nullptr));

    ESP_LOGI(TAG, "button init: gpio=%d active_low=%d debounce_ms=%d",
             (int)pin,
             (int)(0
#if CONFIG_TUTORIAL_0013_BUTTON_ACTIVE_LOW
                   + 1
#endif
                   ),
             CONFIG_TUTORIAL_0013_BUTTON_DEBOUNCE_MS);
}

void button_poll_debug_log(void) {
    const gpio_num_t pin = (gpio_num_t)CONFIG_TUTORIAL_0013_BUTTON_GPIO;
    int level = gpio_get_level(pin);
#if CONFIG_TUTORIAL_0013_BUTTON_ACTIVE_LOW
    const int pressed = (level == 0) ? 1 : 0;
#else
    const int pressed = (level != 0) ? 1 : 0;
#endif
    ESP_LOGI(TAG, "button poll: gpio=%d level=%d pressed=%d", (int)pin, level, pressed);
}


