/*
 * Tutorial 0017: HTTP server (esp_http_server) + REST API for graphics.
 */

#include "http_server.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"

#include "display_app.h"
#include "storage_fatfs.h"

static const char *TAG = "atoms3r_web_ui_0017";

static httpd_handle_t s_server = nullptr;

// Embedded frontend assets (built from web/ and copied into main/assets/).
extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t assets_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t assets_app_js_end[] asm("_binary_app_js_end");
extern const uint8_t assets_app_css_start[] asm("_binary_app_css_start");
extern const uint8_t assets_app_css_end[] asm("_binary_app_css_end");

#if CONFIG_HTTPD_WS_SUPPORT
static SemaphoreHandle_t s_ws_mu = nullptr;
static int s_ws_clients[8] = {};
static size_t s_ws_clients_n = 0;
static http_server_ws_binary_rx_cb_t s_ws_binary_rx_cb = nullptr;

static void ws_client_add(int fd) {
    if (!s_ws_mu) return;
    xSemaphoreTake(s_ws_mu, portMAX_DELAY);
    for (size_t i = 0; i < s_ws_clients_n; i++) {
        if (s_ws_clients[i] == fd) {
            xSemaphoreGive(s_ws_mu);
            return;
        }
    }
    if (s_ws_clients_n < (sizeof(s_ws_clients) / sizeof(s_ws_clients[0]))) {
        s_ws_clients[s_ws_clients_n++] = fd;
    }
    xSemaphoreGive(s_ws_mu);
}

static void ws_client_remove(int fd) {
    if (!s_ws_mu) return;
    xSemaphoreTake(s_ws_mu, portMAX_DELAY);
    for (size_t i = 0; i < s_ws_clients_n; i++) {
        if (s_ws_clients[i] == fd) {
            s_ws_clients[i] = s_ws_clients[s_ws_clients_n - 1];
            s_ws_clients_n--;
            break;
        }
    }
    xSemaphoreGive(s_ws_mu);
}

static size_t ws_clients_snapshot(int *out, size_t max_out) {
    if (!out || max_out == 0 || !s_ws_mu) return 0;
    xSemaphoreTake(s_ws_mu, portMAX_DELAY);
    const size_t n = (s_ws_clients_n < max_out) ? s_ws_clients_n : max_out;
    for (size_t i = 0; i < n; i++) {
        out[i] = s_ws_clients[i];
    }
    xSemaphoreGive(s_ws_mu);
    return n;
}

static void ws_send_free_cb(esp_err_t err, int fd, void *arg) {
    (void)err;
    (void)fd;
    free(arg);
}

esp_err_t http_server_ws_broadcast_binary(const uint8_t *data, size_t len) {
    if (!s_server) return ESP_ERR_INVALID_STATE;
    if (!data || len == 0) return ESP_OK;

    int fds[8];
    const size_t n = ws_clients_snapshot(fds, sizeof(fds) / sizeof(fds[0]));
    for (size_t i = 0; i < n; i++) {
        const int fd = fds[i];
        if (httpd_ws_get_fd_info(s_server, fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
            continue;
        }

        uint8_t *copy = (uint8_t *)malloc(len);
        if (!copy) {
            return ESP_ERR_NO_MEM;
        }
        memcpy(copy, data, len);

        httpd_ws_frame_t frame = {};
        frame.type = HTTPD_WS_TYPE_BINARY;
        frame.payload = copy;
        frame.len = len;

        (void)httpd_ws_send_data_async(s_server, fd, &frame, ws_send_free_cb, copy);
    }
    return ESP_OK;
}

esp_err_t http_server_ws_broadcast_text(const char *text) {
    if (!s_server) return ESP_ERR_INVALID_STATE;
    if (!text) return ESP_OK;
    const size_t len = strlen(text);
    if (len == 0) return ESP_OK;

    int fds[8];
    const size_t n = ws_clients_snapshot(fds, sizeof(fds) / sizeof(fds[0]));
    for (size_t i = 0; i < n; i++) {
        const int fd = fds[i];
        if (httpd_ws_get_fd_info(s_server, fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
            continue;
        }

        char *copy = (char *)malloc(len);
        if (!copy) {
            return ESP_ERR_NO_MEM;
        }
        memcpy(copy, text, len);

        httpd_ws_frame_t frame = {};
        frame.type = HTTPD_WS_TYPE_TEXT;
        frame.payload = (uint8_t *)copy;
        frame.len = len;

        (void)httpd_ws_send_data_async(s_server, fd, &frame, ws_send_free_cb, copy);
    }
    return ESP_OK;
}

void http_server_ws_set_binary_rx_cb(http_server_ws_binary_rx_cb_t cb) {
    s_ws_binary_rx_cb = cb;
}

static esp_err_t ws_handler(httpd_req_t *req) {
    const int fd = httpd_req_to_sockfd(req);
    ws_client_add(fd);

    // Handshake request hits here first. Subsequent WS data frames arrive with a different method.
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "ws connected: fd=%d", fd);
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {};
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
    if (err != ESP_OK) {
        return err;
    }
    if (frame.len > 1024) {
        ESP_LOGW(TAG, "ws frame too large: fd=%d len=%u", fd, (unsigned)frame.len);
        return ESP_FAIL; // closes socket
    }

    uint8_t buf[1024];
    frame.payload = buf;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) {
        return err;
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "ws closed: fd=%d", fd);
        ws_client_remove(fd);
        return ESP_OK;
    }

    if (frame.type == HTTPD_WS_TYPE_BINARY) {
        if (s_ws_binary_rx_cb) {
            (void)s_ws_binary_rx_cb(frame.payload, frame.len);
        }
    }
    return ESP_OK;
}
#else
esp_err_t http_server_ws_broadcast_binary(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t http_server_ws_broadcast_text(const char *text) {
    (void)text;
    return ESP_ERR_NOT_SUPPORTED;
}
void http_server_ws_set_binary_rx_cb(http_server_ws_binary_rx_cb_t cb) {
    (void)cb;
}
#endif

static const char *graphics_prefix(void) {
    return "/api/graphics/";
}

static esp_err_t send_json(httpd_req_t *req, const char *json) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t root_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    return httpd_resp_send(req, (const char *)assets_index_html_start, len);
}

