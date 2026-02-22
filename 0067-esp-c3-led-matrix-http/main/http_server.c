#include "http_server.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "httpd_assets_embed.h"
#include "matrix_engine.h"
#include "mqjs/js_runtime_bridge.h"
#include "sdkconfig.h"

static const char *TAG = "0067_http";
static httpd_handle_t s_server;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");

static const char *loop_mode_to_str(matrix_scroll_loop_t mode)
{
    if (mode == MATRIX_SCROLL_LOOP_WRAP) return "wrap";
    if (mode == MATRIX_SCROLL_LOOP_RIGHT_EXIT) return "right_exit";
    return "gap";
}

static bool loop_mode_from_str(const char *s, matrix_scroll_loop_t *out)
{
    if (!s || !out) return false;
    if (strcmp(s, "gap") == 0) {
        *out = MATRIX_SCROLL_LOOP_GAP;
        return true;
    }
    if (strcmp(s, "wrap") == 0 || strcmp(s, "loop") == 0 || strcmp(s, "direct") == 0) {
        *out = MATRIX_SCROLL_LOOP_WRAP;
        return true;
    }
    if (strcmp(s, "right_exit") == 0 || strcmp(s, "exit_right") == 0) {
        *out = MATRIX_SCROLL_LOOP_RIGHT_EXIT;
        return true;
    }
    return false;
}

static bool json_read_body(httpd_req_t *req, char *buf, size_t buf_len, size_t *out_len)
{
    if (!req || !buf || buf_len == 0) return false;
    if (req->content_len <= 0 || (size_t)req->content_len >= buf_len) return false;

    size_t off = 0;
    while (off < (size_t)req->content_len) {
        int n = httpd_req_recv(req, buf + off, (size_t)req->content_len - off);
        if (n <= 0) return false;
        off += (size_t)n;
    }
    buf[off] = 0;
    if (out_len) *out_len = off;
    return true;
}

static esp_err_t root_get(httpd_req_t *req)
{
    return httpd_assets_embed_send(req,
                                   assets_index_html_start,
                                   assets_index_html_end,
                                   "text/html; charset=utf-8",
                                   "no-store",
                                   true);
}

static esp_err_t send_status(httpd_req_t *req)
{
    matrix_status_t st = {0};
    if (matrix_engine_get_status(&st) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "engine status failed");
    }

    const char *mode = "idle";
    if (st.mode == MATRIX_MODE_TEXT) mode = "text";
    if (st.mode == MATRIX_MODE_SCROLL) mode = "scroll";
    if (st.mode == MATRIX_MODE_DROP) mode = "drop";
    if (st.mode == MATRIX_MODE_SCRIPT) mode = "script";

    cJSON *root = cJSON_CreateObject();
    if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddBoolToObject(root, "ready", st.ready);
    cJSON_AddStringToObject(root, "mode", mode);
    cJSON_AddStringToObject(root, "text", st.text);
    cJSON_AddNumberToObject(root, "chain_len", st.chain_len);
    cJSON_AddNumberToObject(root, "width", st.width);
    cJSON_AddNumberToObject(root, "spi_hz", st.spi_hz);
    cJSON_AddNumberToObject(root, "intensity", st.intensity);
    cJSON_AddBoolToObject(root, "test_mode", st.test_mode);
    cJSON_AddNumberToObject(root, "fps", st.fps);
    cJSON_AddNumberToObject(root, "pause_ms", st.pause_ms);
    cJSON_AddNumberToObject(root, "repeat_count", st.repeat_count);
    cJSON_AddStringToObject(root, "loop_mode", loop_mode_to_str(st.scroll_loop));
    cJSON_AddBoolToObject(root, "rotate_180", st.rotate_180);
    cJSON_AddBoolToObject(root, "reverse_modules", st.reverse_modules);
    cJSON_AddBoolToObject(root, "flip_vertical", st.flip_vertical);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "encode failed");

    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "cache-control", "no-store");
    esp_err_t err = httpd_resp_sendstr(req, body);
    cJSON_free(body);
    return err;
}

static esp_err_t matrix_status_get(httpd_req_t *req)
{
    return send_status(req);
}

static esp_err_t matrix_stop_post(httpd_req_t *req)
{
    (void)req;
    (void)matrix_engine_stop();
    return send_status(req);
}

