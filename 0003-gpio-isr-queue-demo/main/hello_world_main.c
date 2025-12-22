/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "gpio_isr_queue_demo";

// Default to BOOT button GPIO on many ESP32-S3 dev boards.
// If your board uses a different GPIO, change this constant.
static const gpio_num_t BUTTON_GPIO = GPIO_NUM_0;

typedef struct {
    gpio_num_t gpio;
    int level;
    TickType_t tick;
} gpio_evt_t;

static QueueHandle_t gpio_evt_queue = NULL;

static void gpio_isr_handler(void *arg) {
    gpio_num_t gpio = (gpio_num_t)(intptr_t)arg;

    gpio_evt_t evt = {
        .gpio = gpio,
        .level = gpio_get_level(gpio),
        .tick = xTaskGetTickCountFromISR(),
    };

    BaseType_t higher_prio_task_woken = pdFALSE;
    (void)xQueueSendFromISR(gpio_evt_queue, &evt, &higher_prio_task_woken);
    if (higher_prio_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void gpio_event_task(void *arg) {
    (void)arg;

    gpio_evt_t evt;
    TickType_t last_tick = 0;
    uint32_t presses = 0;

    while (true) {
        if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        // Simple debounce: ignore events within 50ms.
        if ((evt.tick - last_tick) < pdMS_TO_TICKS(50)) {
            continue;
        }
        last_tick = evt.tick;

        presses++;
        ESP_LOGI(TAG,
                 "gpio=%d level=%d tick=%" PRIu32 " presses=%" PRIu32,
                 (int)evt.gpio,
                 evt.level,
                 (uint32_t)evt.tick,
                 presses);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "starting (button gpio=%d)", (int)BUTTON_GPIO);

    gpio_evt_queue = xQueueCreate(/* length */ 32, /* item size */ sizeof(gpio_evt_t));
    if (gpio_evt_queue == NULL) {
        ESP_LOGE(TAG, "failed to create queue");
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void *)(intptr_t)BUTTON_GPIO));

    BaseType_t ok = xTaskCreate(gpio_event_task, "gpio_evt", 4096, NULL, 10, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create gpio_evt task");
        return;
    }

    ESP_LOGI(TAG, "ready: press the BOOT button (or pull gpio=%d low) to generate interrupts", (int)BUTTON_GPIO);
}
