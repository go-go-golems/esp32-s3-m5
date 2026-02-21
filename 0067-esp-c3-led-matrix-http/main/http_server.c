#include "http_server.h"

#include <stdbool.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "httpd_assets_embed.h"
#include "matrix_engine.h"
#include "sdkconfig.h"

static const char *TAG = "0067_http";
static httpd_handle_t s_server;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");

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

    cJSON *root = cJSON_CreateObject();
    if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddBoolToObject(root, "ready", st.ready);
    cJSON_AddStringToObject(root, "mode", mode);
    cJSON_AddStringToObject(root, "text", st.text);
    cJSON_AddNumberToObject(root, "chain_len", st.chain_len);
    cJSON_AddNumberToObject(root, "width", st.width);
    cJSON_AddNumberToObject(root, "spi_hz", st.spi_hz);
    cJSON_AddNumberToObject(root, "fps", st.fps);
    cJSON_AddNumberToObject(root, "pause_ms", st.pause_ms);
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

    if (!cJSON_IsString(mode) || !mode->valuestring || !cJSON_IsString(text) || !text->valuestring) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing mode/text");
    }

    uint32_t fps_v = cJSON_IsNumber(fps) ? (uint32_t)fps->valuedouble : 15;
    uint32_t pause_v = cJSON_IsNumber(pause) ? (uint32_t)pause->valuedouble : 250;

    esp_err_t err = ESP_ERR_INVALID_ARG;
    if (strcmp(mode->valuestring, "scroll") == 0) err = matrix_engine_start_scroll(text->valuestring, fps_v, pause_v, false);
    else if (strcmp(mode->valuestring, "wave") == 0) err = matrix_engine_start_scroll(text->valuestring, fps_v, pause_v, true);
    else if (strcmp(mode->valuestring, "drop") == 0) err = matrix_engine_start_drop(text->valuestring, fps_v, pause_v);

    cJSON_Delete(root);
    if (err != ESP_OK) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "anim failed");
    return send_status(req);
}

esp_err_t http_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.max_uri_handlers = 12;

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

    return ESP_OK;
}