static esp_err_t asset_app_js_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    const size_t len = (size_t)(assets_app_js_end - assets_app_js_start);
    return httpd_resp_send(req, (const char *)assets_app_js_start, len);
}

static esp_err_t asset_app_css_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    const size_t len = (size_t)(assets_app_css_end - assets_app_css_start);
    return httpd_resp_send(req, (const char *)assets_app_css_start, len);
}

static esp_err_t status_get(httpd_req_t *req) {
    // Keep this extremely small and dependency-free.
    char buf[256];
    const int n = snprintf(buf, sizeof(buf),
                           "{\"ok\":true,\"ssid\":\"%s\",\"ap_ip\":\"%s\"}",
                           CONFIG_TUTORIAL_0017_WIFI_SOFTAP_SSID,
                           "192.168.4.1");
    if (n <= 0 || n >= (int)sizeof(buf)) {
        return send_json(req, "{\"ok\":false}");
    }
    return send_json(req, buf);
}

static esp_err_t graphics_list_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send_chunk(req, "[", HTTPD_RESP_USE_STRLEN);

    const char *dir_path = CONFIG_TUTORIAL_0017_STORAGE_GRAPHICS_DIR;
    DIR *dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGW(TAG, "opendir(%s) failed: errno=%d", dir_path, errno);
        httpd_resp_send_chunk(req, "]", HTTPD_RESP_USE_STRLEN);
        return httpd_resp_send_chunk(req, nullptr, 0);
    }

    bool first = true;
    while (true) {
        struct dirent *ent = readdir(dir);
        if (!ent) break;
        if (ent->d_name[0] == '.') continue;

        char entry[192];
        const int wrote = snprintf(entry, sizeof(entry),
                                   "%s{\"name\":\"%s\"}",
                                   first ? "" : ",",
                                   ent->d_name);
        if (wrote > 0 && wrote < (int)sizeof(entry)) {
            httpd_resp_send_chunk(req, entry, HTTPD_RESP_USE_STRLEN);
            first = false;
        }
    }

    closedir(dir);
    httpd_resp_send_chunk(req, "]", HTTPD_RESP_USE_STRLEN);
    return httpd_resp_send_chunk(req, nullptr, 0);
}

static const char *uri_basename(const char *uri, const char *prefix) {
    if (!uri || !prefix) return nullptr;
    const size_t pfx_len = strlen(prefix);
    if (strncmp(uri, prefix, pfx_len) != 0) return nullptr;
    const char *name = uri + pfx_len;
    if (!name[0]) return nullptr;
    return name;
}

