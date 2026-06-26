// http_server.cpp — host-owned HTTP service for 0103 AtomS3R M12.
#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "storage_namespace.h"

namespace {
constexpr const char *kTag = "0103_http";
constexpr size_t kMaxStaticMounts = 4;
constexpr size_t kMaxUrlPrefixBytes = 31;
constexpr size_t kMaxVirtualRootBytes = 127;
constexpr size_t kMaxVirtualPathBytes = 160;
constexpr size_t kMaxStaticFileBytes = 128 * 1024;

struct StaticMount {
    bool used;
    char url_prefix[kMaxUrlPrefixBytes + 1];
    char virtual_root[kMaxVirtualRootBytes + 1];
};

struct SendCtx {
    httpd_req_t *req;
    bool sent;
};

httpd_handle_t s_server = nullptr;
uint16_t s_port = 0;
StaticMount s_static_mounts[kMaxStaticMounts] = {};
http_dynamic_get_handler_t s_dynamic_get_handler = nullptr;
void *s_dynamic_get_user = nullptr;
SemaphoreHandle_t s_lock = nullptr;
StaticSemaphore_t s_lock_storage = {};

SemaphoreHandle_t http_lock()
{
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutexStatic(&s_lock_storage);
    }
    return s_lock;
}

void lock_http()
{
    SemaphoreHandle_t lock = http_lock();
    if (lock) {
        xSemaphoreTake(lock, portMAX_DELAY);
    }
}

void unlock_http()
{
    if (s_lock) {
        xSemaphoreGive(s_lock);
    }
}

bool has_bad_path_chars(const char *s)
{
    return !s || !*s || strstr(s, "//") || strstr(s, "/../") || strstr(s, "/./") ||
           strchr(s, '\\') || strchr(s, ':') || strchr(s, '?') || strchr(s, '#');
}

