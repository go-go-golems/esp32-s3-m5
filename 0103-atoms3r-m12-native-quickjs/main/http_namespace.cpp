// http_namespace.cpp — ESP-IDF wrapper for the shared QuickJS HTTP/fetch core.
#include "http_namespace.h"

#include <cstdlib>
#include <cstring>
#include <new>

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "http_namespace_core.h"
#include "http_server.h"

namespace {
constexpr const char *kTag = "0103_http_ns";
constexpr uint32_t kInstallTimeoutMs = 1000;
constexpr size_t kFetchChunkBytes = 512;
constexpr size_t kMaxPendingFetches = 4;
constexpr uint32_t kFetchSettleTimeoutMs = 1000;
constexpr int kMaxFetchSettleJobs = 64;

qjs_http::Runtime *s_runtime = nullptr;
qjs_service_t *s_fetch_service = nullptr;
QueueHandle_t s_fetch_queue = nullptr;
TaskHandle_t s_fetch_task = nullptr;
uint32_t s_fetch_generation = 1;
uint32_t s_next_fetch_id = 1;

struct PendingFetch {
    bool active = false;
    uint32_t id = 0;
    uint32_t generation = 0;
    qjs_http::FetchRequest request;
    JSValue resolve = JS_UNDEFINED;
    JSValue reject = JS_UNDEFINED;
};

struct FetchWork {
    uint32_t id = 0;
    uint32_t generation = 0;
    qjs_http::FetchRequest request;
};

struct FetchCompletion {
    uint32_t id = 0;
    uint32_t generation = 0;
    int rc = 0;
    qjs_http::FetchResult result;
    std::string error;
};

PendingFetch s_pending_fetches[kMaxPendingFetches];

struct DispatchGetJob {
    const char *path = nullptr;
    qjs_http::HttpResponse response;
    esp_err_t err = ESP_FAIL;
    std::string error;
};

int op_start(void *, uint16_t port)
{
    return (int)http_server_start(port);
}

int op_stop(void *)
{
    return (int)http_server_stop();
}

int op_static(void *, const char *url_prefix, const char *virtual_root)
{
    return (int)http_server_add_static_mount(url_prefix, virtual_root);
}

int op_clear_static(void *)
{
    return (int)http_server_clear_static_mounts();
}

int op_status(void *, qjs_http::HostStatus *out)
{
    if (!out) {
        return (int)ESP_ERR_INVALID_ARG;
    }
    bool running = false;
    uint16_t port = 0;
    esp_err_t err = http_server_get_status(&running, &port);
    if (err != ESP_OK) {
        return (int)err;
    }
    out->running = running;
    out->port = port;
    return 0;
}

int op_fetch(void *, const qjs_http::FetchRequest *req, qjs_http::FetchResult *out, std::string *error)
{
    if (!req || !out) {
        if (error) *error = "invalid fetch arguments";
        return (int)ESP_ERR_INVALID_ARG;
    }
    if (req->url.rfind("http://", 0) != 0) {
        if (error) *error = "firmware fetch supports http:// only";
        return (int)ESP_ERR_NOT_SUPPORTED;
    }

    esp_http_client_config_t config = {};
    config.url = req->url.c_str();
    config.timeout_ms = (int)req->timeout_ms;
    config.disable_auto_redirect = true;
    config.buffer_size = (int)kFetchChunkBytes;
    config.buffer_size_tx = (int)kFetchChunkBytes;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        if (error) *error = "esp_http_client_init failed";
        return (int)ESP_ERR_NO_MEM;
    }

    esp_err_t err = ESP_OK;
    if (req->method == "POST") {
        err = esp_http_client_set_method(client, HTTP_METHOD_POST);
    } else {
        err = esp_http_client_set_method(client, HTTP_METHOD_GET);
    }
    if (err == ESP_OK) {
        for (const auto &h : req->headers) {
            err = esp_http_client_set_header(client, h.name.c_str(), h.value.c_str());
            if (err != ESP_OK) break;
        }
    }
    if (err != ESP_OK) {
        if (error) *error = esp_err_to_name(err);
        esp_http_client_cleanup(client);
        return (int)err;
    }

