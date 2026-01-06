#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t hub_http_start(void);
void hub_http_stop(void);

// Broadcast a protobuf message to any connected event-stream clients (binary WS frames).
esp_err_t hub_http_events_broadcast_pb(const uint8_t *data, size_t len);

// Best-effort client count (for gating work upstream).
size_t hub_http_events_client_count(void);
