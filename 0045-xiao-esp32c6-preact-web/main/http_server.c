/*
 * MO-033 / 0044: Minimal HTTP server (esp_http_server) serving embedded assets.
 *
 * Static asset embedding pattern is based on 0017-atoms3r-web-ui (hardcoded routes
 * and EMBED_TXTFILES).
 */

#include "http_server.h"

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_chip_info.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "wifi_mgr.h"

static const char *TAG = "mo033_http";

static httpd_handle_t s_server = NULL;

// Embedded frontend assets (built from web/ into main/assets/).
extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t assets_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t assets_app_js_end[] asm("_binary_app_js_end");
extern const uint8_t assets_app_css_start[] asm("_binary_app_css_start");
extern const uint8_t assets_app_css_end[] asm("_binary_app_css_end");

static void maybe_set_no_store(httpd_req_t *req)
{
#if CONFIG_MO033_HTTP_CACHE_NO_STORE
    (void)httpd_resp_set_hdr(req, "Cache-Control", "no-store");
#else
    (void)req;
#endif
}

static esp_err_t root_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    return httpd_resp_send(req, (const char *)assets_index_html_start, len);
}

static esp_err_t asset_app_js_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "application/javascript");
    const size_t len = (size_t)(assets_app_js_end - assets_app_js_start);
    return httpd_resp_send(req, (const char *)assets_app_js_start, len);
}

static esp_err_t asset_app_css_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "text/css");
    const size_t len = (size_t)(assets_app_css_end - assets_app_css_start);
    return httpd_resp_send(req, (const char *)assets_app_css_start, len);
}

static esp_err_t status_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    maybe_set_no_store(req);

    wifi_mgr_status_t st = {};
    (void)wifi_mgr_get_status(&st);

    int rssi = 0;
    wifi_ap_record_t ap = {};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        rssi = ap.rssi;
    }

    const uint32_t free_heap = esp_get_free_heap_size();
    esp_chip_info_t chip = {};
    esp_chip_info(&chip);

    char ip_str[32] = "-";
    if (st.ip4) {
        snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u",
                 (unsigned)((st.ip4 >> 24) & 0xff),
                 (unsigned)((st.ip4 >> 16) & 0xff),
                 (unsigned)((st.ip4 >> 8) & 0xff),
                 (unsigned)((st.ip4 >> 0) & 0xff));
    }

    char body[256];
    const int n = snprintf(body,
                           sizeof(body),
                           "{\"ok\":true,\"chip\":\"esp32c6\",\"cores\":%d,\"ip\":\"%s\",\"rssi\":%d,\"free_heap\":%u}",
                           chip.cores,
                           ip_str,
                           rssi,
                           (unsigned)free_heap);
    if (n <= 0) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "encode failed");
    }
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

esp_err_t http_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &root);

    httpd_uri_t app_js = {
        .uri = "/assets/app.js",
        .method = HTTP_GET,
        .handler = asset_app_js_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &app_js);

    httpd_uri_t app_css = {
        .uri = "/assets/app.css",
        .method = HTTP_GET,
        .handler = asset_app_css_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &app_css);

    httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &status);

    return ESP_OK;
}

