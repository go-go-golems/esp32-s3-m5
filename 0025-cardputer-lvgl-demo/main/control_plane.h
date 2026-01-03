#pragma once

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

enum class CtrlType : uint8_t {
    OpenMenu = 1,
    OpenBasics = 2,
    OpenPomodoro = 3,
    PomodoroSetMinutes = 4,
    ScreenshotPngToUsbSerialJtag = 5,
    OpenSplitConsole = 6,
    TogglePalette = 7,
    OpenSystemMonitor = 8,
    OpenFileBrowser = 9,
    InjectKeys = 10,
};

struct CtrlEvent {
    CtrlType type{};
    int32_t arg = 0;
    TaskHandle_t reply_task = nullptr;
    void *ptr = nullptr;
};

QueueHandle_t ctrl_create_queue(size_t len);
bool ctrl_send(QueueHandle_t q, const CtrlEvent &ev, TickType_t wait);
bool ctrl_recv(QueueHandle_t q, CtrlEvent *out, TickType_t wait);
