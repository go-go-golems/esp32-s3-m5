#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Start an esp_console REPL over USB-Serial/JTAG, registering the demo's commands.
// Commands enqueue CtrlEvents into the provided queue; the UI thread owns execution.
void console_start(QueueHandle_t ctrl_queue);

