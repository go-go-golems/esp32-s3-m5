#pragma once

#include "esp_err.h"

// Start UART terminal bridge:
// - UART RX bytes are broadcast to websocket clients as binary frames
// - websocket binary frames are written to UART TX
esp_err_t uart_terminal_start(void);


