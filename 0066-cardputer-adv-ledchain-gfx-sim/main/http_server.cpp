#include "http_server.h"

#include <stdbool.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "httpd_assets_embed.h"

static const char *TAG = "0066_http";

static httpd_handle_t s_server = nullptr;
static sim_engine_t *s_engine = nullptr;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");

static esp_err_t root_get(httpd_req_t *req)
{
    return httpd_assets_embed_send(req,
                                  assets_index_html_start,
                                  assets_index_html_end,
                                  "text/html; charset=utf-8",
                                  "no-store",
                                  true);
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

static void rgb_to_hex(const led_rgb8_t c, char out[8])
{
    snprintf(out, 8, "#%02X%02X%02X", (unsigned)c.r, (unsigned)c.g, (unsigned)c.b);
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
        return "off";
    }
}

static led_pattern_type_t str_to_pattern_type(const char *s, bool *ok)
{
    if (ok) *ok = true;
    if (!s) {
        if (ok) *ok = false;
        return LED_PATTERN_OFF;
    }
    if (strcmp(s, "off") == 0) return LED_PATTERN_OFF;
    if (strcmp(s, "rainbow") == 0) return LED_PATTERN_RAINBOW;
    if (strcmp(s, "chase") == 0) return LED_PATTERN_CHASE;
    if (strcmp(s, "breathing") == 0) return LED_PATTERN_BREATHING;
    if (strcmp(s, "sparkle") == 0) return LED_PATTERN_SPARKLE;
    if (ok) *ok = false;
    return LED_PATTERN_OFF;
}

static bool parse_hex_color(const char *s, led_rgb8_t *out)
{
    if (!s || !out) return false;
    if (*s == '#') s++;
    if (strlen(s) != 6) return false;
    char *end = nullptr;
    const unsigned long v = strtoul(s, &end, 16);
    if (!end || *end != '\0' || v > 0xFFFFFFul) return false;
    out->r = (uint8_t)((v >> 16) & 0xFF);
    out->g = (uint8_t)((v >> 8) & 0xFF);
    out->b = (uint8_t)(v & 0xFF);
    return true;
}

static esp_err_t status_get(httpd_req_t *req)
{
    if (!s_engine) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "engine not set");

    httpd_resp_set_type(req, "application/json; charset=utf-8");
    (void)httpd_resp_set_hdr(req, "cache-control", "no-store");

    led_pattern_cfg_t cfg;
    sim_engine_get_cfg(s_engine, &cfg);
    const uint32_t frame_ms = sim_engine_get_frame_ms(s_engine);

    cJSON *root = cJSON_CreateObject();
    if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    (void)cJSON_AddBoolToObject(root, "ok", true);
    (void)cJSON_AddNumberToObject(root, "frame_ms", (double)frame_ms);
    (void)cJSON_AddNumberToObject(root, "led_count", (double)s_engine->strip.cfg.led_count);

    cJSON *pattern = cJSON_CreateObject();
    if (!pattern) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    }
    cJSON_AddItemToObject(root, "pattern", pattern);
    (void)cJSON_AddStringToObject(pattern, "type", pattern_type_to_str(cfg.type));
    (void)cJSON_AddNumberToObject(pattern, "global_brightness_pct", (double)cfg.global_brightness_pct);

    // Provide current per-pattern params to the extent they apply.
    if (cfg.type == LED_PATTERN_RAINBOW) {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "rainbow", p);
            (void)cJSON_AddNumberToObject(p, "speed", (double)cfg.u.rainbow.speed);
            (void)cJSON_AddNumberToObject(p, "sat", (double)cfg.u.rainbow.saturation);
            (void)cJSON_AddNumberToObject(p, "spread", (double)cfg.u.rainbow.spread_x10);
        }
    } else if (cfg.type == LED_PATTERN_CHASE) {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "chase", p);
            char fg[8], bg[8];
            rgb_to_hex(cfg.u.chase.fg, fg);
            rgb_to_hex(cfg.u.chase.bg, bg);
            (void)cJSON_AddNumberToObject(p, "speed", (double)cfg.u.chase.speed);
            (void)cJSON_AddNumberToObject(p, "tail", (double)cfg.u.chase.tail_len);
            (void)cJSON_AddNumberToObject(p, "gap", (double)cfg.u.chase.gap_len);
            (void)cJSON_AddNumberToObject(p, "trains", (double)cfg.u.chase.trains);
            (void)cJSON_AddStringToObject(p, "fg", fg);
            (void)cJSON_AddStringToObject(p, "bg", bg);
            (void)cJSON_AddNumberToObject(p, "dir", (double)cfg.u.chase.dir);
            (void)cJSON_AddBoolToObject(p, "fade", cfg.u.chase.fade_tail);
        }
    } else if (cfg.type == LED_PATTERN_BREATHING) {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "breathing", p);
            char c[8];
            rgb_to_hex(cfg.u.breathing.color, c);
            (void)cJSON_AddNumberToObject(p, "speed", (double)cfg.u.breathing.speed);
            (void)cJSON_AddStringToObject(p, "color", c);
            (void)cJSON_AddNumberToObject(p, "min", (double)cfg.u.breathing.min_bri);
            (void)cJSON_AddNumberToObject(p, "max", (double)cfg.u.breathing.max_bri);
            (void)cJSON_AddNumberToObject(p, "curve", (double)cfg.u.breathing.curve);
        }
    } else if (cfg.type == LED_PATTERN_SPARKLE) {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON_AddItemToObject(pattern, "sparkle", p);
            char c[8], bg[8];
            rgb_to_hex(cfg.u.sparkle.color, c);
            rgb_to_hex(cfg.u.sparkle.background, bg);
            (void)cJSON_AddNumberToObject(p, "speed", (double)cfg.u.sparkle.speed);
            (void)cJSON_AddStringToObject(p, "color", c);
            (void)cJSON_AddNumberToObject(p, "density", (double)cfg.u.sparkle.density_pct);
            (void)cJSON_AddNumberToObject(p, "fade", (double)cfg.u.sparkle.fade_speed);
            (void)cJSON_AddNumberToObject(p, "mode", (double)cfg.u.sparkle.color_mode);
            (void)cJSON_AddStringToObject(p, "bg", bg);
        }
    }

    char *body = cJSON_PrintUnformatted(root);
    if (!body) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "encode failed");
    }
    const esp_err_t err = httpd_resp_sendstr(req, body);
    cJSON_free(body);
    cJSON_Delete(root);
    return err;
}

