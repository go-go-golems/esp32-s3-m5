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
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"

#include "display_app.h"
#include "httpd_assets_embed.h"
#include "httpd_ws_hub.h"
#include "storage_fatfs.h"
#include "wifi_app.h"

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
static httpd_ws_hub_t s_ws_hub = {};

esp_err_t http_server_ws_broadcast_binary(const uint8_t *data, size_t len) {
    return httpd_ws_hub_broadcast_binary(&s_ws_hub, data, len);
}

esp_err_t http_server_ws_broadcast_text(const char *text) {
    return httpd_ws_hub_broadcast_text(&s_ws_hub, text);
}

void http_server_ws_set_binary_rx_cb(http_server_ws_binary_rx_cb_t cb) {
    httpd_ws_hub_set_binary_rx_cb(&s_ws_hub, cb);
}

static esp_err_t ws_handler(httpd_req_t *req) {
    // Handshake request hits here first. Subsequent WS data frames arrive with a different method.
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "ws connected: fd=%d", httpd_req_to_sockfd(req));
    }
    return httpd_ws_hub_handle_req(&s_ws_hub, req);
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
    return httpd_assets_embed_send(req,
                                  assets_index_html_start,
                                  assets_index_html_end,
                                  "text/html; charset=utf-8",
                                  NULL,
                                  true);
}

static esp_err_t asset_app_js_get(httpd_req_t *req) {
    return httpd_assets_embed_send(req,
                                  assets_app_js_start,
                                  assets_app_js_end,
                                  "application/javascript",
                                  NULL,
                                  true);
}

static esp_err_t asset_app_css_get(httpd_req_t *req) {
    return httpd_assets_embed_send(req,
                                  assets_app_css_start,
                                  assets_app_css_end,
                                  "text/css",
                                  NULL,
                                  true);
}

static esp_err_t status_get(httpd_req_t *req) {
    char buf[256];
    char ap_ip[16] = "0.0.0.0";
    char sta_ip[16] = "0.0.0.0";

    esp_netif_t *ap = wifi_app_get_ap_netif();
    if (ap) {
        esp_netif_ip_info_t ip = {};
        if (esp_netif_get_ip_info(ap, &ip) == ESP_OK) {
            snprintf(ap_ip, sizeof(ap_ip), IPSTR, IP2STR(&ip.ip));
        }
    }

    esp_netif_t *sta = wifi_app_get_sta_netif();
    if (sta) {
        esp_netif_ip_info_t ip = {};
        if (esp_netif_get_ip_info(sta, &ip) == ESP_OK) {
            snprintf(sta_ip, sizeof(sta_ip), IPSTR, IP2STR(&ip.ip));
        }
    }

    const char *mode =
#if CONFIG_TUTORIAL_0017_WIFI_MODE_SOFTAP
        "softap";
#elif CONFIG_TUTORIAL_0017_WIFI_MODE_STA
        "sta";
#elif CONFIG_TUTORIAL_0017_WIFI_MODE_APSTA
        "apsta";
#else
        "unknown";
#endif

#if CONFIG_TUTORIAL_0017_WIFI_MODE_STA || CONFIG_TUTORIAL_0017_WIFI_MODE_APSTA
    const char *sta_ssid = CONFIG_TUTORIAL_0017_WIFI_STA_SSID;
#else
    const char *sta_ssid = "";
#endif

    const int n = snprintf(buf, sizeof(buf),
                           "{\"ok\":true,\"mode\":\"%s\",\"ap_ssid\":\"%s\",\"ap_ip\":\"%s\",\"sta_ssid\":\"%s\",\"sta_ip\":\"%s\"}",
                           mode,
                           CONFIG_TUTORIAL_0017_WIFI_SOFTAP_SSID,
                           ap_ip,
                           sta_ssid,
                           sta_ip);
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
    if (httpd_ws_hub_init(&s_ws_hub, s_server) != ESP_OK) {
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
