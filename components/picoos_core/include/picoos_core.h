#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "picojs_runtime.h"
#include "qjs_service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PICOOS_MAX_APPS 12
#define PICOOS_APP_ID_MAX 24
#define PICOOS_TITLE_MAX 40

typedef struct picoos_supervisor picoos_supervisor_t;

typedef enum {
    PICOOS_SURFACE_LAUNCHER = 0,
    PICOOS_SURFACE_APP,
    PICOOS_SURFACE_REPL,
    PICOOS_SURFACE_SWITCHER,
    PICOOS_SURFACE_CRASH,
} picoos_surface_t;

typedef enum {
    PICOOS_APP_STOPPED = 0,
    PICOOS_APP_STARTING,
    PICOOS_APP_RUNNING,
    PICOOS_APP_FOREGROUND,
    PICOOS_APP_BACKGROUND,
    PICOOS_APP_PAUSED,
    PICOOS_APP_CRASHED,
} picoos_app_state_t;

typedef struct {
    qjs_service_t *qjs;
    picojs_runtime_t *runtime;
    uint16_t cols;
    uint16_t rows;
    uint32_t default_fps;
    esp_err_t (*render_active)(void *user);
    esp_err_t (*render_repl)(void *user);
    void *render_user;
} picoos_supervisor_config_t;

typedef struct {
    const char *id;
    const char *title;
    const char *source;
    const char *filename;
    bool system;
    bool autostart;
    bool allow_background_ticks;
    uint32_t preferred_fps;
} picoos_app_descriptor_t;

typedef struct {
    bool initialized;
    bool running;
    picoos_surface_t surface;
    char active_app_id[PICOOS_APP_ID_MAX];
    uint16_t cols;
    uint16_t rows;
    uint32_t default_fps;
    uint32_t app_count;
    uint32_t frame_count;
    uint32_t error_count;
} picoos_status_t;

typedef struct {
    char id[PICOOS_APP_ID_MAX];
    char title[PICOOS_TITLE_MAX];
    picoos_app_state_t state;
    bool registered;
    bool system;
    bool autostart;
    bool allow_background_ticks;
    uint32_t preferred_fps;
    uint32_t frame_count;
    uint32_t error_count;
} picoos_app_info_t;

esp_err_t picoos_supervisor_create(const picoos_supervisor_config_t *cfg, picoos_supervisor_t **out);
void picoos_supervisor_destroy(picoos_supervisor_t *os);

esp_err_t picoos_register_app(picoos_supervisor_t *os, const picoos_app_descriptor_t *desc);
esp_err_t picoos_launch(picoos_supervisor_t *os, const char *app_id);
esp_err_t picoos_show_repl(picoos_supervisor_t *os);
esp_err_t picoos_start(picoos_supervisor_t *os, uint32_t fps);
esp_err_t picoos_stop(picoos_supervisor_t *os);
esp_err_t picoos_frame(picoos_supervisor_t *os, uint32_t dt_ms);
esp_err_t picoos_key(picoos_supervisor_t *os, const char *token);
esp_err_t picoos_get_status(picoos_supervisor_t *os, picoos_status_t *out);
esp_err_t picoos_list_apps(picoos_supervisor_t *os, picoos_app_info_t *out, size_t cap, size_t *count);

const char *picoos_surface_name(picoos_surface_t surface);
const char *picoos_app_state_name(picoos_app_state_t state);

#ifdef __cplusplus
}
#endif
