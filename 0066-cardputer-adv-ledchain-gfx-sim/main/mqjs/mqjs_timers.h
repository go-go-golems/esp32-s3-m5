#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "mqjs_service.h"

#ifdef __cplusplus
extern "C" {
#endif

// One-shot timer scheduler for MicroQuickJS.
//
// Invariants:
// - Never executes JS in timer task context.
// - On expiry, posts a job to `mqjs_service` which calls back into JS.

esp_err_t mqjs_0066_timers_start(mqjs_service_t* svc);
void mqjs_0066_timers_stop(void);

esp_err_t mqjs_0066_timers_schedule(uint32_t id, uint32_t delay_ms);
esp_err_t mqjs_0066_timers_cancel(uint32_t id);

#ifdef __cplusplus
}  // extern "C"
#endif

