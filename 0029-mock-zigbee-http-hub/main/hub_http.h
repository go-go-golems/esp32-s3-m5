#pragma once

#include "esp_err.h"

esp_err_t hub_http_start(void);
void hub_http_stop(void);

// Broadcast a JSON message to any connected event-stream clients.
esp_err_t hub_http_events_broadcast_json(const char *json);

