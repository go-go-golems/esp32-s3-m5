#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "esp_err.h"

#include "sim_engine.h"

// JS service: single MicroQuickJS VM owner task.
//
// All MicroQuickJS calls (JS_Eval/JS_Call/etc) happen on the service task.
// Other tasks communicate via a bounded queue and synchronous request objects.

esp_err_t js_service_start(sim_engine_t* engine);
void js_service_stop(void);

esp_err_t js_service_reset(sim_engine_t* engine);

// Evaluate JS and return output and error (if any). Both strings include a trailing '\n' when present.
esp_err_t js_service_eval(const char* code,
                          size_t code_len,
                          uint32_t timeout_ms,
                          const char* filename,
                          std::string* out,
                          std::string* err);

// Dump VM memory stats (same output as JS_DumpMemory).
esp_err_t js_service_dump_memory(std::string* out);

// Evaluate JS and return a JSON response string:
// {"ok":true|false,"output":"...","error":null|"...","timed_out":true|false}
//
// This is intended for HTTP handlers; it avoids any JSON parsing on the device.
std::string js_service_eval_to_json(const char* code,
                                   size_t code_len,
                                   uint32_t timeout_ms,
                                   const char* filename);
