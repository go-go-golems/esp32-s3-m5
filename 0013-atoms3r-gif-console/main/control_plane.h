#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Control-plane events sent from console commands / button ISR to the main loop.
enum class CtrlType : uint8_t {
    PlayIndex,
    Stop,
    Next,
    Info,
    SetBrightness,
};

struct CtrlEvent {
    CtrlType type;
    int32_t arg; // index or brightness
};

// Create the control queue and install it as the global control target for ctrl_send() and button ISR.
QueueHandle_t ctrl_init(void);

// Send an event to the control queue (drops if queue is full).
void ctrl_send(const CtrlEvent &ev);

// Button wiring -> CtrlType::Next via GPIO ISR.
void button_init(void);
void button_poll_debug_log(void);


