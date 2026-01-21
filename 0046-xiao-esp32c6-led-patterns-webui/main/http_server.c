/*
 * MO-033 / 0044: Minimal HTTP server (esp_http_server) serving embedded assets.
 *
 * Static asset embedding pattern is based on 0017-atoms3r-web-ui (hardcoded routes
 * and EMBED_TXTFILES).
 */

#include "http_server.h"

#include <ctype.h>
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

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}

static bool parse_hex_rgb(const char *s, led_rgb8_t *out)
{
    if (!s || !out) return false;
    if (s[0] == '#') s++;
    if (strlen(s) != 6) return false;
    const int r0 = hex_nibble(s[0]);
    const int r1 = hex_nibble(s[1]);
    const int g0 = hex_nibble(s[2]);
    const int g1 = hex_nibble(s[3]);
    const int b0 = hex_nibble(s[4]);
    const int b1 = hex_nibble(s[5]);
    if (r0 < 0 || r1 < 0 || g0 < 0 || g1 < 0 || b0 < 0 || b1 < 0) return false;
    out->r = (uint8_t)((r0 << 4) | r1);
    out->g = (uint8_t)((g0 << 4) | g1);
    out->b = (uint8_t)((b0 << 4) | b1);
    return true;
}

static void format_hex_rgb(char out[8], led_rgb8_t c)
{
    (void)snprintf(out, 8, "#%02x%02x%02x", (unsigned)c.r, (unsigned)c.g, (unsigned)c.b);
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

    cJSON *root = cJSON_CreateObject();
    if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");

    (void)cJSON_AddBoolToObject(root, "ok", true);
    (void)cJSON_AddBoolToObject(root, "running", st.running);
    (void)cJSON_AddBoolToObject(root, "paused", st.paused);
    (void)cJSON_AddNumberToObject(root, "frame_ms", (double)st.frame_ms);

    cJSON *pattern = cJSON_CreateObject();
    if (!pattern) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    }
    cJSON_AddItemToObject(root, "pattern", pattern);
    (void)cJSON_AddStringToObject(pattern, "type", pattern_type_to_str(st.pat_cfg.type));
    (void)cJSON_AddNumberToObject(pattern, "global_brightness_pct", (double)st.pat_cfg.global_brightness_pct);

    // Pattern-specific parameters use the same names/ranges as the esp_console flags.
    switch (st.pat_cfg.type) {
    case LED_PATTERN_RAINBOW: {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "rainbow", p);
            (void)cJSON_AddNumberToObject(p, "speed", (double)st.pat_cfg.u.rainbow.speed);
            (void)cJSON_AddNumberToObject(p, "sat", (double)st.pat_cfg.u.rainbow.saturation);
            (void)cJSON_AddNumberToObject(p, "spread", (double)st.pat_cfg.u.rainbow.spread_x10);
        }
        break;
    }
    case LED_PATTERN_CHASE: {
        char fg[8], bg[8];
        format_hex_rgb(fg, st.pat_cfg.u.chase.fg);
        format_hex_rgb(bg, st.pat_cfg.u.chase.bg);
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "chase", p);
            (void)cJSON_AddNumberToObject(p, "speed", (double)st.pat_cfg.u.chase.speed);
            (void)cJSON_AddNumberToObject(p, "tail", (double)st.pat_cfg.u.chase.tail_len);
            (void)cJSON_AddNumberToObject(p, "gap", (double)st.pat_cfg.u.chase.gap_len);
            (void)cJSON_AddNumberToObject(p, "trains", (double)st.pat_cfg.u.chase.trains);
            (void)cJSON_AddStringToObject(p, "fg", fg);
            (void)cJSON_AddStringToObject(p, "bg", bg);
            (void)cJSON_AddStringToObject(p,
                                         "dir",
                                         (st.pat_cfg.u.chase.dir == LED_DIR_REVERSE) ? "reverse"
                                         : (st.pat_cfg.u.chase.dir == LED_DIR_BOUNCE) ? "bounce"
                                                                                      : "forward");
            (void)cJSON_AddBoolToObject(p, "fade", st.pat_cfg.u.chase.fade_tail);
        }
        break;
    }
    case LED_PATTERN_BREATHING: {
        char c[8];
        format_hex_rgb(c, st.pat_cfg.u.breathing.color);
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "breathing", p);
            (void)cJSON_AddNumberToObject(p, "speed", (double)st.pat_cfg.u.breathing.speed);
            (void)cJSON_AddStringToObject(p, "color", c);
            (void)cJSON_AddNumberToObject(p, "min", (double)st.pat_cfg.u.breathing.min_bri);
            (void)cJSON_AddNumberToObject(p, "max", (double)st.pat_cfg.u.breathing.max_bri);
            (void)cJSON_AddStringToObject(p,
                                         "curve",
                                         (st.pat_cfg.u.breathing.curve == LED_CURVE_LINEAR) ? "linear"
                                         : (st.pat_cfg.u.breathing.curve == LED_CURVE_EASE_IN_OUT) ? "ease"
                                                                                                   : "sine");
        }
        break;
    }
    case LED_PATTERN_SPARKLE: {
        char c[8], bg[8];
        format_hex_rgb(c, st.pat_cfg.u.sparkle.color);
        format_hex_rgb(bg, st.pat_cfg.u.sparkle.background);
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "sparkle", p);
            (void)cJSON_AddNumberToObject(p, "speed", (double)st.pat_cfg.u.sparkle.speed);
            (void)cJSON_AddStringToObject(p, "color", c);
            (void)cJSON_AddNumberToObject(p, "density", (double)st.pat_cfg.u.sparkle.density_pct);
            (void)cJSON_AddNumberToObject(p, "fade", (double)st.pat_cfg.u.sparkle.fade_speed);
            (void)cJSON_AddStringToObject(p,
                                         "mode",
                                         (st.pat_cfg.u.sparkle.color_mode == LED_SPARKLE_RANDOM) ? "random"
                                         : (st.pat_cfg.u.sparkle.color_mode == LED_SPARKLE_RAINBOW) ? "rainbow"
                                                                                                    : "fixed");
            (void)cJSON_AddStringToObject(p, "bg", bg);
        }
        break;
    }
    default:
        break;
    }

    char *body = cJSON_PrintUnformatted(root);
    if (!body) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "encode failed");
    }
    const esp_err_t err = httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
    cJSON_free(body);
    cJSON_Delete(root);
    return err;
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