static esp_err_t apply_post(httpd_req_t *req)
{
    if (!s_engine) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "engine not set");

    httpd_resp_set_type(req, "application/json; charset=utf-8");
    (void)httpd_resp_set_hdr(req, "cache-control", "no-store");

    char buf[512];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "expected object");
    }

    led_pattern_cfg_t cfg;
    sim_engine_get_cfg(s_engine, &cfg);

    const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (cJSON_IsString(type) && type->valuestring) {
        bool ok = false;
        const led_pattern_type_t t = str_to_pattern_type(type->valuestring, &ok);
        if (!ok) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad type");
        }
        cfg.type = t;
    }

    const cJSON *bri = cJSON_GetObjectItemCaseSensitive(root, "brightness_pct");
    if (cJSON_IsNumber(bri)) {
        const double v = bri->valuedouble;
        if (v < 1 || v > 100) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad brightness_pct");
        }
        cfg.global_brightness_pct = (uint8_t)v;
    }

    const cJSON *frame_ms = cJSON_GetObjectItemCaseSensitive(root, "frame_ms");
    if (cJSON_IsNumber(frame_ms)) {
        const double v = frame_ms->valuedouble;
        if (v < 1 || v > 1000) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad frame_ms");
        }
        sim_engine_set_frame_ms(s_engine, (uint32_t)v);
    }

    // Optional per-pattern nested objects.
    const cJSON *rainbow = cJSON_GetObjectItemCaseSensitive(root, "rainbow");
    if (cJSON_IsObject(rainbow)) {
        cfg.type = LED_PATTERN_RAINBOW;
        const cJSON *speed = cJSON_GetObjectItemCaseSensitive(rainbow, "speed");
        const cJSON *sat = cJSON_GetObjectItemCaseSensitive(rainbow, "sat");
        const cJSON *spread = cJSON_GetObjectItemCaseSensitive(rainbow, "spread");
        if (cJSON_IsNumber(speed)) cfg.u.rainbow.speed = (uint8_t)speed->valuedouble;
        if (cJSON_IsNumber(sat)) cfg.u.rainbow.saturation = (uint8_t)sat->valuedouble;
        if (cJSON_IsNumber(spread)) cfg.u.rainbow.spread_x10 = (uint8_t)spread->valuedouble;
    }

    const cJSON *chase = cJSON_GetObjectItemCaseSensitive(root, "chase");
    if (cJSON_IsObject(chase)) {
        cfg.type = LED_PATTERN_CHASE;
        const cJSON *speed = cJSON_GetObjectItemCaseSensitive(chase, "speed");
        const cJSON *tail = cJSON_GetObjectItemCaseSensitive(chase, "tail");
        const cJSON *gap = cJSON_GetObjectItemCaseSensitive(chase, "gap");
        const cJSON *trains = cJSON_GetObjectItemCaseSensitive(chase, "trains");
        const cJSON *fg = cJSON_GetObjectItemCaseSensitive(chase, "fg");
        const cJSON *bg = cJSON_GetObjectItemCaseSensitive(chase, "bg");
        const cJSON *dir = cJSON_GetObjectItemCaseSensitive(chase, "dir");
        const cJSON *fade = cJSON_GetObjectItemCaseSensitive(chase, "fade");
        if (cJSON_IsNumber(speed)) cfg.u.chase.speed = (uint8_t)speed->valuedouble;
        if (cJSON_IsNumber(tail)) cfg.u.chase.tail_len = (uint8_t)tail->valuedouble;
        if (cJSON_IsNumber(gap)) cfg.u.chase.gap_len = (uint8_t)gap->valuedouble;
        if (cJSON_IsNumber(trains)) cfg.u.chase.trains = (uint8_t)trains->valuedouble;
        if (cJSON_IsString(fg) && fg->valuestring) (void)parse_hex_color(fg->valuestring, &cfg.u.chase.fg);
        if (cJSON_IsString(bg) && bg->valuestring) (void)parse_hex_color(bg->valuestring, &cfg.u.chase.bg);
        if (cJSON_IsNumber(dir)) cfg.u.chase.dir = (led_direction_t)((uint8_t)dir->valuedouble);
        if (cJSON_IsBool(fade)) cfg.u.chase.fade_tail = cJSON_IsTrue(fade);
    }

    const cJSON *breathing = cJSON_GetObjectItemCaseSensitive(root, "breathing");
    if (cJSON_IsObject(breathing)) {
        cfg.type = LED_PATTERN_BREATHING;
        const cJSON *speed = cJSON_GetObjectItemCaseSensitive(breathing, "speed");
        const cJSON *color = cJSON_GetObjectItemCaseSensitive(breathing, "color");
        const cJSON *min = cJSON_GetObjectItemCaseSensitive(breathing, "min");
        const cJSON *max = cJSON_GetObjectItemCaseSensitive(breathing, "max");
        const cJSON *curve = cJSON_GetObjectItemCaseSensitive(breathing, "curve");
        if (cJSON_IsNumber(speed)) cfg.u.breathing.speed = (uint8_t)speed->valuedouble;
        if (cJSON_IsString(color) && color->valuestring) (void)parse_hex_color(color->valuestring, &cfg.u.breathing.color);
        if (cJSON_IsNumber(min)) cfg.u.breathing.min_bri = (uint8_t)min->valuedouble;
        if (cJSON_IsNumber(max)) cfg.u.breathing.max_bri = (uint8_t)max->valuedouble;
        if (cJSON_IsNumber(curve)) cfg.u.breathing.curve = (led_curve_t)((uint8_t)curve->valuedouble);
    }

    const cJSON *sparkle = cJSON_GetObjectItemCaseSensitive(root, "sparkle");
    if (cJSON_IsObject(sparkle)) {
        cfg.type = LED_PATTERN_SPARKLE;
        const cJSON *speed = cJSON_GetObjectItemCaseSensitive(sparkle, "speed");
        const cJSON *color = cJSON_GetObjectItemCaseSensitive(sparkle, "color");
        const cJSON *density = cJSON_GetObjectItemCaseSensitive(sparkle, "density");
        const cJSON *fade = cJSON_GetObjectItemCaseSensitive(sparkle, "fade");
        const cJSON *mode = cJSON_GetObjectItemCaseSensitive(sparkle, "mode");
        const cJSON *bg = cJSON_GetObjectItemCaseSensitive(sparkle, "bg");
        if (cJSON_IsNumber(speed)) cfg.u.sparkle.speed = (uint8_t)speed->valuedouble;
        if (cJSON_IsString(color) && color->valuestring) (void)parse_hex_color(color->valuestring, &cfg.u.sparkle.color);
        if (cJSON_IsNumber(density)) cfg.u.sparkle.density_pct = (uint8_t)density->valuedouble;
        if (cJSON_IsNumber(fade)) cfg.u.sparkle.fade_speed = (uint8_t)fade->valuedouble;
        if (cJSON_IsNumber(mode)) cfg.u.sparkle.color_mode = (led_sparkle_color_mode_t)((uint8_t)mode->valuedouble);
        if (cJSON_IsString(bg) && bg->valuestring) (void)parse_hex_color(bg->valuestring, &cfg.u.sparkle.background);
    }

    cJSON_Delete(root);

    sim_engine_set_cfg(s_engine, &cfg);
    return status_get(req);
}

esp_err_t http_server_start(sim_engine_t *engine)
{
    if (s_server) return ESP_OK;
    s_engine = engine;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = nullptr;
        return err;
    }

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &root);

    const httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &status);

    const httpd_uri_t apply = {
        .uri = "/api/apply",
        .method = HTTP_POST,
        .handler = apply_post,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_server, &apply);

    return ESP_OK;
}

