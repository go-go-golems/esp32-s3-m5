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

#include "cJSON.h"

#include "led_task.h"
#include "wifi_mgr.h"

static const char *TAG = "mo034_http";

static httpd_handle_t s_server = NULL;

// Embedded frontend assets (built from web/ into main/assets/).
extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t assets_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t assets_app_js_end[] asm("_binary_app_js_end");
extern const uint8_t assets_app_js_map_gz_start[] asm("_binary_app_js_map_gz_start");
extern const uint8_t assets_app_js_map_gz_end[] asm("_binary_app_js_map_gz_end");
extern const uint8_t assets_app_css_start[] asm("_binary_app_css_start");
extern const uint8_t assets_app_css_end[] asm("_binary_app_css_end");

static size_t embedded_txt_len(const uint8_t *start, const uint8_t *end)
{
    size_t len = (size_t)(end - start);
    if (len > 0 && start[len - 1] == 0) len--;
    return len;
}

static size_t embedded_bin_len(const uint8_t *start, const uint8_t *end)
{
    return (size_t)(end - start);
}

static void maybe_set_no_store(httpd_req_t *req)
{
#if CONFIG_MO033_HTTP_CACHE_NO_STORE
    (void)httpd_resp_set_hdr(req, "Cache-Control", "no-store");
#else
    (void)req;
#endif
}

static void register_or_log(const httpd_uri_t *uri)
{
    if (!s_server || !uri) return;
    const esp_err_t err = httpd_register_uri_handler(s_server, uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register uri failed: %s %s: %s",
                 (uri->method == HTTP_GET)   ? "GET"
                 : (uri->method == HTTP_POST) ? "POST"
                                              : "?",
                 uri->uri ? uri->uri : "(null)",
                 esp_err_to_name(err));
    }
}

static esp_err_t root_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const size_t len = embedded_txt_len(assets_index_html_start, assets_index_html_end);
    return httpd_resp_send(req, (const char *)assets_index_html_start, len);
}

static esp_err_t asset_app_js_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "application/javascript; charset=utf-8");
    const size_t len = embedded_txt_len(assets_app_js_start, assets_app_js_end);
    return httpd_resp_send(req, (const char *)assets_app_js_start, len);
}

static esp_err_t asset_app_css_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "text/css; charset=utf-8");
    const size_t len = embedded_txt_len(assets_app_css_start, assets_app_css_end);
    return httpd_resp_send(req, (const char *)assets_app_css_start, len);
}

static esp_err_t asset_app_js_map_get(httpd_req_t *req)
{
    maybe_set_no_store(req);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    (void)httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    const size_t len = embedded_bin_len(assets_app_js_map_gz_start, assets_app_js_map_gz_end);
    return httpd_resp_send(req, (const char *)assets_app_js_map_gz_start, len);
}

static esp_err_t status_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
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

static const char *pattern_type_to_str(led_pattern_type_t t)
{
    switch (t) {
    case LED_PATTERN_OFF:
        return "off";
    case LED_PATTERN_RAINBOW:
        return "rainbow";
    case LED_PATTERN_CHASE:
        return "chase";
    case LED_PATTERN_BREATHING:
        return "breathing";
    case LED_PATTERN_SPARKLE:
        return "sparkle";
    default:
        return "unknown";
    }
}

static bool json_read_body(httpd_req_t *req, char *buf, size_t buf_len, size_t *out_len)
{
    if (!req || !buf || buf_len == 0) return false;
    if (req->content_len <= 0 || (size_t)req->content_len >= buf_len) return false;

    size_t off = 0;
    while (off < (size_t)req->content_len) {
        const int n = httpd_req_recv(req, buf + off, (size_t)req->content_len - off);
        if (n <= 0) return false;
        off += (size_t)n;
    }
    buf[off] = '\0';
    if (out_len) *out_len = off;
    return true;
}