static esp_err_t graphics_put(httpd_req_t *req) {
    const char *name = uri_basename(req->uri, graphics_prefix());
    if (!name || !storage_fatfs_is_valid_filename(name, 64)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid filename");
        return ESP_OK;
    }

    if (req->content_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
        return ESP_OK;
    }
    if ((size_t)req->content_len > (size_t)CONFIG_TUTORIAL_0017_MAX_UPLOAD_BYTES) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "too large");
        return ESP_OK;
    }

    char path[256];
    const int wrote = snprintf(path, sizeof(path), "%s/%s",
                               CONFIG_TUTORIAL_0017_STORAGE_GRAPHICS_DIR,
                               name);
    if (wrote <= 0 || wrote >= (int)sizeof(path)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "path too long");
        return ESP_OK;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "fopen(%s) failed: errno=%d", path, errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "open failed");
        return ESP_OK;
    }

    char buf[1024];
    int remaining = req->content_len;
    while (remaining > 0) {
        const int to_read = (remaining > (int)sizeof(buf)) ? (int)sizeof(buf) : remaining;
        const int n = httpd_req_recv(req, buf, to_read);
        if (n <= 0) {
            fclose(f);
            unlink(path);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
            return ESP_OK;
        }
        const size_t nw = fwrite(buf, 1, (size_t)n, f);
        if (nw != (size_t)n) {
            fclose(f);
            unlink(path);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "write failed");
            return ESP_OK;
        }
        remaining -= n;
    }
    fclose(f);

    // Default display policy (MVP): newest upload auto-displays.
    (void)display_app_png_from_file(path);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

static esp_err_t graphics_get(httpd_req_t *req) {
    const char *name = uri_basename(req->uri, graphics_prefix());
    if (!name || !storage_fatfs_is_valid_filename(name, 64)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid filename");
        return ESP_OK;
    }

    char path[256];
    const int wrote = snprintf(path, sizeof(path), "%s/%s",
                               CONFIG_TUTORIAL_0017_STORAGE_GRAPHICS_DIR,
                               name);
    if (wrote <= 0 || wrote >= (int)sizeof(path)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "path too long");
        return ESP_OK;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }

    // Best-effort content-type.
    const char *ext = strrchr(name, '.');
    if (ext && (strcasecmp(ext, ".png") == 0)) {
        httpd_resp_set_type(req, "image/png");
    } else {
        httpd_resp_set_type(req, "application/octet-stream");
    }

    char buf[1024];
    while (true) {
        const size_t nr = fread(buf, 1, sizeof(buf), f);
        if (nr > 0) {
            if (httpd_resp_send_chunk(req, buf, nr) != ESP_OK) {
                fclose(f);
                return ESP_FAIL;
            }
        }
        if (nr < sizeof(buf)) {
            break;
        }
    }
    fclose(f);
    return httpd_resp_send_chunk(req, nullptr, 0);
}

static esp_err_t graphics_delete(httpd_req_t *req) {
    const char *name = uri_basename(req->uri, graphics_prefix());
    if (!name || !storage_fatfs_is_valid_filename(name, 64)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid filename");
        return ESP_OK;
    }

    char path[256];
    const int wrote = snprintf(path, sizeof(path), "%s/%s",
                               CONFIG_TUTORIAL_0017_STORAGE_GRAPHICS_DIR,
                               name);
    if (wrote <= 0 || wrote >= (int)sizeof(path)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "path too long");
        return ESP_OK;
    }

    if (unlink(path) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

esp_err_t http_server_start(void) {
    if (s_server) {
        return ESP_OK;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = nullptr;
        return err;
    }

#if CONFIG_HTTPD_WS_SUPPORT
    s_ws_mu = xSemaphoreCreateMutex();
    if (!s_ws_mu) {
        return ESP_ERR_NO_MEM;
    }
#endif

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &root);

    httpd_uri_t app_js = {
        .uri = "/assets/app.js",
        .method = HTTP_GET,
        .handler = asset_app_js_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &app_js);

    httpd_uri_t app_css = {
        .uri = "/assets/app.css",
        .method = HTTP_GET,
        .handler = asset_app_css_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &app_css);

    httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &status);

    httpd_uri_t list = {
        .uri = "/api/graphics",
        .method = HTTP_GET,
        .handler = graphics_list_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &list);

    httpd_uri_t put = {
        .uri = "/api/graphics/*",
        .method = HTTP_PUT,
        .handler = graphics_put,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &put);

    httpd_uri_t get = {
        .uri = "/api/graphics/*",
        .method = HTTP_GET,
        .handler = graphics_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &get);

    httpd_uri_t del = {
        .uri = "/api/graphics/*",
        .method = HTTP_DELETE,
        .handler = graphics_delete,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &del);

#if CONFIG_HTTPD_WS_SUPPORT
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = nullptr,
        .is_websocket = true,
        .handle_ws_control_frames = true,
        .supported_subprotocol = nullptr,
    };
    httpd_register_uri_handler(s_server, &ws);
#endif

    return ESP_OK;
}