static led_rainbow_cfg_t default_rainbow_cfg(void)
{
    return (led_rainbow_cfg_t){
        .speed = 5,
        .saturation = 100,
        .spread_x10 = 10,
    };
}

static led_chase_cfg_t default_chase_cfg(void)
{
    return (led_chase_cfg_t){
        .speed = 30,
        .tail_len = 16,
        .gap_len = 8,
        .trains = 1,
        .fg = (led_rgb8_t){.r = 255, .g = 255, .b = 255},
        .bg = (led_rgb8_t){.r = 0, .g = 0, .b = 0},
        .dir = LED_DIR_FORWARD,
        .fade_tail = true,
    };
}

static led_breathing_cfg_t default_breathing_cfg(void)
{
    return (led_breathing_cfg_t){
        .speed = 6,
        .color = (led_rgb8_t){.r = 80, .g = 160, .b = 255},
        .min_bri = 0,
        .max_bri = 255,
        .curve = LED_CURVE_SINE,
    };
}

static led_sparkle_cfg_t default_sparkle_cfg(void)
{
    return (led_sparkle_cfg_t){
        .speed = 10,
        .color = (led_rgb8_t){.r = 255, .g = 255, .b = 255},
        .density_pct = 25,
        .fade_speed = 24,
        .color_mode = LED_SPARKLE_RANDOM,
        .background = (led_rgb8_t){.r = 0, .g = 0, .b = 0},
    };
}

static bool parse_direction(const char *s, led_direction_t *out)
{
    if (!s || !out) return false;
    if (strcmp(s, "forward") == 0) {
        *out = LED_DIR_FORWARD;
        return true;
    }
    if (strcmp(s, "reverse") == 0) {
        *out = LED_DIR_REVERSE;
        return true;
    }
    if (strcmp(s, "bounce") == 0) {
        *out = LED_DIR_BOUNCE;
        return true;
    }
    return false;
}

static bool parse_curve(const char *s, led_curve_t *out)
{
    if (!s || !out) return false;
    if (strcmp(s, "sine") == 0) {
        *out = LED_CURVE_SINE;
        return true;
    }
    if (strcmp(s, "linear") == 0) {
        *out = LED_CURVE_LINEAR;
        return true;
    }
    if (strcmp(s, "ease") == 0) {
        *out = LED_CURVE_EASE_IN_OUT;
        return true;
    }
    return false;
}

static bool parse_sparkle_mode(const char *s, led_sparkle_color_mode_t *out)
{
    if (!s || !out) return false;
    if (strcmp(s, "fixed") == 0) {
        *out = LED_SPARKLE_FIXED;
        return true;
    }
    if (strcmp(s, "random") == 0) {
        *out = LED_SPARKLE_RANDOM;
        return true;
    }
    if (strcmp(s, "rainbow") == 0) {
        *out = LED_SPARKLE_RAINBOW;
        return true;
    }
    return false;
}

