#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Control-plane events sent from console commands to the main loop.
enum class CtrlType : uint8_t {
    SetMode,     // arg0: 0=idle 1=tx 2=rx
    SetPin,      // arg0: 1 or 2
    Status,      // print status

    TxHigh,
    TxLow,
    TxStop,
    TxSquare,    // arg0: hz
    TxPulse,     // arg0: width_us, arg1: period_ms

    RxEdges,     // arg0: 0=rising 1=falling 2=both
    RxPull,      // arg0: 0=none 1=up 2=down
    RxReset,
};

struct CtrlEvent {
    CtrlType type;
    int32_t arg0;
    int32_t arg1;
};

// Create the control queue and install it as the global target for ctrl_send().
QueueHandle_t ctrl_init(void);

// Send an event to the control queue (drops if queue is full).
void ctrl_send(const CtrlEvent &ev);



