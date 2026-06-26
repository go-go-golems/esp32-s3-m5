// http_namespace.cpp — ESP-IDF wrapper for the shared QuickJS HTTP/fetch core.
#include "http_namespace.h"

#include <new>

#include "esp_err.h"
#include "esp_log.h"
#include "http_namespace_core.h"
#include "http_server.h"

namespace {
constexpr const char *kTag = "0103_http_ns";
constexpr uint32_t kInstallTimeoutMs = 1000;

qjs_http::Runtime *s_runtime = nullptr;

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

qjs_http::HostOps make_firmware_ops()
{
    qjs_http::HostOps ops = {};
    ops.start = op_start;
    ops.stop = op_stop;
    ops.add_static_mount = op_static;
    ops.clear_static_mounts = op_clear_static;
    ops.status = op_status;
    // Firmware fetch is deliberately left unimplemented in this phase. The shared
    // core exposes fetch(), but calls reject/throw until a bounded ESP-IDF HTTP
    // client or worker-backed adapter is wired in a later phase.
    ops.fetch = nullptr;
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
    (void)user;
    if (!ctx) {
        return ESP_ERR_INVALID_ARG;
    }

    delete s_runtime;
    s_runtime = new (std::nothrow) qjs_http::Runtime(ctx, make_firmware_ops());
    if (!s_runtime) {
        return ESP_ERR_NO_MEM;
    }
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
    job.timeout_ms = kInstallTimeoutMs;
    return qjs_service_run(svc, &job);
}
