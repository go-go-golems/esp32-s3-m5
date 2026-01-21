#pragma once

#include "esp_err.h"

// Phase 2: best-effort background task that reads an encoder backend and broadcasts
// telemetry over WebSocket as JSON text frames.
//
// Safe to call multiple times.
esp_err_t encoder_telemetry_start(void);

