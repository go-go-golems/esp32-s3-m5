#pragma once

#include "esp_err.h"

esp_err_t http_server_start(void);

// Phase 2 (WebSocket): broadcast helpers.
// These are safe to call even if no clients are connected.
esp_err_t http_server_ws_broadcast_text(const char* text);
