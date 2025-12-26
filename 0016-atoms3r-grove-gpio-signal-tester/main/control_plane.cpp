/*
 * Tutorial 0016 (AtomS3R GROVE GPIO signal tester): control plane.
 *
 * A small queue-based event bus:
 * - console commands enqueue CtrlEvent
 * - app_main consumes CtrlEvent and mutates tester state
 */

#include "control_plane.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

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



