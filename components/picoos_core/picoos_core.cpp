#include "picoos_core.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "esp_log.h"

namespace {
constexpr const char *kTag = "picoos_core";
constexpr uint32_t kDefaultFps = 4;

struct PicoOsAppRecord {
    picoos_app_info_t info = {};
    const char *source = nullptr;
    const char *filename = nullptr;
};

void copy_cstr(char *dst, size_t dst_len, const char *src)
{
    if (!dst || dst_len == 0) return;
    if (!src) src = "";
    std::snprintf(dst, dst_len, "%s", src);
}

PicoOsAppRecord *find_app(struct picoos_supervisor *os, const char *id);
}  // namespace

struct picoos_supervisor {
    bool initialized = false;
    bool running = false;
    picoos_surface_t surface = PICOOS_SURFACE_REPL;
    char active_app_id[PICOOS_APP_ID_MAX] = {};
    uint16_t cols = PICOJS_RUNTIME_DEFAULT_COLS;
    uint16_t rows = PICOJS_RUNTIME_DEFAULT_ROWS;
    uint32_t default_fps = kDefaultFps;
    uint32_t frame_count = 0;
    uint32_t error_count = 0;
    qjs_service_t *qjs = nullptr;
    picojs_runtime_t *runtime = nullptr;
    PicoOsAppRecord apps[PICOOS_MAX_APPS] = {};
    size_t app_count = 0;
};

namespace {
PicoOsAppRecord *find_app(picoos_supervisor *os, const char *id)
{
    if (!os || !id || id[0] == 0) return nullptr;
    for (size_t i = 0; i < os->app_count; ++i) {
        if (std::strcmp(os->apps[i].info.id, id) == 0) return &os->apps[i];
    }
    return nullptr;
}

}  // namespace

const char *picoos_surface_name(picoos_surface_t surface)
{
    switch (surface) {
        case PICOOS_SURFACE_LAUNCHER: return "launcher";
        case PICOOS_SURFACE_APP: return "app";
        case PICOOS_SURFACE_REPL: return "repl";
        case PICOOS_SURFACE_SWITCHER: return "switcher";
        case PICOOS_SURFACE_CRASH: return "crash";
        default: return "unknown";
    }
}

const char *picoos_app_state_name(picoos_app_state_t state)
{
    switch (state) {
        case PICOOS_APP_STOPPED: return "stopped";
        case PICOOS_APP_STARTING: return "starting";
        case PICOOS_APP_RUNNING: return "running";
        case PICOOS_APP_FOREGROUND: return "foreground";
        case PICOOS_APP_BACKGROUND: return "background";
        case PICOOS_APP_PAUSED: return "paused";
        case PICOOS_APP_CRASHED: return "crashed";
        default: return "unknown";
    }
}

esp_err_t picoos_supervisor_create(const picoos_supervisor_config_t *cfg, picoos_supervisor_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = nullptr;
    if (!cfg || !cfg->qjs || !cfg->runtime) return ESP_ERR_INVALID_ARG;

    auto *os = new picoos_supervisor();
    if (!os) return ESP_ERR_NO_MEM;
    os->qjs = cfg->qjs;
    os->runtime = cfg->runtime;
    os->cols = cfg->cols ? cfg->cols : PICOJS_RUNTIME_DEFAULT_COLS;
    os->rows = cfg->rows ? cfg->rows : PICOJS_RUNTIME_DEFAULT_ROWS;
    os->default_fps = cfg->default_fps ? cfg->default_fps : kDefaultFps;
    os->initialized = true;
    os->surface = PICOOS_SURFACE_REPL;
    *out = os;
    ESP_LOGI(kTag, "supervisor initialized: %ux%u default_fps=%u", os->cols, os->rows, (unsigned)os->default_fps);
    return ESP_OK;
}

void picoos_supervisor_destroy(picoos_supervisor_t *os)
{
    delete os;
}

esp_err_t picoos_register_app(picoos_supervisor_t *os, const picoos_app_descriptor_t *desc)
{
    if (!os || !os->initialized || !desc || !desc->id || desc->id[0] == 0) return ESP_ERR_INVALID_ARG;
    if (std::strlen(desc->id) >= PICOOS_APP_ID_MAX) return ESP_ERR_INVALID_ARG;
    if (find_app(os, desc->id)) return ESP_ERR_INVALID_STATE;
    if (os->app_count >= PICOOS_MAX_APPS) return ESP_ERR_NO_MEM;

    PicoOsAppRecord &record = os->apps[os->app_count++];
    record.info = {};
    copy_cstr(record.info.id, sizeof(record.info.id), desc->id);
    copy_cstr(record.info.title, sizeof(record.info.title), desc->title ? desc->title : desc->id);
    record.info.state = PICOOS_APP_STOPPED;
    record.info.registered = true;
    record.info.system = desc->system;
    record.info.autostart = desc->autostart;
    record.info.allow_background_ticks = desc->allow_background_ticks;
    record.info.preferred_fps = desc->preferred_fps ? desc->preferred_fps : os->default_fps;
    record.source = desc->source;
    record.filename = desc->filename ? desc->filename : desc->id;
    ESP_LOGI(kTag, "registered app id=%s title=%s system=%d fps=%u", record.info.id, record.info.title,
             record.info.system, (unsigned)record.info.preferred_fps);
    return ESP_OK;
}

esp_err_t picoos_get_status(picoos_supervisor_t *os, picoos_status_t *out)
{
    if (!os || !out) return ESP_ERR_INVALID_ARG;
    *out = {};
    out->initialized = os->initialized;
    out->running = os->running;
    out->surface = os->surface;
    copy_cstr(out->active_app_id, sizeof(out->active_app_id), os->active_app_id);
    out->cols = os->cols;
    out->rows = os->rows;
    out->default_fps = os->default_fps;
    out->app_count = static_cast<uint32_t>(os->app_count);
    out->frame_count = os->frame_count;
    out->error_count = os->error_count;
    return ESP_OK;
}

esp_err_t picoos_list_apps(picoos_supervisor_t *os, picoos_app_info_t *out, size_t cap, size_t *count)
{
    if (!os || (!out && cap > 0)) return ESP_ERR_INVALID_ARG;
    const size_t n = std::min(cap, os->app_count);
    for (size_t i = 0; i < n; ++i) out[i] = os->apps[i].info;
    if (count) *count = os->app_count;
    return cap < os->app_count ? ESP_ERR_NO_MEM : ESP_OK;
}