static esp_err_t matrix_text_post(httpd_req_t *req)
{
    char buf[CONFIG_TUTORIAL_0067_HTTP_MAX_BODY];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    const cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");
    if (!cJSON_IsString(text) || !text->valuestring) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing text");
    }
    esp_err_t err = matrix_engine_set_text(text->valuestring);
    cJSON_Delete(root);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "set text failed");
    return send_status(req);
}

static esp_err_t matrix_anim_post(httpd_req_t *req)
{
    char buf[CONFIG_TUTORIAL_0067_HTTP_MAX_BODY];
    size_t len = 0;
    if (!json_read_body(req, buf, sizeof(buf), &len)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");

    const cJSON *mode = cJSON_GetObjectItemCaseSensitive(root, "mode");
    const cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");
    const cJSON *fps = cJSON_GetObjectItemCaseSensitive(root, "fps");
    const cJSON *pause = cJSON_GetObjectItemCaseSensitive(root, "pause_ms");
    const cJSON *repeat = cJSON_GetObjectItemCaseSensitive(root, "repeat_count");
    const cJSON *loop_mode = cJSON_GetObjectItemCaseSensitive(root, "loop_mode");

    if (!cJSON_IsString(mode) || !mode->valuestring || !cJSON_IsString(text) || !text->valuestring) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing mode/text");
    }

    uint32_t fps_v = cJSON_IsNumber(fps) ? (uint32_t)fps->valuedouble : 15;
    uint32_t pause_v = cJSON_IsNumber(pause) ? (uint32_t)pause->valuedouble : 250;
    uint32_t repeat_v = cJSON_IsNumber(repeat) ? (uint32_t)repeat->valuedouble : 0;
    matrix_scroll_loop_t loop_v = MATRIX_SCROLL_LOOP_GAP;
    if (cJSON_IsString(loop_mode) && loop_mode->valuestring && loop_mode->valuestring[0] != '\0') {
        if (!loop_mode_from_str(loop_mode->valuestring, &loop_v)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid loop_mode");
        }
    }

    esp_err_t err = ESP_ERR_INVALID_ARG;
    if (strcmp(mode->valuestring, "scroll") == 0) err = matrix_engine_start_scroll(text->valuestring, fps_v, pause_v, repeat_v, false, loop_v);
    else if (strcmp(mode->valuestring, "wave") == 0) err = matrix_engine_start_scroll(text->valuestring, fps_v, pause_v, repeat_v, true, loop_v);
    else if (strcmp(mode->valuestring, "drop") == 0) err = matrix_engine_start_drop(text->valuestring, fps_v, pause_v, repeat_v);

    cJSON_Delete(root);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "anim failed");
    return send_status(req);
}

static esp_err_t send_text(httpd_req_t *req, const char *content_type, const char *body)
{
    httpd_resp_set_type(req, content_type ? content_type : "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "cache-control", "no-store");
    return httpd_resp_sendstr(req, body ? body : "");
}

static esp_err_t js_eval_post(httpd_req_t *req)
{
    const int max_body = CONFIG_TUTORIAL_0067_JS_MAX_BODY;
    if (max_body <= 0) {
        return send_text(req, "application/json; charset=utf-8",
                         "{\"ok\":false,\"output\":\"\",\"error\":\"server misconfigured\",\"timed_out\":false}");
    }
    if (req->content_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
        return ESP_OK;
    }
    if (req->content_len > max_body) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "body too large");
        return ESP_OK;
    }

    const size_t n = (size_t)req->content_len;
    char *buf = (char *)malloc(n + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
        return ESP_OK;
    }

    size_t off = 0;
    while (off < n) {
        const int got = httpd_req_recv(req, buf + off, n - off);
        if (got <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
            return ESP_OK;
        }
        off += (size_t)got;
    }
    buf[n] = '\0';

    char *json = NULL;
    const esp_err_t st = js_service_eval_json(buf, n, 0, "<http>", &json);
    free(buf);
    if (st != ESP_OK || !json) {
        if (json) js_service_free(json);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "js eval failed");
    }
    esp_err_t err = send_text(req, "application/json; charset=utf-8", json);
    js_service_free(json);
    return err;
}

static esp_err_t js_reset_post(httpd_req_t *req)
{
    (void)req;
    if (js_service_reset() != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "reset failed");
    }
    return send_text(req, "application/json; charset=utf-8", "{\"ok\":true}");
}

