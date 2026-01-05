#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

// Starts the hub event stream bridge (protobuf over WebSocket).
// Safe to call even when the feature is disabled by Kconfig; it becomes a no-op.
esp_err_t hub_stream_start(esp_event_loop_handle_t loop);

// Basic telemetry about the stream bridge.
void hub_stream_get_stats(uint32_t *out_enqueue_drops, uint32_t *out_encode_failures, uint32_t *out_send_failures);