    const int write_len = (int)req->body.size();
    err = esp_http_client_open(client, write_len);
    if (err != ESP_OK) {
        if (error) *error = esp_err_to_name(err);
        esp_http_client_cleanup(client);
        return (int)err;
    }

    if (!req->body.empty()) {
        int wrote = esp_http_client_write(client, req->body.data(), write_len);
        if (wrote != write_len) {
            if (error) *error = "esp_http_client_write failed";
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return (int)ESP_FAIL;
        }
    }

    int64_t content_len = esp_http_client_fetch_headers(client);
    if (content_len < 0 && content_len != -ESP_ERR_HTTP_EAGAIN) {
        if (error) *error = "esp_http_client_fetch_headers failed";
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return (int)ESP_FAIL;
    }
    if (content_len > 0 && (size_t)content_len > req->max_response_bytes) {
        if (error) *error = "fetch response too large";
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return (int)ESP_ERR_INVALID_SIZE;
    }

    out->status = esp_http_client_get_status_code(client);
    out->status_text = "";
    out->final_url = req->url;

    char chunk[kFetchChunkBytes];
    while (true) {
        int got = esp_http_client_read(client, chunk, sizeof(chunk));
        if (got == 0) {
            break;
        }
        if (got < 0) {
            if (got == -ESP_ERR_HTTP_EAGAIN) {
                continue;
            }
            if (error) *error = "esp_http_client_read failed";
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return (int)ESP_FAIL;
        }
        if (out->body.size() + (size_t)got > req->max_response_bytes) {
            if (error) *error = "fetch response too large";
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return (int)ESP_ERR_INVALID_SIZE;
        }
        out->body.append(chunk, (size_t)got);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return 0;
}

PendingFetch *find_pending_fetch(uint32_t id, uint32_t generation)
{
    for (auto &p : s_pending_fetches) {
        if (p.active && p.id == id && p.generation == generation) return &p;
    }
    return nullptr;
}

PendingFetch *alloc_pending_fetch()
{
    for (auto &p : s_pending_fetches) {
        if (!p.active) return &p;
    }
    return nullptr;
}

void free_pending_fetch(JSContext *ctx, PendingFetch &p)
{
    if (p.active && ctx) {
        JS_FreeValue(ctx, p.resolve);
        JS_FreeValue(ctx, p.reject);
    }
    p = PendingFetch{};
}

void invalidate_pending_fetches(JSContext *ctx)
{
    s_fetch_generation++;
    if (s_fetch_generation == 0) s_fetch_generation = 1;
    for (auto &p : s_pending_fetches) {
        if (p.active) {
            std::string reject_error;
            (void)qjs_http::reject_fetch_promise(ctx, p.reject, "fetch cancelled by QuickJS reset", &reject_error);
            free_pending_fetch(ctx, p);
        }
    }
    std::string drain_error;
    (void)qjs_http::drain_pending_jobs(ctx, kMaxFetchSettleJobs, &drain_error);
}

esp_err_t settle_fetch_job(JSContext *ctx, void *user)
{
    auto *completion = static_cast<FetchCompletion *>(user);
    if (!ctx || !completion) {
        delete completion;
        return ESP_ERR_INVALID_ARG;
    }

    PendingFetch *pending = find_pending_fetch(completion->id, completion->generation);
    if (!pending) {
        delete completion;
        return ESP_OK;
    }

    std::string settle_error;
    if (completion->rc == 0) {
        (void)qjs_http::resolve_fetch_promise(ctx, pending->resolve, pending->request, completion->result, &settle_error);
    } else {
        (void)qjs_http::reject_fetch_promise(ctx,
                                             pending->reject,
                                             completion->error.empty() ? "fetch failed" : completion->error.c_str(),
                                             &settle_error);
    }
    free_pending_fetch(ctx, *pending);

    std::string drain_error;
    if (!qjs_http::drain_pending_jobs(ctx, kMaxFetchSettleJobs, &drain_error)) {
        ESP_LOGW(kTag, "fetch settle promise-drain warning: %s", drain_error.c_str());
    }
    if (!settle_error.empty()) {
        ESP_LOGW(kTag, "fetch settle warning: %s", settle_error.c_str());
    }
    delete completion;
    return ESP_OK;
}

void fetch_worker_task(void *)
{
    ESP_LOGI(kTag, "fetch worker start");
    for (;;) {
        FetchWork *work = nullptr;
        if (xQueueReceive(s_fetch_queue, &work, portMAX_DELAY) != pdTRUE || !work) continue;

        auto *completion = new (std::nothrow) FetchCompletion();
        if (!completion) {
            delete work;
            continue;
        }
        completion->id = work->id;
        completion->generation = work->generation;
        completion->rc = op_fetch(nullptr, &work->request, &completion->result, &completion->error);
        delete work;

        qjs_job_t job = {};
        job.fn = settle_fetch_job;
        job.user = completion;
        job.timeout_ms = kFetchSettleTimeoutMs;
        esp_err_t post_err = ESP_FAIL;
        for (int i = 0; i < 10; ++i) {
            post_err = qjs_service_post(s_fetch_service, &job);
            if (post_err == ESP_OK) break;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (post_err != ESP_OK) {
            ESP_LOGE(kTag, "failed to post fetch completion: %s", esp_err_to_name(post_err));
            delete completion;
        }
    }
}

esp_err_t ensure_fetch_worker(qjs_service_t *svc)
{
    if (!svc) return ESP_ERR_INVALID_ARG;
    s_fetch_service = svc;
    if (!s_fetch_queue) {
        s_fetch_queue = xQueueCreate((UBaseType_t)kMaxPendingFetches, sizeof(FetchWork *));
        if (!s_fetch_queue) return ESP_ERR_NO_MEM;
    }
    if (!s_fetch_task) {
        BaseType_t ok = xTaskCreate(fetch_worker_task, "qjs_fetch", 6144, nullptr, 6, &s_fetch_task);
        if (ok != pdPASS) return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

int op_fetch_async(void *user,
                   JSContext *ctx,
                   const qjs_http::FetchRequest *req,
                   JSValueConst resolve,
                   JSValueConst reject,
                   std::string *error)
{
    auto *svc = static_cast<qjs_service_t *>(user);
    if (!svc || !ctx || !req) {
        if (error) *error = "invalid async fetch arguments";
        return (int)ESP_ERR_INVALID_ARG;
    }
    esp_err_t worker_err = ensure_fetch_worker(svc);
    if (worker_err != ESP_OK) {
        if (error) *error = esp_err_to_name(worker_err);
        return (int)worker_err;
    }

    PendingFetch *pending = alloc_pending_fetch();
    if (!pending) {
        if (error) *error = "too many pending fetches";
        return (int)ESP_ERR_NO_MEM;
    }

    auto *work = new (std::nothrow) FetchWork();
    if (!work) {
        if (error) *error = "fetch work allocation failed";
        return (int)ESP_ERR_NO_MEM;
    }

    const uint32_t id = s_next_fetch_id++;
    if (s_next_fetch_id == 0) s_next_fetch_id = 1;
    pending->active = true;
    pending->id = id;
    pending->generation = s_fetch_generation;
    pending->request = *req;
    pending->resolve = JS_DupValue(ctx, resolve);
    pending->reject = JS_DupValue(ctx, reject);

    work->id = id;
    work->generation = s_fetch_generation;
    work->request = *req;

    if (xQueueSend(s_fetch_queue, &work, 0) != pdTRUE) {
        delete work;
        free_pending_fetch(ctx, *pending);
        if (error) *error = "fetch queue full";
        return (int)ESP_ERR_TIMEOUT;
    }
    return 0;
}

esp_err_t dispatch_get_job(JSContext *ctx, void *user)
{
    (void)ctx;
    auto *job = static_cast<DispatchGetJob *>(user);
    if (!job || !job->path) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_runtime) {
        job->err = ESP_ERR_NOT_FOUND;
        job->error = "http runtime unavailable";
        return ESP_OK;
    }
    bool ok = s_runtime->dispatch_get(job->path, &job->response, &job->error);
    job->err = ok ? ESP_OK : ESP_ERR_NOT_FOUND;
    return ESP_OK;
}

esp_err_t dynamic_get_handler(const char *path, http_dynamic_response_t *out, void *user)
{
    auto *svc = static_cast<qjs_service_t *>(user);
    if (!path || !out || !svc) {
        return ESP_ERR_INVALID_ARG;
    }

    DispatchGetJob dispatch = {};
    dispatch.path = path;
    qjs_job_t job = {};
    job.fn = dispatch_get_job;
    job.user = &dispatch;
    job.timeout_ms = 1000;
    esp_err_t err = qjs_service_run(svc, &job);
    if (err != ESP_OK) {
        return err;
    }
    if (dispatch.err != ESP_OK) {
        return dispatch.err;
    }

    out->status = dispatch.response.status;
    if (out->status < 100 || out->status > 599) {
        out->status = 500;
    }
    std::snprintf(out->content_type,
                  sizeof(out->content_type),
                  "%s",
                  dispatch.response.content_type.empty() ? "text/plain; charset=utf-8" : dispatch.response.content_type.c_str());
    out->body_len = dispatch.response.body.size();
    if (out->body_len > 0) {
        out->body = static_cast<char *>(std::malloc(out->body_len));
        if (!out->body) {
            return ESP_ERR_NO_MEM;
        }
        std::memcpy(out->body, dispatch.response.body.data(), out->body_len);
    }
    return ESP_OK;
}

qjs_http::HostOps make_firmware_ops(qjs_service_t *svc)
{
    qjs_http::HostOps ops = {};
    ops.user = svc;
    ops.start = op_start;
    ops.stop = op_stop;
    ops.add_static_mount = op_static;
    ops.clear_static_mounts = op_clear_static;
    ops.status = op_status;
    // Keep the blocking adapter available as a synchronous fallback/reference,
    // but firmware fetch() uses fetch_async so esp_http_client work runs on the
    // qjs_fetch worker instead of the QuickJS owner task.
    ops.fetch = op_fetch;
    ops.fetch_async = op_fetch_async;
    return ops;
}

esp_err_t clear_http_job(JSContext *ctx, void *user)
{
    (void)user;
    invalidate_pending_fetches(ctx);
    delete s_runtime;
    s_runtime = nullptr;
    qjs_http::set_active_runtime(nullptr);
    return ESP_OK;
}

esp_err_t install_http_job(JSContext *ctx, void *user)
{
    if (!ctx || !user) {
        return ESP_ERR_INVALID_ARG;
    }

    delete s_runtime;
    auto *svc = static_cast<qjs_service_t *>(user);
    esp_err_t worker_err = ensure_fetch_worker(svc);
    if (worker_err != ESP_OK) {
        return worker_err;
    }
    s_runtime = new (std::nothrow) qjs_http::Runtime(ctx, make_firmware_ops(svc));
    if (!s_runtime) {
        return ESP_ERR_NO_MEM;
    }
    (void)http_server_set_dynamic_get_handler(dynamic_get_handler, user);

    const int rc = s_runtime->install_global();
    if (rc != 0) {
        delete s_runtime;
        s_runtime = nullptr;
        qjs_http::set_active_runtime(nullptr);
        return ESP_FAIL;
    }
    ESP_LOGI(kTag, "installed QuickJS http namespace");
    return ESP_OK;
}
}  // namespace

esp_err_t clear_http_namespace_state(qjs_service_t *svc)
{
    if (!svc) {
        return ESP_ERR_INVALID_ARG;
    }
    qjs_job_t job = {};
    job.fn = clear_http_job;
    job.timeout_ms = kInstallTimeoutMs;
    return qjs_service_run(svc, &job);
}

esp_err_t install_http_namespace(qjs_service_t *svc)
{
    if (!svc) {
        return ESP_ERR_INVALID_ARG;
    }
    qjs_job_t job = {};
    job.fn = install_http_job;
    job.user = svc;
    job.timeout_ms = kInstallTimeoutMs;
    return qjs_service_run(svc, &job);
}