static esp_err_t js_hard_reset_post(httpd_req_t *req)
{
    (void)req;
    if (js_service_hard_reset() != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "hard reset failed");
    }
    return send_text(req, "application/json; charset=utf-8", "{\"ok\":true}");
}

static esp_err_t js_stop_post(httpd_req_t *req)
{
    (void)req;
    if (js_service_request_stop() != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "stop failed");
    }
    return send_text(req, "application/json; charset=utf-8", "{\"ok\":true}");
}

static esp_err_t js_mem_get(httpd_req_t *req)
{
    char *out = NULL;
    const esp_err_t st = js_service_dump_memory_text(&out);
    if (st != ESP_OK || !out) {
        if (out) js_service_free(out);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "dump failed");
    }
    esp_err_t err = send_text(req, "text/plain; charset=utf-8", out);
    js_service_free(out);
    return err;
}

static esp_err_t js_status_get(httpd_req_t *req)
{
    js_service_status_t st = {0};
    if (js_service_get_status(&st) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "status failed");
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddBoolToObject(root, "started", st.started);
    cJSON_AddBoolToObject(root, "busy", st.busy);
    cJSON_AddBoolToObject(root, "stop_requested", st.stop_requested);
    cJSON_AddBoolToObject(root, "last_timed_out", st.last_timed_out);
    cJSON_AddNumberToObject(root, "eval_count", st.eval_count);
    cJSON_AddNumberToObject(root, "last_eval_ms", st.last_eval_ms);
    cJSON_AddNumberToObject(root, "timer_cb_keys", st.timer_cb_keys);
    cJSON_AddNumberToObject(root, "timer_cb_active", st.timer_cb_active);
    cJSON_AddNumberToObject(root, "timer_cb_keys_high_water", st.timer_cb_keys_high_water);
    cJSON_AddNumberToObject(root, "animations_registered", st.animations_registered);
    cJSON_AddStringToObject(root, "active_animation", st.active_animation);
    cJSON_AddNumberToObject(root, "heap_free_8bit", st.heap_free_8bit);
    cJSON_AddNumberToObject(root, "heap_largest_free_8bit", st.heap_largest_free_8bit);
    cJSON_AddNumberToObject(root, "heap_min_free_8bit", st.heap_min_free_8bit);
    cJSON_AddStringToObject(root, "last_error", st.last_error);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "encode failed");
    esp_err_t err = send_text(req, "application/json; charset=utf-8", body);
    cJSON_free(body);
    return err;
}

esp_err_t http_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.max_uri_handlers = 13;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    httpd_register_uri_handler(s_server, &root);

    httpd_uri_t st = {.uri = "/api/matrix/status", .method = HTTP_GET, .handler = matrix_status_get};
    httpd_register_uri_handler(s_server, &st);

    httpd_uri_t text = {.uri = "/api/matrix/text", .method = HTTP_POST, .handler = matrix_text_post};
    httpd_register_uri_handler(s_server, &text);

    httpd_uri_t anim = {.uri = "/api/matrix/anim", .method = HTTP_POST, .handler = matrix_anim_post};
    httpd_register_uri_handler(s_server, &anim);

    httpd_uri_t stop = {.uri = "/api/matrix/stop", .method = HTTP_POST, .handler = matrix_stop_post};
    httpd_register_uri_handler(s_server, &stop);

    httpd_uri_t js_eval = {.uri = "/api/js/eval", .method = HTTP_POST, .handler = js_eval_post};
    httpd_register_uri_handler(s_server, &js_eval);

    httpd_uri_t js_reset = {.uri = "/api/js/reset", .method = HTTP_POST, .handler = js_reset_post};
    httpd_register_uri_handler(s_server, &js_reset);

    httpd_uri_t js_hard_reset = {.uri = "/api/js/reset-hard", .method = HTTP_POST, .handler = js_hard_reset_post};
    httpd_register_uri_handler(s_server, &js_hard_reset);

    httpd_uri_t js_stop = {.uri = "/api/js/stop", .method = HTTP_POST, .handler = js_stop_post};
    httpd_register_uri_handler(s_server, &js_stop);

    httpd_uri_t js_mem = {.uri = "/api/js/mem", .method = HTTP_GET, .handler = js_mem_get};
    httpd_register_uri_handler(s_server, &js_mem);

    httpd_uri_t js_status = {.uri = "/api/js/status", .method = HTTP_GET, .handler = js_status_get};
    httpd_register_uri_handler(s_server, &js_status);

    return ESP_OK;
}
