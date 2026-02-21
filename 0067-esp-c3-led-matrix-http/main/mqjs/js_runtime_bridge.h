#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool started;
    bool busy;
    bool stop_requested;
    bool last_timed_out;
    uint32_t eval_count;
    uint32_t last_eval_ms;
    uint32_t timer_cb_keys;
    uint32_t timer_cb_active;
    uint32_t timer_cb_keys_high_water;
    uint32_t animations_registered;
    char active_animation[32];
    uint32_t heap_free_8bit;
    uint32_t heap_largest_free_8bit;
    uint32_t heap_min_free_8bit;
    char last_error[128];
} js_service_status_t;

esp_err_t js_service_start(void);
void js_service_stop(void);
esp_err_t js_service_reset(void);
esp_err_t js_service_hard_reset(void);
esp_err_t js_service_request_stop(void);

esp_err_t js_service_eval_json(const char *code,
                               size_t code_len,
                               uint32_t timeout_ms,
                               const char *filename,
                               char **out_json);

esp_err_t js_service_dump_memory_text(char **out_text);
esp_err_t js_service_get_status(js_service_status_t *out);

void js_service_free(char *p);

#ifdef __cplusplus
}  // extern "C"
#endif
