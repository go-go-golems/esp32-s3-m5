#include "picoos_core.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char *kTag = "picoos_core";
constexpr uint32_t kDefaultFps = 4;
constexpr uint32_t kFrameJobTimeoutMs = 1000;
constexpr uint32_t kFrameTaskStackWords = 4096;
constexpr UBaseType_t kFrameTaskPriority = 4;

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
esp_err_t install_picojs_job(JSContext *ctx, void *user);
esp_err_t picoos_frame_job(JSContext *ctx, void *user);
esp_err_t picoos_key_job(JSContext *ctx, void *user);
void picoos_frame_task(void *user);
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
    uint32_t frame_period_ms = 250;
    uint32_t pending_dt_ms = 0;
    char pending_key[16] = {};
    bool stop_requested = false;
    TaskHandle_t frame_task = nullptr;
    esp_err_t (*render_active)(void *user) = nullptr;
    esp_err_t (*render_repl)(void *user) = nullptr;
    void *render_user = nullptr;
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

void mark_foreground(picoos_supervisor *os, PicoOsAppRecord *active)
{
    if (!os || !active) return;
    for (size_t i = 0; i < os->app_count; ++i) {
        PicoOsAppRecord &record = os->apps[i];
        if (&record == active) {
            record.info.state = PICOOS_APP_FOREGROUND;
        } else if (record.info.state == PICOOS_APP_FOREGROUND || record.info.state == PICOOS_APP_RUNNING) {
            record.info.state = PICOOS_APP_BACKGROUND;
        }
    }
    copy_cstr(os->active_app_id, sizeof(os->active_app_id), active->info.id);
}

esp_err_t install_picojs_job(JSContext *ctx, void *user)
{
    auto *os = static_cast<picoos_supervisor *>(user);
    if (!ctx || !os || !os->runtime) return ESP_ERR_INVALID_ARG;
    return picojs_runtime_install(ctx, os->runtime);
}

esp_err_t ensure_picojs_installed(picoos_supervisor *os)
{
    qjs_job_t job = {};
    job.fn = install_picojs_job;
    job.user = os;
    job.timeout_ms = kFrameJobTimeoutMs;
    return qjs_service_run(os->qjs, &job);
}

esp_err_t picoos_frame_job(JSContext *ctx, void *user)
{
    auto *os = static_cast<picoos_supervisor *>(user);
    if (!ctx || !os || !os->runtime) return ESP_ERR_INVALID_ARG;
    return picojs_runtime_frame_js(ctx, os->runtime, os->pending_dt_ms);
}

esp_err_t picoos_key_job(JSContext *ctx, void *user)
{
    auto *os = static_cast<picoos_supervisor *>(user);
    if (!ctx || !os || !os->runtime || os->pending_key[0] == 0) return ESP_ERR_INVALID_ARG;
    return picojs_runtime_key_js(ctx, os->runtime, os->pending_key);
}

