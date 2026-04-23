/*
 * Tab5 text echo demo: HTTP server.
 */

#include "http_server.h"

#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "echo_state.h"

static const char *TAG = "tab5_text_echo_http";
static httpd_handle_t s_server = NULL;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t assets_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t assets_app_js_end[] asm("_binary_app_js_end");

static void send_json_error(httpd_req_t *req, int code, const char *msg) {
    if (code == 400) {
        httpd_resp_set_status(req, "400 Bad Request");
    } else if (code == 413) {
        httpd_resp_set_status(req, "413 Content Too Large");
    } else {
        httpd_resp_set_status(req, "500 Internal Server Error");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    char buf[128];
    const int n = snprintf(buf, sizeof(buf), "{\"ok\":false,\"error\":\"%s\"}", msg);
    if (n > 0 && n < (int)sizeof(buf)) {
        (void)httpd_resp_send(req, buf, n);
    } else {
        (void)httpd_resp_send(req, "{\"ok\":false}", HTTPD_RESP_USE_STRLEN);
    }
}

static esp_err_t send_state_json(httpd_req_t *req) {
    echo_state_snapshot_t st = {0};
    esp_err_t err = echo_state_snapshot(&st);
    if (err != ESP_OK) {
        send_json_error(req, 500, "state unavailable");
        return ESP_OK;
    }

    char escaped[(ECHO_STATE_MAX_TEXT_BYTES * 6) + 1];
    size_t out = 0;
    for (size_t i = 0; i < st.len; ++i) {
        const unsigned char c = (unsigned char)st.text[i];
        const char *rep = NULL;
        char tmp[7];

        switch (c) {
        case '\\': rep = "\\\\"; break;
        case '"': rep = "\\\""; break;
        case '\n': rep = "\\n"; break;
        case '\r': rep = "\\r"; break;
        case '\t': rep = "\\t"; break;
        case '\b': rep = "\\b"; break;
        case '\f': rep = "\\f"; break;
        default: break;
        }

        if (rep) {
            const size_t rep_len = strlen(rep);
            if (out + rep_len >= sizeof(escaped)) {
                send_json_error(req, 500, "escape overflow");
                return ESP_OK;
            }
            memcpy(escaped + out, rep, rep_len);
            out += rep_len;
            continue;
        }

        if (c < 0x20) {
            const int n = snprintf(tmp, sizeof(tmp), "\\u%04x", c);
            if (n <= 0 || out + (size_t)n >= sizeof(escaped)) {
                send_json_error(req, 500, "escape overflow");
                return ESP_OK;
            }
            memcpy(escaped + out, tmp, (size_t)n);
            out += (size_t)n;
            continue;
        }

        if (out + 1 >= sizeof(escaped)) {
            send_json_error(req, 500, "escape overflow");
            return ESP_OK;
        }
        escaped[out++] = (char)c;
    }
    escaped[out] = '\0';

    char version_buf[32];
    const int version_n = snprintf(version_buf, sizeof(version_buf), "%u", (unsigned)st.version);
    if (version_n <= 0 || version_n >= (int)sizeof(version_buf)) {
        send_json_error(req, 500, "version overflow");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    if (httpd_resp_sendstr_chunk(req, "{\"ok\":true,\"version\":") != ESP_OK ||
        httpd_resp_sendstr_chunk(req, version_buf) != ESP_OK ||
        httpd_resp_sendstr_chunk(req, ",\"text\":\"") != ESP_OK ||
        httpd_resp_sendstr_chunk(req, escaped) != ESP_OK ||
        httpd_resp_sendstr_chunk(req, "\"}") != ESP_OK ||
        httpd_resp_send_chunk(req, NULL, 0) != ESP_OK) {
        ESP_LOGW(TAG, "failed to send state response");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t root_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    const size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    return httpd_resp_send(req, (const char *)assets_index_html_start, (ssize_t)len);
}

static esp_err_t app_js_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    const size_t len = (size_t)(assets_app_js_end - assets_app_js_start);
    return httpd_resp_send(req, (const char *)assets_app_js_start, (ssize_t)len);
}

static esp_err_t health_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t state_get(httpd_req_t *req) {
    return send_state_json(req);
}

static esp_err_t text_post(httpd_req_t *req) {
    const int content_len = req->content_len;
    if (content_len < 0) {
        send_json_error(req, 400, "missing body");
        return ESP_OK;
    }
    if (content_len > ECHO_STATE_MAX_TEXT_BYTES) {
        send_json_error(req, 413, "text too long");
        return ESP_OK;
    }

    char body[ECHO_STATE_MAX_TEXT_BYTES + 1];
    int received = 0;
    while (received < content_len) {
        const int n = httpd_req_recv(req, body + received, content_len - received);
        if (n <= 0) {
            send_json_error(req, 500, "recv failed");
            return ESP_OK;
        }
        received += n;
    }
    body[received] = '\0';

    esp_err_t err = echo_state_set(body, (size_t)received);
    if (err != ESP_OK) {
        send_json_error(req, 500, "state update failed");
        return ESP_OK;
    }

    return send_state_json(req);
}

esp_err_t http_server_start(void) {
    if (s_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    cfg.max_uri_handlers = 8;

    ESP_LOGI(TAG, "starting server on port %d", cfg.server_port);
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
    httpd_uri_t app_js = {
        .uri = "/app.js",
        .method = HTTP_GET,
        .handler = app_js_get,
        .user_ctx = NULL,
    };
    httpd_uri_t health = {
        .uri = "/api/health",
        .method = HTTP_GET,
        .handler = health_get,
        .user_ctx = NULL,
    };
    httpd_uri_t state = {
        .uri = "/api/state",
        .method = HTTP_GET,
        .handler = state_get,
        .user_ctx = NULL,
    };
    httpd_uri_t text = {
        .uri = "/api/text",
        .method = HTTP_POST,
        .handler = text_post,
        .user_ctx = NULL,
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &app_js));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &health));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &text));

    return ESP_OK;
}
