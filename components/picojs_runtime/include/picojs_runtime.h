#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs.h"
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PICOJS_RUNTIME_DEFAULT_COLS 40
#define PICOJS_RUNTIME_DEFAULT_ROWS 20

typedef struct picojs_runtime picojs_runtime_t;

typedef struct {
    uint16_t cols;              // default: 40
    uint16_t rows;              // default: 20
    uint32_t frame_interval_ms; // default: 100
} picojs_runtime_config_t;

typedef struct {
    bool initialized;
    uint16_t cols;
    uint16_t rows;
    bool js_installed;
    bool app_mode;
    uint32_t frame_count;
    uint32_t app_count;
    uint32_t mounted_app_count;
    uint32_t last_frame_ms;
    uint32_t last_error_count;
} picojs_runtime_status_t;

esp_err_t picojs_runtime_create(const picojs_runtime_config_t *cfg, picojs_runtime_t **out);
void picojs_runtime_destroy(picojs_runtime_t *rt);
esp_err_t picojs_runtime_install(JSContext *ctx, picojs_runtime_t *rt);
esp_err_t picojs_runtime_reset(picojs_runtime_t *rt);
esp_err_t picojs_runtime_get_status(picojs_runtime_t *rt, picojs_runtime_status_t *out);
esp_err_t picojs_runtime_frame(picojs_runtime_t *rt, uint32_t dt_ms);
esp_err_t picojs_runtime_frame_js(JSContext *ctx, picojs_runtime_t *rt, uint32_t dt_ms);
esp_err_t picojs_runtime_key(picojs_runtime_t *rt, const char *token);
esp_err_t picojs_runtime_key_js(JSContext *ctx, picojs_runtime_t *rt, const char *token);
esp_err_t picojs_runtime_take_launch_request(picojs_runtime_t *rt, char *dst, size_t dst_len);
esp_err_t picojs_runtime_set_app_mode(picojs_runtime_t *rt, bool enabled);
bool picojs_runtime_app_mode(picojs_runtime_t *rt);
esp_err_t picojs_runtime_dump_text(picojs_runtime_t *rt, char *dst, size_t dst_len);

#ifdef __cplusplus
}
#endif