esp_err_t copy_normalized_path(const char *in, char *out, size_t out_len)
{
    if (!in || !out || out_len == 0 || in[0] != '/' || has_bad_path_chars(in)) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t in_len = strlen(in);
    if (in_len == 0 || in_len >= out_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    if ((in_len >= 2 && strcmp(in + in_len - 2, "/.") == 0) ||
        (in_len >= 3 && strcmp(in + in_len - 3, "/..") == 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(out, in, in_len + 1);
    size_t out_len_actual = in_len;
    while (out_len_actual > 1 && out[out_len_actual - 1] == '/') {
        out[--out_len_actual] = '\0';
    }
    return ESP_OK;
}

bool prefix_matches(const char *path, size_t path_len, const char *prefix)
{
    const size_t prefix_len = strlen(prefix);
    if (path_len < prefix_len || strncmp(path, prefix, prefix_len) != 0) {
        return false;
    }
    return path_len == prefix_len || path[prefix_len] == '/';
}

esp_err_t uri_path_only(const char *uri, char *out, size_t out_len)
{
    if (!uri || !out || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t path_len = strcspn(uri, "?#");
    if (path_len == 0 || path_len >= out_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(out, uri, path_len);
    out[path_len] = '\0';
    return ESP_OK;
}

esp_err_t uri_to_virtual_path(const char *uri, char *out, size_t out_len)
{
    if (!uri || !out || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t path_len = strcspn(uri, "?#");
    if (path_len == 0 || path_len >= kMaxVirtualPathBytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    lock_http();
    for (const StaticMount &mount : s_static_mounts) {
        if (!mount.used || !prefix_matches(uri, path_len, mount.url_prefix)) {
            continue;
        }
        const size_t prefix_len = strlen(mount.url_prefix);
        const size_t suffix_len = path_len - prefix_len;
        char suffix_buf[kMaxVirtualPathBytes] = {};
        const char *suffix = "/index.html";
        if (suffix_len > 1 || (suffix_len == 1 && uri[prefix_len] != '/')) {
            if (suffix_len >= sizeof(suffix_buf)) {
                unlock_http();
                return ESP_ERR_INVALID_SIZE;
            }
            memcpy(suffix_buf, uri + prefix_len, suffix_len);
            suffix_buf[suffix_len] = '\0';
            suffix = suffix_buf;
        }
        const int n = snprintf(out, out_len, "%s%s", mount.virtual_root, suffix);
        unlock_http();
        if (n < 0 || (size_t)n >= out_len) {
            return ESP_ERR_INVALID_SIZE;
        }
        return storage_namespace_validate_virtual_path(out);
    }
    unlock_http();
    return ESP_ERR_NOT_FOUND;
}

const char *mime_for_path(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
    if (strcmp(dot, ".css") == 0) return "text/css; charset=utf-8";
    if (strcmp(dot, ".json") == 0) return "application/json; charset=utf-8";
    if (strcmp(dot, ".txt") == 0) return "text/plain; charset=utf-8";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

esp_err_t send_chunk_writer(const void *data, size_t len, void *user)
{
    SendCtx *ctx = static_cast<SendCtx *>(user);
    if (!ctx || !ctx->req) {
        return ESP_ERR_INVALID_ARG;
    }
    ctx->sent = ctx->sent || len > 0;
    return httpd_resp_send_chunk(ctx->req, static_cast<const char *>(data), len);
}

const char *status_line_for(int status)
{
    switch (status) {
        case 200: return "200 OK";
        case 201: return "201 Created";
        case 202: return "202 Accepted";
        case 204: return "204 No Content";
        case 400: return "400 Bad Request";
        case 404: return "404 Not Found";
        case 405: return "405 Method Not Allowed";
        case 413: return "413 Content Too Large";
        case 500: return "500 Internal Server Error";
        default: return (status >= 200 && status <= 299) ? "200 OK" : "500 Internal Server Error";
    }
}

bool try_dynamic_get(httpd_req_t *req)
{
    http_dynamic_get_handler_t handler = nullptr;
    void *user = nullptr;
    lock_http();
    handler = s_dynamic_get_handler;
    user = s_dynamic_get_user;
    unlock_http();
    if (!handler) {
        return false;
    }

    char path[kMaxVirtualPathBytes] = {};
    esp_err_t err = uri_path_only(req->uri, path, sizeof(path));
    if (err != ESP_OK) {
        return false;
    }

    http_dynamic_response_t response = {};
    err = handler(path, &response, user);
    if (err == ESP_ERR_NOT_FOUND) {
        http_dynamic_response_free(&response);
        return false;
    }
    if (err != ESP_OK) {
        http_dynamic_response_free(&response);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
        return true;
    }

    httpd_resp_set_status(req, status_line_for(response.status));
    httpd_resp_set_type(req, response.content_type[0] ? response.content_type : "text/plain; charset=utf-8");
    err = httpd_resp_send(req,
                          response.body ? response.body : "",
                          response.body ? response.body_len : 0);
    ESP_LOGI(kTag, "dynamic %s status=%d bytes=%u", path, response.status, (unsigned)response.body_len);
    http_dynamic_response_free(&response);
    return true;
}

void send_error_for_storage(httpd_req_t *req, esp_err_t err)
{
    switch (err) {
        case ESP_ERR_NOT_FOUND:
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
            break;
        case ESP_ERR_INVALID_ARG:
        case ESP_ERR_NOT_ALLOWED:
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
            break;
        case ESP_ERR_INVALID_STATE:
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "storage not mounted");
            break;
        case ESP_ERR_INVALID_SIZE:
            httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "file too large");
            break;
        default:
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
            break;
    }
}

esp_err_t healthz_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, "ok\n");
}

esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_sendstr(req,
                              "<!doctype html><html><head><title>AtomS3R QuickJS</title></head>"
                              "<body><h1>AtomS3R QuickJS</h1><p>Try <a href=\"/healthz\">/healthz</a>. "
                              "Static assets can be mounted with <code>http static /static /data/www</code>.</p></body></html>");
}

esp_err_t static_handler(httpd_req_t *req)
{
    if (try_dynamic_get(req)) {
        return ESP_OK;
    }

    char virtual_path[kMaxVirtualPathBytes] = {};
    esp_err_t err = uri_to_virtual_path(req->uri, virtual_path, sizeof(virtual_path));
    if (err != ESP_OK) {
        send_error_for_storage(req, err);
        return ESP_OK;
    }

    httpd_resp_set_type(req, mime_for_path(virtual_path));
    SendCtx send_ctx = {.req = req, .sent = false};
    size_t sent_bytes = 0;
    err = storage_namespace_stream_file(virtual_path, kMaxStaticFileBytes, send_chunk_writer, &send_ctx, &sent_bytes);
    if (err != ESP_OK) {
        if (!send_ctx.sent) {
            send_error_for_storage(req, err);
            return ESP_OK;
        }
        return err;
    }
    ESP_LOGI(kTag, "served %s bytes=%u", virtual_path, (unsigned)sent_bytes);
    return httpd_resp_send_chunk(req, nullptr, 0);
}

esp_err_t register_routes(httpd_handle_t server)
{
    httpd_uri_t healthz = {};
    healthz.uri = "/healthz";
    healthz.method = HTTP_GET;
    healthz.handler = healthz_handler;
    esp_err_t err = httpd_register_uri_handler(server, &healthz);
    if (err != ESP_OK) return err;

    httpd_uri_t root = {};
    root.uri = "/";
    root.method = HTTP_GET;
    root.handler = root_handler;
    err = httpd_register_uri_handler(server, &root);
    if (err != ESP_OK) return err;

    httpd_uri_t statics = {};
    statics.uri = "/*";
    statics.method = HTTP_GET;
    statics.handler = static_handler;
    return httpd_register_uri_handler(server, &statics);
}

void print_static_mounts_locked()
{
    bool any = false;
    for (const StaticMount &mount : s_static_mounts) {
        if (!mount.used) continue;
        printf("static %s -> %s\n", mount.url_prefix, mount.virtual_root);
        any = true;
    }
    if (!any) {
        printf("static mounts: none\n");
    }
}

void print_usage()
{
    printf("usage:\n");
    printf("  http status\n");
    printf("  http start [port]\n");
    printf("  http stop\n");
    printf("  http static\n");
    printf("  http static <url-prefix> <storage-virtual-root>\n");
    printf("  http static clear\n");
}

bool parse_port(const char *s, uint16_t *out)
{
    if (!out) return false;
    if (!s || !*s) {
        *out = 80;
        return true;
    }
    char *end = nullptr;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0' || v <= 0 || v >= 65535) {
        return false;
    }
    *out = (uint16_t)v;
    return true;
}

int cmd_http(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }
    if (strcmp(argv[1], "status") == 0) {
        lock_http();
        printf("running=%d port=%u max_static_file=%u\n",
               s_server != nullptr,
               (unsigned)s_port,
               (unsigned)kMaxStaticFileBytes);
        print_static_mounts_locked();
        unlock_http();
        return 0;
    }
    if (strcmp(argv[1], "start") == 0) {
        uint16_t port = 80;
        if (argc >= 3 && !parse_port(argv[2], &port)) {
            printf("http start: invalid port: %s\n", argv[2]);
            return 1;
        }
        esp_err_t err = http_server_start(port);
        printf("http start: %s port=%u\n", esp_err_to_name(err), (unsigned)port);
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "stop") == 0) {
        esp_err_t err = http_server_stop();
        printf("http stop: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "static") == 0) {
        if (argc == 2) {
            lock_http();
            print_static_mounts_locked();
            unlock_http();
            return 0;
        }
        if (argc == 3 && strcmp(argv[2], "clear") == 0) {
            esp_err_t err = http_server_clear_static_mounts();
            printf("http static clear: %s\n", esp_err_to_name(err));
            return err == ESP_OK ? 0 : 1;
        }
        if (argc == 4) {
            esp_err_t err = http_server_add_static_mount(argv[2], argv[3]);
            printf("http static: %s %s -> %s\n", esp_err_to_name(err), argv[2], argv[3]);
            return err == ESP_OK ? 0 : 1;
        }
        print_usage();
        return 1;
    }
    print_usage();
    return 1;
}
}  // namespace

