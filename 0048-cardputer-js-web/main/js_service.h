#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "esp_err.h"

// JS service: single MicroQuickJS VM owner task.
//
// All MicroQuickJS calls (JS_Eval/JS_Call/etc) happen on the service task.
// Other tasks communicate via a bounded queue and synchronous request objects.

esp_err_t js_service_start(void);

// Evaluate JS and return a JSON string:
//   { ok, output, error, timed_out }
//
// `timeout_ms==0` uses CONFIG_TUTORIAL_0048_JS_TIMEOUT_MS.
std::string js_service_eval_to_json(const char* code, size_t code_len, uint32_t timeout_ms, const char* filename);

// Phase 2B hooks (no-ops until implemented in js_service.cpp).
esp_err_t js_service_post_encoder_click(int kind);  // kind: 0 single, 1 double, 2 long
esp_err_t js_service_update_encoder_delta(int32_t pos, int32_t delta);

