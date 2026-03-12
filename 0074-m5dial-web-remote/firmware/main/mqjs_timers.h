#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mqjs_service mqjs_service_t;

esp_err_t mqjs_0074_timers_start(mqjs_service_t* svc);
void mqjs_0074_timers_stop(void);
esp_err_t mqjs_0074_timers_schedule(uint32_t id, uint32_t delay_ms);
esp_err_t mqjs_0074_timers_cancel(uint32_t id);
esp_err_t mqjs_0074_timers_cancel_all(void);

#ifdef __cplusplus
}
#endif