static esp_err_t led_status_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    led_status_t st = {};
    led_task_get_status(&st);

    char body[512];
    const int n = snprintf(body,
                           sizeof(body),
                           "{\"ok\":true,\"running\":%s,\"paused\":%s,\"frame_ms\":%u,"
                           "\"pattern\":{\"type\":\"%s\",\"global_brightness_pct\":%u}}",
                           st.running ? "true" : "false",
                           st.paused ? "true" : "false",
                           (unsigned)st.frame_ms,
                           pattern_type_to_str(st.pat_cfg.type),
                           (unsigned)st.pat_cfg.global_brightness_pct);
    if (n <= 0 || (size_t)n >= sizeof(body)) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "encode failed");
    }
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static bool parse_pattern_type(const char *s, led_pattern_type_t *out)
{
    if (!s || !out) return false;
    if (strcmp(s, "off") == 0) {
        *out = LED_PATTERN_OFF;
        return true;
    }
    if (strcmp(s, "rainbow") == 0) {
        *out = LED_PATTERN_RAINBOW;
        return true;
    }
    if (strcmp(s, "chase") == 0) {
        *out = LED_PATTERN_CHASE;
        return true;
    }
    if (strcmp(s, "breathing") == 0) {
        *out = LED_PATTERN_BREATHING;
        return true;
    }
    if (strcmp(s, "sparkle") == 0) {
        *out = LED_PATTERN_SPARKLE;
        return true;
    }
    return false;
}

static esp_err_t led_pattern_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[256];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    }
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (!cJSON_IsString(type) || !type->valuestring) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing type");
    }

    led_pattern_type_t t;
    if (!parse_pattern_type(type->valuestring, &t)) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad type");
    }

    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_PATTERN_TYPE, .u.pattern_type = t}, 200);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    }
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_brightness_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[256];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    }
    const cJSON *pct = cJSON_GetObjectItemCaseSensitive(root, "brightness_pct");
    if (!cJSON_IsNumber(pct) || pct->valuedouble < 1 || pct->valuedouble > 100) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad brightness_pct");
    }
    const uint8_t v = (uint8_t)pct->valuedouble;
    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_GLOBAL_BRIGHTNESS_PCT, .u.brightness_pct = v}, 200);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    }
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_frame_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[256];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    }
    const cJSON *ms = cJSON_GetObjectItemCaseSensitive(root, "frame_ms");
    if (!cJSON_IsNumber(ms) || ms->valuedouble < 1 || ms->valuedouble > 1000) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad frame_ms");
    }
    const uint32_t v = (uint32_t)ms->valuedouble;
    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_FRAME_MS, .u.frame_ms = v}, 200);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    }
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_simple_post(httpd_req_t *req, led_msg_type_t t)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);
    const esp_err_t err = led_task_send(&(led_msg_t){.type = t}, 200);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    }
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_pause_post(httpd_req_t *req) { return led_simple_post(req, LED_MSG_PAUSE); }
static esp_err_t led_resume_post(httpd_req_t *req) { return led_simple_post(req, LED_MSG_RESUME); }
static esp_err_t led_clear_post(httpd_req_t *req) { return led_simple_post(req, LED_MSG_CLEAR); }

esp_err_t http_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    // Default is small; we have many endpoints (assets + /api/status + /api/led/*).
    cfg.max_uri_handlers = 16;

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
    register_or_log(&root);

    httpd_uri_t app_js = {
        .uri = "/assets/app.js",
        .method = HTTP_GET,
        .handler = asset_app_js_get,
        .user_ctx = NULL,
    };
    register_or_log(&app_js);

    httpd_uri_t app_css = {
        .uri = "/assets/app.css",
        .method = HTTP_GET,
        .handler = asset_app_css_get,
        .user_ctx = NULL,
    };
    register_or_log(&app_css);

    httpd_uri_t app_js_map = {
        .uri = "/assets/app.js.map",
        .method = HTTP_GET,
        .handler = asset_app_js_map_get,
        .user_ctx = NULL,
    };
    register_or_log(&app_js_map);

    httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get,
        .user_ctx = NULL,
    };
    register_or_log(&status);

    httpd_uri_t led_status = {
        .uri = "/api/led/status",
        .method = HTTP_GET,
        .handler = led_status_get,
        .user_ctx = NULL,
    };
    register_or_log(&led_status);

    httpd_uri_t led_pattern = {
        .uri = "/api/led/pattern",
        .method = HTTP_POST,
        .handler = led_pattern_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_pattern);

    httpd_uri_t led_brightness = {
        .uri = "/api/led/brightness",
        .method = HTTP_POST,
        .handler = led_brightness_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_brightness);

    httpd_uri_t led_frame = {
        .uri = "/api/led/frame",
        .method = HTTP_POST,
        .handler = led_frame_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_frame);

    httpd_uri_t led_pause = {
        .uri = "/api/led/pause",
        .method = HTTP_POST,
        .handler = led_pause_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_pause);

    httpd_uri_t led_resume = {
        .uri = "/api/led/resume",
        .method = HTTP_POST,
        .handler = led_resume_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_resume);

    httpd_uri_t led_clear = {
        .uri = "/api/led/clear",
        .method = HTTP_POST,
        .handler = led_clear_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_clear);

    return ESP_OK;
}