static esp_err_t led_pattern_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[512];
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

    char buf[512];
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

    char buf[512];
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

static esp_err_t led_rainbow_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[512];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");

    led_status_t st = {};
    led_task_get_status(&st);
    led_rainbow_cfg_t cfg = (st.pat_cfg.type == LED_PATTERN_RAINBOW) ? st.pat_cfg.u.rainbow : default_rainbow_cfg();

    const cJSON *speed = cJSON_GetObjectItemCaseSensitive(root, "speed");
    const cJSON *sat = cJSON_GetObjectItemCaseSensitive(root, "sat");
    const cJSON *spread = cJSON_GetObjectItemCaseSensitive(root, "spread");

    if (cJSON_IsNumber(speed)) {
        if (speed->valuedouble < 0 || speed->valuedouble > 20) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad speed");
        }
        cfg.speed = (uint8_t)speed->valuedouble;
    }
    if (cJSON_IsNumber(sat)) {
        if (sat->valuedouble < 0 || sat->valuedouble > 100) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad sat");
        }
        cfg.saturation = (uint8_t)sat->valuedouble;
    }
    if (cJSON_IsNumber(spread)) {
        if (spread->valuedouble < 1 || spread->valuedouble > 50) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad spread");
        }
        cfg.spread_x10 = (uint8_t)spread->valuedouble;
    }

    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_RAINBOW, .u.rainbow = cfg}, 200);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_chase_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[512];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");

    led_status_t st = {};
    led_task_get_status(&st);
    led_chase_cfg_t cfg = (st.pat_cfg.type == LED_PATTERN_CHASE) ? st.pat_cfg.u.chase : default_chase_cfg();

    const cJSON *speed = cJSON_GetObjectItemCaseSensitive(root, "speed");
    const cJSON *tail = cJSON_GetObjectItemCaseSensitive(root, "tail");
    const cJSON *gap = cJSON_GetObjectItemCaseSensitive(root, "gap");
    const cJSON *trains = cJSON_GetObjectItemCaseSensitive(root, "trains");
    const cJSON *fg = cJSON_GetObjectItemCaseSensitive(root, "fg");
    const cJSON *bg = cJSON_GetObjectItemCaseSensitive(root, "bg");
    const cJSON *dir = cJSON_GetObjectItemCaseSensitive(root, "dir");
    const cJSON *fade = cJSON_GetObjectItemCaseSensitive(root, "fade");

    if (cJSON_IsNumber(speed)) {
        if (speed->valuedouble < 0 || speed->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad speed");
        }
        cfg.speed = (uint8_t)speed->valuedouble;
    }
    if (cJSON_IsNumber(tail)) {
        if (tail->valuedouble < 1 || tail->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad tail");
        }
        cfg.tail_len = (uint8_t)tail->valuedouble;
    }
    if (cJSON_IsNumber(gap)) {
        if (gap->valuedouble < 0 || gap->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad gap");
        }
        cfg.gap_len = (uint8_t)gap->valuedouble;
    }
    if (cJSON_IsNumber(trains)) {
        if (trains->valuedouble < 1 || trains->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad trains");
        }
        cfg.trains = (uint8_t)trains->valuedouble;
    }
    if (cJSON_IsString(fg) && fg->valuestring) {
        led_rgb8_t c;
        if (!parse_hex_rgb(fg->valuestring, &c)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad fg");
        }
        cfg.fg = c;
    }
    if (cJSON_IsString(bg) && bg->valuestring) {
        led_rgb8_t c;
        if (!parse_hex_rgb(bg->valuestring, &c)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad bg");
        }
        cfg.bg = c;
    }
    if (cJSON_IsString(dir) && dir->valuestring) {
        led_direction_t d;
        if (!parse_direction(dir->valuestring, &d)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad dir");
        }
        cfg.dir = d;
    }
    if (cJSON_IsBool(fade)) {
        cfg.fade_tail = cJSON_IsTrue(fade);
    } else if (cJSON_IsNumber(fade)) {
        if (fade->valuedouble != 0 && fade->valuedouble != 1) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad fade");
        }
        cfg.fade_tail = (fade->valuedouble != 0);
    }

    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_CHASE, .u.chase = cfg}, 200);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_breathing_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[512];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");

    led_status_t st = {};
    led_task_get_status(&st);
    led_breathing_cfg_t cfg = (st.pat_cfg.type == LED_PATTERN_BREATHING) ? st.pat_cfg.u.breathing : default_breathing_cfg();

    const cJSON *speed = cJSON_GetObjectItemCaseSensitive(root, "speed");
    const cJSON *color = cJSON_GetObjectItemCaseSensitive(root, "color");
    const cJSON *min = cJSON_GetObjectItemCaseSensitive(root, "min");
    const cJSON *max = cJSON_GetObjectItemCaseSensitive(root, "max");
    const cJSON *curve = cJSON_GetObjectItemCaseSensitive(root, "curve");

    if (cJSON_IsNumber(speed)) {
        if (speed->valuedouble < 0 || speed->valuedouble > 20) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad speed");
        }
        cfg.speed = (uint8_t)speed->valuedouble;
    }
    if (cJSON_IsString(color) && color->valuestring) {
        led_rgb8_t c;
        if (!parse_hex_rgb(color->valuestring, &c)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad color");
        }
        cfg.color = c;
    }
    if (cJSON_IsNumber(min)) {
        if (min->valuedouble < 0 || min->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad min");
        }
        cfg.min_bri = (uint8_t)min->valuedouble;
    }
    if (cJSON_IsNumber(max)) {
        if (max->valuedouble < 0 || max->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad max");
        }
        cfg.max_bri = (uint8_t)max->valuedouble;
    }
    if (cJSON_IsString(curve) && curve->valuestring) {
        led_curve_t c;
        if (!parse_curve(curve->valuestring, &c)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad curve");
        }
        cfg.curve = c;
    }

    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_BREATHING, .u.breathing = cfg}, 200);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t led_sparkle_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    maybe_set_no_store(req);

    char buf[512];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");

    led_status_t st = {};
    led_task_get_status(&st);
    led_sparkle_cfg_t cfg = (st.pat_cfg.type == LED_PATTERN_SPARKLE) ? st.pat_cfg.u.sparkle : default_sparkle_cfg();

    const cJSON *speed = cJSON_GetObjectItemCaseSensitive(root, "speed");
    const cJSON *color = cJSON_GetObjectItemCaseSensitive(root, "color");
    const cJSON *density = cJSON_GetObjectItemCaseSensitive(root, "density");
    const cJSON *fade = cJSON_GetObjectItemCaseSensitive(root, "fade");
    const cJSON *mode = cJSON_GetObjectItemCaseSensitive(root, "mode");
    const cJSON *bg = cJSON_GetObjectItemCaseSensitive(root, "bg");

    if (cJSON_IsNumber(speed)) {
        if (speed->valuedouble < 0 || speed->valuedouble > 20) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad speed");
        }
        cfg.speed = (uint8_t)speed->valuedouble;
    }
    if (cJSON_IsString(color) && color->valuestring) {
        led_rgb8_t c;
        if (!parse_hex_rgb(color->valuestring, &c)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad color");
        }
        cfg.color = c;
    }
    if (cJSON_IsNumber(density)) {
        if (density->valuedouble < 0 || density->valuedouble > 100) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad density");
        }
        cfg.density_pct = (uint8_t)density->valuedouble;
    }
    if (cJSON_IsNumber(fade)) {
        if (fade->valuedouble < 1 || fade->valuedouble > 255) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad fade");
        }
        cfg.fade_speed = (uint8_t)fade->valuedouble;
    }
    if (cJSON_IsString(mode) && mode->valuestring) {
        led_sparkle_color_mode_t m;
        if (!parse_sparkle_mode(mode->valuestring, &m)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad mode");
        }
        cfg.color_mode = m;
    }
    if (cJSON_IsString(bg) && bg->valuestring) {
        led_rgb8_t c;
        if (!parse_hex_rgb(bg->valuestring, &c)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad bg");
        }
        cfg.background = c;
    }

    cJSON_Delete(root);

    const esp_err_t err = led_task_send(&(led_msg_t){.type = LED_MSG_SET_SPARKLE, .u.sparkle = cfg}, 200);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "apply failed");
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
    cfg.max_uri_handlers = 24;

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

    httpd_uri_t led_rainbow = {
        .uri = "/api/led/rainbow",
        .method = HTTP_POST,
        .handler = led_rainbow_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_rainbow);

    httpd_uri_t led_chase = {
        .uri = "/api/led/chase",
        .method = HTTP_POST,
        .handler = led_chase_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_chase);

    httpd_uri_t led_breathing = {
        .uri = "/api/led/breathing",
        .method = HTTP_POST,
        .handler = led_breathing_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_breathing);

    httpd_uri_t led_sparkle = {
        .uri = "/api/led/sparkle",
        .method = HTTP_POST,
        .handler = led_sparkle_post,
        .user_ctx = NULL,
    };
    register_or_log(&led_sparkle);

    return ESP_OK;
}