esp_err_t http_server_start(uint16_t port)
{
    if (port == 0) {
        port = 80;
    }
    if (port >= 65535) {
        return ESP_ERR_INVALID_ARG;
    }

    lock_http();
    if (s_server) {
        const bool same_port = s_port == port;
        unlock_http();
        return same_port ? ESP_OK : ESP_ERR_INVALID_STATE;
    }
    unlock_http();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.ctrl_port = (uint16_t)(port + 1);
    config.max_uri_handlers = 12;
    config.stack_size = 4096;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = nullptr;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        return err;
    }

    err = register_routes(server);
    if (err != ESP_OK) {
        httpd_stop(server);
        return err;
    }

    lock_http();
    s_server = server;
    s_port = port;
    unlock_http();
    ESP_LOGI(kTag, "HTTP server started on port %u", (unsigned)s_port);
    return ESP_OK;
}

esp_err_t http_server_stop(void)
{
    lock_http();
    httpd_handle_t server = s_server;
    s_server = nullptr;
    s_port = 0;
    unlock_http();

    if (!server) {
        return ESP_OK;
    }
    return httpd_stop(server);
}

esp_err_t http_server_get_status(bool *running, uint16_t *port)
{
    lock_http();
    if (running) {
        *running = s_server != nullptr;
    }
    if (port) {
        *port = s_port;
    }
    unlock_http();
    return ESP_OK;
}

