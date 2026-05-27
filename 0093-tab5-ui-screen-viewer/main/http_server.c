/*
 * Tab5 UI screen viewer: HTTP server.
 *
 * Endpoints:
 *   GET  /            — Web UI (drag-drop image upload)
 *   GET  /app.js      — Frontend JavaScript
 *   GET  /api/health  — Health check
 *   GET  /api/screen  — Screen metadata (resolution, format, state)
 *   POST /api/upload  — Upload raw or gzip-compressed RGB565 pixel data
 *   POST /api/clear   — Fill screen with black
 */

#include "http_server.h"

#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "miniz.h"

#include "display_app.h"

static const char *TAG = "tab5_viewer_http";
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

/* ---- Static assets ---- */

static esp_err_t root_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    /* EMBED_TXTFILES appends a NUL terminator; strip it for HTTP. */
    size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    if (len > 0 && assets_index_html_start[len - 1] == '\0') len--;
    return httpd_resp_send(req, (const char *)assets_index_html_start, (ssize_t)len);
}

static esp_err_t app_js_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    /* EMBED_TXTFILES appends a NUL terminator; strip it for HTTP. */
    size_t len = (size_t)(assets_app_js_end - assets_app_js_start);
    if (len > 0 && assets_app_js_start[len - 1] == '\0') len--;
    return httpd_resp_send(req, (const char *)assets_app_js_start, (ssize_t)len);
}

/* ---- API endpoints ---- */

static esp_err_t health_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t screen_get(httpd_req_t *req) {
    const int w = display_app_get_width();
    const int h = display_app_get_height();
    const size_t buf_size = display_app_get_buf_size();

    char json[256];
    const int n = snprintf(json, sizeof(json),
        "{\"ok\":true,\"width\":%d,\"height\":%d,\"format\":\"rgb565\",\"buf_size\":%zu,\"has_image\":%s}",
        w, h, buf_size,
        display_app_has_image() ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    if (n > 0 && n < (int)sizeof(json)) {
        return httpd_resp_send(req, json, n);
    }
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* Check whether the request body is zlib-compressed (Content-Encoding: deflate).
 * Browser CompressionStream('deflate') produces zlib format (RFC 1950),
 * which mz_uncompress() handles directly. */
static bool is_deflate_request(httpd_req_t *req) {
    char hdr[32] = {0};
    if (httpd_req_get_hdr_value_str(req, "Content-Encoding", hdr, sizeof(hdr)) == ESP_OK) {
        return (strcmp(hdr, "deflate") == 0);
    }
    return false;
}

static esp_err_t upload_post(httpd_req_t *req) {
    const int content_len = req->content_len;
    const size_t expected = display_app_get_buf_size();
    const bool deflated = is_deflate_request(req);

    if (content_len < 0) {
        send_json_error(req, 400, "missing body");
        return ESP_OK;
    }
    const size_t max_recv = deflated ? expected * 2 : expected;
    if ((size_t)content_len > max_recv) {
        send_json_error(req, 413, "payload too large");
        return ESP_OK;
    }

    uint8_t *buf = display_app_get_buffer();
    if (!buf) {
        send_json_error(req, 500, "no screen buffer");
        return ESP_OK;
    }

    uint8_t *recv_buf = heap_caps_malloc(max_recv, MALLOC_CAP_SPIRAM);
    if (!recv_buf) {
        send_json_error(req, 500, "no recv memory");
        return ESP_OK;
    }

    int received = 0;
    while (received < content_len) {
        const int n = httpd_req_recv(req, (char *)recv_buf + received, content_len - received);
        if (n <= 0) {
            free(recv_buf);
            send_json_error(req, 500, "recv failed");
            return ESP_OK;
        }
        received += n;
    }

    if (deflated) {
        /* Decompress zlib format (RFC 1950) to the screen buffer.
         * Browser CompressionStream('deflate') produces this format.
         * tinfl_decompress_mem_to_mem is in ROM — zero flash cost.
         * TINFL_FLAG_PARSE_ZLIB_HEADER tells it to parse the 2-byte zlib
         * header and 4-byte Adler-32 trailer. */
        size_t dec_len = tinfl_decompress_mem_to_mem(
            buf, expected,
            recv_buf, received,
            TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        free(recv_buf);

        if (dec_len == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED || dec_len > expected) {
            ESP_LOGE(TAG, "decompress failed");
            memset(buf, 0, expected);
            send_json_error(req, 500, "decompress failed");
            return ESP_OK;
        }
        if (dec_len < expected) {
            memset(buf + dec_len, 0, expected - dec_len);
        }

        display_app_invalidate();
        ESP_LOGI(TAG, "deflate upload: %d -> %zu/%zu bytes",
                 content_len, dec_len, expected);
    } else {
        /* Raw RGB565: copy to screen buffer. */
        memcpy(buf, recv_buf, (size_t)received);
        free(recv_buf);

        if ((size_t)received < expected) {
            memset(buf + received, 0, expected - (size_t)received);
        }

        display_app_invalidate();
        ESP_LOGI(TAG, "image uploaded: %d/%zu bytes", received, expected);
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static esp_err_t clear_post(httpd_req_t *req) {
    display_app_clear();
    ESP_LOGI(TAG, "screen cleared");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---- Server lifecycle ---- */

esp_err_t http_server_start(void) {
    if (s_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    /* The HTTP handler needs a large stack because tinfl_decompress_mem_to_mem
     * places a ~43 KB decompressor struct + LZ dictionary on the stack.
     * With 32 MB PSRAM this is not a constraint; 48 KB stack is safe. */
    cfg.stack_size = 48 * 1024;
    cfg.max_uri_handlers = 8;
    /* recv_wait_timeout is 5s by default — too short for 1.8 MB uploads.
     * Set to 30s to allow large image transfers over WiFi. */
    cfg.recv_wait_timeout = 30;

    ESP_LOGI(TAG, "starting server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    /* Static assets */
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
    };
    httpd_uri_t app_js = {
        .uri = "/app.js",
        .method = HTTP_GET,
        .handler = app_js_get,
    };

    /* API endpoints */
    httpd_uri_t health = {
        .uri = "/api/health",
        .method = HTTP_GET,
        .handler = health_get,
    };
    httpd_uri_t screen = {
        .uri = "/api/screen",
        .method = HTTP_GET,
        .handler = screen_get,
    };
    httpd_uri_t upload = {
        .uri = "/api/upload",
        .method = HTTP_POST,
        .handler = upload_post,
    };
    httpd_uri_t clear = {
        .uri = "/api/clear",
        .method = HTTP_POST,
        .handler = clear_post,
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &app_js));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &health));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &screen));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &upload));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &clear));

    return ESP_OK;
}
