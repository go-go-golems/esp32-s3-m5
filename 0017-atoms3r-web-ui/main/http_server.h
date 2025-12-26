#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

// Start the HTTP server (port 80) and register routes.
esp_err_t http_server_start(void);

// WebSocket helpers (server → browser).
esp_err_t http_server_ws_broadcast_binary(const uint8_t *data, size_t len);
esp_err_t http_server_ws_broadcast_text(const char *text);

// WebSocket RX hook (browser → device).
typedef esp_err_t (*http_server_ws_binary_rx_cb_t)(const uint8_t *data, size_t len);
void http_server_ws_set_binary_rx_cb(http_server_ws_binary_rx_cb_t cb);


