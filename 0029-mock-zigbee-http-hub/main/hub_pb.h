#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

// Generated nanopb types for 0029 hub events.
#include "hub_events.pb.h"

// Registers an esp_event handler that can (optionally) capture hub events and
// store the most recent protobuf envelope for later encoding.
esp_err_t hub_pb_register(esp_event_loop_handle_t loop);

// Control capture and query current state.
void hub_pb_set_capture(bool enabled);
void hub_pb_get_status(bool *out_capture_enabled, bool *out_have_last);

// Encode the last captured event into `buf`.
esp_err_t hub_pb_encode_last(uint8_t *buf, size_t cap, size_t *out_len);

// Builds a protobuf envelope for a hub bus event.
// Returns true if the event is recognized and `out` was filled.
bool hub_pb_build_event(int32_t id, const void *data, hub_v1_HubEvent *out);
