// http_namespace.cpp — ESP-IDF wrapper for the shared QuickJS HTTP/fetch core.
#include "http_namespace.h"

#include <cstdlib>
#include <cstring>
#include <new>

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "http_namespace_core.h"
#include "http_server.h"

namespace {
constexpr const char *kTag = "0103_http_ns";
constexpr uint32_t kInstallTimeoutMs = 1000;
constexpr size_t kFetchChunkBytes = 512;

qjs_http::Runtime *s_runtime = nullptr;

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

qjs_http::HostOps make_firmware_ops()
{
    qjs_http::HostOps ops = {};
    ops.start = op_start;
    ops.stop = op_stop;
    ops.add_static_mount = op_static;
    ops.clear_static_mounts = op_clear_static;
    ops.status = op_status;
    // First firmware fetch milestone: bounded blocking HTTP client on the owner
    // task. This keeps the JS API shape shared with the host; a later phase can
    // move the adapter to a worker-backed Promise settlement path if needed.
    ops.fetch = op_fetch;
    return ops;
}

esp_err_t clear_http_job(JSContext *ctx, void *user)
{
    (void)ctx;
    (void)user;
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
    s_runtime = new (std::nothrow) qjs_http::Runtime(ctx, make_firmware_ops());
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
