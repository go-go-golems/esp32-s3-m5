/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <stdio.h>

#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "freertos_queue_demo";

typedef struct {
    uint32_t seq;
    uint32_t produced_ms;
} demo_msg_t;

static QueueHandle_t demo_queue = NULL;

static uint32_t now_ms(void) {
    return (uint32_t)(esp_log_timestamp());
}

static void producer_task(void *arg) {
    (void)arg;
    uint32_t seq = 0;

    while (true) {
        demo_msg_t msg = {
            .seq = seq++,
            .produced_ms = now_ms(),
        };

        // Block up to 100ms if the queue is full.
        if (xQueueSend(demo_queue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "producer: queue full, dropping seq=%" PRIu32, msg.seq);
        } else {
            ESP_LOGI(TAG, "producer: sent seq=%" PRIu32, msg.seq);
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void consumer_task(void *arg) {
    (void)arg;
    demo_msg_t msg;

    while (true) {
        // Wait forever for a message.
        if (xQueueReceive(demo_queue, &msg, portMAX_DELAY) == pdTRUE) {
            uint32_t age_ms = now_ms() - msg.produced_ms;
            ESP_LOGI(TAG, "consumer: got seq=%" PRIu32 " age=%" PRIu32 "ms", msg.seq, age_ms);
        }
    }
}

void app_main(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "boot: cores=%d rev=%d", chip_info.cores, chip_info.revision);

    demo_queue = xQueueCreate(/* queue length */ 8, /* item size */ sizeof(demo_msg_t));
    if (demo_queue == NULL) {
        ESP_LOGE(TAG, "failed to create queue");
        return;
    }

    BaseType_t ok1 = xTaskCreate(producer_task, "producer", 4096, NULL, 5, NULL);
    BaseType_t ok2 = xTaskCreate(consumer_task, "consumer", 4096, NULL, 5, NULL);

    if (ok1 != pdPASS || ok2 != pdPASS) {
        ESP_LOGE(TAG, "failed to create tasks (producer=%d consumer=%d)", (int)ok1, (int)ok2);
        return;
    }

    ESP_LOGI(TAG, "started; check logs for producer/consumer messages");
}
