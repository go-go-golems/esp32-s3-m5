/*
 * web_server.c — Minimal HTTP server for SToMS3R.
 *
 * Serves the embedded index.html at / and provides:
 *   POST /api/print/text    — { "text": "..." }
 *   POST /api/print/bitmap  — raw 1-bit bitmap body (width x height in headers)
 *   GET  /api/status        — JSON: wifi, printer state
 */

#include "web_server.h"

#include <string.h>
#include "driver/uart.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "printer_drv.h"
#include "wifi_mgr.h"

static const char *TAG = "web_server";
static httpd_handle_t s_server = NULL;

/* ---- Embedded HTML --------------------------------------------------- */

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

/* ---- Helper: read full body ------------------------------------------ */

static char *read_body(httpd_req_t *req, size_t *out_len)
{
    size_t len = (size_t)req->content_len;
    if (len == 0 || len > 256 * 1024) return NULL;

    char *buf = malloc(len + 1);
    if (!buf) return NULL;

    size_t off = 0;
    while (off < len) {
        int n = httpd_req_recv(req, buf + off, len - off);
        if (n <= 0) { free(buf); return NULL; }
        off += (size_t)n;
    }
    buf[off] = '\0';
    if (out_len) *out_len = off;
    return buf;
}

/* ---- Helper: send JSON ----------------------------------------------- */

static void send_json_ok(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
}

static void send_json_error(httpd_req_t *req, const char *msg)
{
    httpd_resp_set_type(req, "application/json");
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"ok\":false,\"error\":\"%s\"}", msg);
    httpd_resp_sendstr(req, buf);
}

/* ---- GET / — serve index.html ---------------------------------------- */

static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    size_t len = (size_t)(index_html_end - index_html_start);
    return httpd_resp_send(req, (const char *)index_html_start, len);
}

/* ---- GET /api/status ------------------------------------------------- */

static esp_err_t api_status_get(httpd_req_t *req)
{
    char ip[16] = "disconnected";
    if (wifi_mgr_is_connected()) {
        wifi_mgr_get_ip(ip, sizeof(ip));
    }

    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"ok\":true,\"wifi\":{\"connected\":%s,\"ip\":\"%s\"},"
             "\"printer\":{\"baud\":%d,\"swapped\":%s}}",
             wifi_mgr_is_connected() ? "true" : "false",
             ip,
             printer_drv_get_baud(),
             printer_drv_is_swapped() ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

/* ---- POST /api/print/text -------------------------------------------- */

static esp_err_t api_print_text_post(httpd_req_t *req)
{
    size_t body_len = 0;
    char *body = read_body(req, &body_len);
    if (!body) {
        send_json_error(req, "empty or too large body");
        return ESP_FAIL;
    }

    /* Find "text":"..." in JSON */
    char *key = strstr(body, "\"text\"");
    if (!key) {
        free(body);
        send_json_error(req, "missing \"text\" field");
        return ESP_FAIL;
    }
    /* key points to "text":"value"} — skip past key to the colon */
    char *colon = strchr(key + 6, ':');
    if (!colon) { free(body); send_json_error(req, "bad JSON: no colon"); return ESP_FAIL; }
    /* Find opening quote of the value */
    char *text_start = strchr(colon, '"');
    if (!text_start) { free(body); send_json_error(req, "bad JSON: no value"); return ESP_FAIL; }
    text_start++; /* skip the opening quote */
    char *text_end = strchr(text_start, '"');
    if (!text_end) { free(body); send_json_error(req, "bad JSON: no closing quote"); return ESP_FAIL; }

    /* Null-terminate the text */
    *text_end = '\0';

    esp_err_t err = printer_drv_print_text(text_start);
    free(body);

    if (err != ESP_OK) {
        send_json_error(req, esp_err_to_name(err));
        return ESP_FAIL;
    }

    send_json_ok(req);
    return ESP_OK;
}

/* ---- POST /api/print/bitmap ------------------------------------------ */

static esp_err_t api_print_bitmap_post(httpd_req_t *req)
{
    /* Expect headers: X-Width (pixels) and X-Height (pixels) */
    char hdr_val[16] = {0};
    uint16_t width = 0, height = 0;

    if (httpd_req_get_hdr_value_str(req, "X-Width", hdr_val, sizeof(hdr_val)) == ESP_OK) {
        width = (uint16_t)atoi(hdr_val);
    }
    if (httpd_req_get_hdr_value_str(req, "X-Height", hdr_val, sizeof(hdr_val)) == ESP_OK) {
        height = (uint16_t)atoi(hdr_val);
    }

    if (width == 0 || height == 0 || (width % 8) != 0) {
        send_json_error(req, "missing or invalid X-Width/X-Height headers");
        return ESP_FAIL;
    }

    size_t expected = (size_t)(width / 8) * height;
    if (req->content_len <= 0 || (size_t)req->content_len != expected) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "content-length %ld != expected %zu",
                 (long)req->content_len, expected);
        send_json_error(req, msg);
        return ESP_FAIL;
    }

    /* Read the entire body into memory first.  Network receive gaps must not
     * occur inside a raster command's pixel payload.  The printer driver then
     * sends one complete GS v 0 raster command while UART CTS flow control lets
     * the printer pause TX when its input buffer or thermal engine is busy. */
    size_t body_len = 0;
    char *body = read_body(req, &body_len);
    if (!body || body_len != expected) {
        free(body);
        send_json_error(req, "failed to read full bitmap body");
        return ESP_FAIL;
    }

    esp_err_t err = printer_drv_print_bitmap(width, height,
                                             (const uint8_t *)body);
    free(body);

    if (err != ESP_OK) {
        send_json_error(req, "bitmap print failed");
        return ESP_FAIL;
    }

    send_json_ok(req);
    return ESP_OK;
}

/* ---- Server start/stop ----------------------------------------------- */

static const httpd_uri_t uri_root = {
    .uri = "/", .method = HTTP_GET, .handler = root_get,
};
static const httpd_uri_t uri_status = {
    .uri = "/api/status", .method = HTTP_GET, .handler = api_status_get,
};
static const httpd_uri_t uri_print_text = {
    .uri = "/api/print/text", .method = HTTP_POST, .handler = api_print_text_post,
};
static const httpd_uri_t uri_print_bitmap = {
    .uri = "/api/print/bitmap", .method = HTTP_POST, .handler = api_print_bitmap_post,
};

esp_err_t web_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 8;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_register_uri_handler(s_server, &uri_root);
    httpd_register_uri_handler(s_server, &uri_status);
    httpd_register_uri_handler(s_server, &uri_print_text);
    httpd_register_uri_handler(s_server, &uri_print_bitmap);

    ESP_LOGI(TAG, "HTTP server started on port 80");
    return ESP_OK;
}

void web_server_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}
