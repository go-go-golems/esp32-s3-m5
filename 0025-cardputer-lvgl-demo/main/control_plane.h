#pragma once

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

enum class CtrlType : uint8_t {
    ScreenshotPngToUsbSerialJtag = 1,
};

struct CtrlEvent {
    CtrlType type{};
    TaskHandle_t reply_task = nullptr;
};

QueueHandle_t ctrl_create_queue(size_t len);
bool ctrl_send(QueueHandle_t q, const CtrlEvent &ev, TickType_t wait);
bool ctrl_recv(QueueHandle_t q, CtrlEvent *out, TickType_t wait);