void picoos_frame_task(void *user)
{
    auto *os = static_cast<picoos_supervisor *>(user);
    int64_t last_us = esp_timer_get_time();
    while (os && !os->stop_requested) {
        vTaskDelay(pdMS_TO_TICKS(os->frame_period_ms ? os->frame_period_ms : 250));
        if (!os->running) {
            last_us = esp_timer_get_time();
            continue;
        }
        const int64_t now_us = esp_timer_get_time();
        uint32_t dt_ms = static_cast<uint32_t>((now_us - last_us) / 1000);
        if (dt_ms == 0) dt_ms = os->frame_period_ms ? os->frame_period_ms : 1;
        last_us = now_us;
        esp_err_t err = picoos_frame(os, dt_ms);
        if (err != ESP_OK) {
            ESP_LOGW(kTag, "frame pump failed: %s", esp_err_to_name(err));
        }
    }
    if (os) os->frame_task = nullptr;
    vTaskDelete(nullptr);
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
    os->frame_period_ms = 1000 / os->default_fps;
    os->render_active = cfg->render_active;
    os->render_repl = cfg->render_repl;
    os->render_user = cfg->render_user;
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

esp_err_t picoos_launch(picoos_supervisor_t *os, const char *app_id)
{
    if (!os || !os->initialized || !app_id || app_id[0] == 0) return ESP_ERR_INVALID_ARG;
    PicoOsAppRecord *record = find_app(os, app_id);
    if (!record) return ESP_ERR_NOT_FOUND;

    if (!record->source) {
        mark_foreground(os, record);
        os->surface = PICOOS_SURFACE_REPL;
        ESP_LOGI(kTag, "show native app id=%s surface=%s", record->info.id, picoos_surface_name(os->surface));
        return ESP_OK;
    }

    record->info.state = PICOOS_APP_STARTING;
    esp_err_t install_err = ensure_picojs_installed(os);
    if (install_err != ESP_OK) {
        record->info.state = PICOOS_APP_CRASHED;
        ++record->info.error_count;
        ++os->error_count;
        return install_err;
    }

    qjs_eval_result_t result = {};
    esp_err_t err = qjs_service_eval(os->qjs, record->source, std::strlen(record->source), 1000, record->filename, &result);
    const bool ok = err == ESP_OK && result.ok && !result.timed_out;
    if (ok) {
        mark_foreground(os, record);
        os->surface = PICOOS_SURFACE_APP;
        ESP_LOGI(kTag, "launched app id=%s elapsed=%ums", record->info.id, (unsigned)result.elapsed_ms);
    } else {
        record->info.state = PICOOS_APP_CRASHED;
        ++record->info.error_count;
        ++os->error_count;
        ESP_LOGW(kTag, "launch failed id=%s err=%s ok=%d timeout=%d error=%s", record->info.id,
                 esp_err_to_name(err), result.ok, result.timed_out, result.error ? result.error : "");
    }
    qjs_eval_result_free(&result);
    return ok ? ESP_OK : (err == ESP_OK ? ESP_FAIL : err);
}

esp_err_t picoos_show_repl(picoos_supervisor_t *os)
{
    if (!os || !os->initialized) return ESP_ERR_INVALID_ARG;
    PicoOsAppRecord *record = find_app(os, "repl");
    if (record) {
        mark_foreground(os, record);
    } else {
        os->active_app_id[0] = 0;
    }
    os->surface = PICOOS_SURFACE_REPL;
    if (os->render_repl) return os->render_repl(os->render_user);
    return ESP_OK;
}

esp_err_t picoos_start(picoos_supervisor_t *os, uint32_t fps)
{
    if (!os || !os->initialized) return ESP_ERR_INVALID_ARG;
    if (fps == 0) fps = os->default_fps ? os->default_fps : kDefaultFps;
    if (fps > 60) return ESP_ERR_INVALID_ARG;
    os->frame_period_ms = std::max<uint32_t>(1, 1000 / fps);
    os->running = true;
    if (!os->frame_task) {
        os->stop_requested = false;
        BaseType_t ok = xTaskCreate(picoos_frame_task, "picoos_frame", kFrameTaskStackWords, os,
                                    kFrameTaskPriority, &os->frame_task);
        if (ok != pdPASS) {
            os->running = false;
            os->frame_task = nullptr;
            return ESP_ERR_NO_MEM;
        }
    }
    ESP_LOGI(kTag, "frame pump started fps=%u period=%ums", (unsigned)fps, (unsigned)os->frame_period_ms);
    return ESP_OK;
}

esp_err_t picoos_stop(picoos_supervisor_t *os)
{
    if (!os || !os->initialized) return ESP_ERR_INVALID_ARG;
    os->running = false;
    ESP_LOGI(kTag, "frame pump stopped");
    return ESP_OK;
}

esp_err_t picoos_frame(picoos_supervisor_t *os, uint32_t dt_ms)
{
    if (!os || !os->initialized) return ESP_ERR_INVALID_ARG;
    if (os->surface != PICOOS_SURFACE_APP || os->active_app_id[0] == 0) return ESP_OK;
    PicoOsAppRecord *active = find_app(os, os->active_app_id);
    if (!active || !active->source) return ESP_OK;

    os->pending_dt_ms = dt_ms;
    qjs_job_t job = {};
    job.fn = picoos_frame_job;
    job.user = os;
    job.timeout_ms = kFrameJobTimeoutMs;
    esp_err_t err = qjs_service_run(os->qjs, &job);
    os->pending_dt_ms = 0;
    if (err == ESP_OK) {
        ++os->frame_count;
        ++active->info.frame_count;
        if (os->render_active) {
            esp_err_t render_err = os->render_active(os->render_user);
            if (render_err != ESP_OK) return render_err;
        }
    } else {
        ++os->error_count;
        ++active->info.error_count;
        active->info.state = PICOOS_APP_CRASHED;
    }
    return err;
}

esp_err_t picoos_key(picoos_supervisor_t *os, const char *token)
{
    if (!os || !os->initialized || !token || token[0] == 0) return ESP_ERR_INVALID_ARG;
    if (std::strcmp(token, "home") == 0) {
        esp_err_t err = picoos_launch(os, "home");
        if (err == ESP_OK && os->render_active) return os->render_active(os->render_user);
        return err;
    }
    if (std::strcmp(token, "escape") == 0 || std::strcmp(token, "repl") == 0) {
        return picoos_show_repl(os);
    }
    if (os->surface != PICOOS_SURFACE_APP || os->active_app_id[0] == 0) return ESP_OK;
    PicoOsAppRecord *active = find_app(os, os->active_app_id);
    if (!active || !active->source) return ESP_OK;

    copy_cstr(os->pending_key, sizeof(os->pending_key), token);
    qjs_job_t job = {};
    job.fn = picoos_key_job;
    job.user = os;
    job.timeout_ms = kFrameJobTimeoutMs;
    esp_err_t err = qjs_service_run(os->qjs, &job);
    os->pending_key[0] = 0;
    if (err == ESP_OK) {
        char requested[PICOOS_APP_ID_MAX] = {};
        if (picojs_runtime_take_launch_request(os->runtime, requested, sizeof(requested)) == ESP_OK && requested[0]) {
            err = picoos_launch(os, requested);
            if (err != ESP_OK) return err;
        }
        if (os->render_active) return os->render_active(os->render_user);
    } else {
        ++os->error_count;
        ++active->info.error_count;
        active->info.state = PICOOS_APP_CRASHED;
    }
    return err;
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