esp_err_t http_server_add_static_mount(const char *url_prefix, const char *virtual_root)
{
    char normalized_prefix[kMaxUrlPrefixBytes + 1] = {};
    char normalized_root[kMaxVirtualRootBytes + 1] = {};
    esp_err_t err = copy_normalized_path(url_prefix, normalized_prefix, sizeof(normalized_prefix));
    if (err != ESP_OK) return err;
    err = copy_normalized_path(virtual_root, normalized_root, sizeof(normalized_root));
    if (err != ESP_OK) return err;
    err = storage_namespace_validate_virtual_path(normalized_root);
    if (err != ESP_OK) return err;

    lock_http();
    StaticMount *slot = nullptr;
    for (StaticMount &mount : s_static_mounts) {
        if (mount.used && strcmp(mount.url_prefix, normalized_prefix) == 0) {
            slot = &mount;
            break;
        }
        if (!mount.used && !slot) {
            slot = &mount;
        }
    }
    if (!slot) {
        unlock_http();
        return ESP_ERR_NO_MEM;
    }
    slot->used = true;
    strlcpy(slot->url_prefix, normalized_prefix, sizeof(slot->url_prefix));
    strlcpy(slot->virtual_root, normalized_root, sizeof(slot->virtual_root));
    unlock_http();
    ESP_LOGI(kTag, "static mount %s -> %s", normalized_prefix, normalized_root);
    return ESP_OK;
}

esp_err_t http_server_clear_static_mounts(void)
{
    lock_http();
    memset(s_static_mounts, 0, sizeof(s_static_mounts));
    unlock_http();
    return ESP_OK;
}

esp_err_t http_server_set_dynamic_get_handler(http_dynamic_get_handler_t handler, void *user)
{
    lock_http();
    s_dynamic_get_handler = handler;
    s_dynamic_get_user = user;
    unlock_http();
    return ESP_OK;
}

void http_dynamic_response_free(http_dynamic_response_t *response)
{
    if (!response) {
        return;
    }
    free(response->body);
    response->body = nullptr;
    response->body_len = 0;
    response->status = 0;
    response->content_type[0] = '\0';
}

void register_http_commands(void)
{
    esp_console_cmd_t cmd = {};
    cmd.command = "http";
    cmd.help = "HTTP server: http status|start [port]|stop|static";
    cmd.func = &cmd_http;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(kTag, "registered HTTP console commands");
}
