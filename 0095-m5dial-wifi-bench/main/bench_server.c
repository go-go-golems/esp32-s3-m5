/*
 * bench_server.c — HTTP benchmark endpoints for WiFi/HTTP stack analysis.
 *
 * Endpoints:
 *   GET  /                — Benchmark web UI
 *   GET  /app.js          — Frontend JavaScript
 *   GET  /api/health      — Health check
 *   GET  /api/system      — System stats (heap, PSRAM, RSSI, uptime)
 *   POST /api/bench/upload   — Upload benchmark (timing + per-segment data)
 *   GET  /api/bench/download  — Download benchmark (send N bytes, time it)
 *   POST /api/bench/ping      — Round-trip ping (echo body back with timing)
 */

#include "bench_server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_wifi.h"
/* No miniz/deflate on CoreS3 — benchmark raw throughput first */

static const char *TAG = "m5dial_bench_http";
static httpd_handle_t s_server = NULL;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t assets_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t assets_app_js_end[] asm("_binary_app_js_end");

/* ---- Helpers ---- */

/* Maximum number of recv segments we track per request.
 * At ~1460 bytes per TCP segment, a 1.8 MB upload has ~1260 segments.
 * We cap at 2048 and merge tail segments if exceeded. */
#define MAX_SEGMENTS 1024

typedef struct {
    uint32_t bytes;
    int64_t  time_us;
} recv_segment_t;

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

static bool is_deflate_request(httpd_req_t *req) {
    char hdr[32] = {0};
    if (httpd_req_get_hdr_value_str(req, "Content-Encoding", hdr, sizeof(hdr)) == ESP_OK) {
        return (strcmp(hdr, "deflate") == 0);
    }
    return false;
}

/* Parse an integer query parameter. Returns default_val if missing/invalid. */
static int query_int(httpd_req_t *req, const char *key, int default_val) {
    char query[128] = {0};
    char val[32] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char key_eq[64];
        snprintf(key_eq, sizeof(key_eq), "%s=", key);
        const char *p = strstr(query, key_eq);
        if (p) {
            p += strlen(key_eq);
            int i = 0;
            while (*p && *p != '&' && i < (int)sizeof(val) - 1) {
                val[i++] = *p++;
            }
            val[i] = '\0';
            return atoi(val);
        }
    }
    return default_val;
}

static int8_t get_sta_rssi(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

/* Write a JSON chunk to the HTTP response. Used for streaming large JSON. */
static esp_err_t json_chunk(httpd_req_t *req, const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return ESP_OK;
    if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
    return httpd_resp_send_chunk(req, buf, n);
}

/* ---- Static assets ---- */

static esp_err_t root_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    if (len > 0 && assets_index_html_start[len - 1] == '\0') len--;
    return httpd_resp_send(req, (const char *)assets_index_html_start, (ssize_t)len);
}

static esp_err_t app_js_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
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

static esp_err_t system_get(httpd_req_t *req) {
    char json[512];
    const int n = snprintf(json, sizeof(json),
        "{\"ok\":true,"
        "\"free_heap\":%lu,"
        "\"free_spiram\":%lu,"
        "\"rssi\":%d,"
        "\"uptime_us\":%lld}",
        (unsigned long)esp_get_free_heap_size(),
        (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT),
        (int)get_sta_rssi(),
        (long long)esp_timer_get_time());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    if (n > 0 && n < (int)sizeof(json)) {
        return httpd_resp_send(req, json, n);
    }
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

/* ---- Upload benchmark ---- */

static esp_err_t bench_upload_post(httpd_req_t *req) {
    const int content_len = req->content_len;
    const bool deflated = is_deflate_request(req);

    if (content_len <= 0) {
        send_json_error(req, 400, "missing or invalid content-length");
        return ESP_OK;
    }

    /* Cap at 2 MB (CoreS3 has 8 MB PSRAM, less headroom than Tab5). */
    const size_t max_recv = 100 * 1024;
    if ((size_t)content_len > max_recv) {
        send_json_error(req, 413, "payload too large (max 4 MB)");
        return ESP_OK;
    }

    /* Allocate receive buffer in SPIRAM. */
    uint8_t *recv_buf = heap_caps_malloc((size_t)content_len, MALLOC_CAP_8BIT);
    if (!recv_buf) {
        send_json_error(req, 500, "SPIRAM alloc failed");
        return ESP_OK;
    }

    /* Allocate segment timing array in SPIRAM. */
    recv_segment_t *segs = heap_caps_malloc(MAX_SEGMENTS * sizeof(recv_segment_t), MALLOC_CAP_8BIT);
    if (!segs) {
        free(recv_buf);
        send_json_error(req, 500, "segment alloc failed");
        return ESP_OK;
    }

    const int64_t T0 = esp_timer_get_time();

    /* Receive body with per-segment timing. */
    const int64_t T1 = esp_timer_get_time();
    int received = 0;
    int seg_count = 0;

    while (received < content_len) {
        const int n = httpd_req_recv(req, (char *)recv_buf + received, content_len - received);
        if (n <= 0) {
            free(recv_buf);
            free(segs);
            send_json_error(req, 500, "recv failed");
            return ESP_OK;
        }
        received += n;

        if (seg_count < MAX_SEGMENTS) {
            segs[seg_count].bytes = (uint32_t)n;
            segs[seg_count].time_us = esp_timer_get_time();
            seg_count++;
        } else {
            /* Merge into last segment if we exceed the cap. */
            segs[MAX_SEGMENTS - 1].bytes += (uint32_t)n;
            segs[MAX_SEGMENTS - 1].time_us = esp_timer_get_time();
        }
    }
    const int64_t T2 = esp_timer_get_time();

    /* No deflate support on CoreS3 benchmark (raw throughput focus). */
    int64_t T3 = T2, T4 = T2;
    size_t dec_len = (size_t)received;
    if (deflated) {
        free(recv_buf);
        free(segs);
        send_json_error(req, 400, "deflate not supported on this firmware");
        return ESP_OK;
    }

    free(recv_buf);

    /* Build response as chunked JSON to avoid large buffer. */
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    const int64_t recv_us = T2 - T1;
    const int64_t decomp_us = T4 - T3;
    const double recv_kbps = (received > 0 && recv_us > 0)
        ? ((double)received * 8.0 / ((double)recv_us / 1000000.0) / 1000.0)
        : 0.0;

    json_chunk(req, "{\"ok\":true,"
        "\"payload_bytes\":%d,"
        "\"decompressed_bytes\":%lu,"
        "\"deflated\":%s,"
        "\"timing_us\":{"
        "\"recv\":%lld,"
        "\"decompress\":%lld,"
        "\"total\":%lld},"
        "\"recv_throughput_kbps\":%.1f,"
        "\"segments\":[",
        content_len, (unsigned long)dec_len,
        deflated ? "true" : "false",
        (long long)recv_us, (long long)decomp_us,
        (long long)(T4 - T0), recv_kbps);

    for (int i = 0; i < seg_count; i++) {
        if (i > 0) json_chunk(req, ",");
        json_chunk(req, "{\"b\":%u,\"t\":%lld}",
            (unsigned)segs[i].bytes, (long long)segs[i].time_us);
    }

    free(segs);

    json_chunk(req, "],"
        "\"system\":{"
        "\"free_heap\":%lu,"
        "\"free_spiram\":%lu,"
        "\"rssi\":%d}}",
        (unsigned long)esp_get_free_heap_size(),
        (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT),
        (int)get_sta_rssi());

    httpd_resp_send_chunk(req, NULL, 0);  /* end chunked response */
    return ESP_OK;
}

/* ---- Download benchmark ---- */

static esp_err_t bench_download_get(httpd_req_t *req) {
    const int size = query_int(req, "size", 102400);  /* 100 KB default for M5Dial (no PSRAM) */

    if (size <= 0 || size > 100 * 1024) {
        send_json_error(req, 400, "size must be 1..102400");
        return ESP_OK;
    }

    /* Allocate pattern buffer (max 100 KB fits in internal RAM). */
    uint8_t *buf = heap_caps_malloc((size_t)size, MALLOC_CAP_8BIT);
    if (!buf) {
        send_json_error(req, 500, "alloc failed");
        return ESP_OK;
    }
    for (int i = 0; i < size; i++) {
        buf[i] = (uint8_t)(i & 0xFF);
    }

    const int64_t T0 = esp_timer_get_time();
    esp_err_t err = httpd_resp_send(req, (const char *)buf, size);
    const int64_t T1 = esp_timer_get_time();

    free(buf);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "download send failed: %s", esp_err_to_name(err));
        return err;
    }

    const int64_t send_us = T1 - T0;
    const double send_kbps = (size > 0 && send_us > 0)
        ? ((double)size * 8.0 / ((double)send_us / 1000000.0) / 1000.0)
        : 0.0;

    ESP_LOGI(TAG, "download: %d bytes, send %lld us, %.1f kbps",
             size, (long long)send_us, send_kbps);

    return ESP_OK;
}

/* ---- Ping benchmark ---- */

static esp_err_t bench_ping_post(httpd_req_t *req) {
    const int content_len = req->content_len;
    if (content_len <= 0 || content_len > 65536) {
        send_json_error(req, 400, "ping body must be 1..65536 bytes");
        return ESP_OK;
    }

    uint8_t *buf = heap_caps_malloc((size_t)content_len, MALLOC_CAP_8BIT);
    if (!buf) {
        send_json_error(req, 500, "alloc failed");
        return ESP_OK;
    }

    int received = 0;
    while (received < content_len) {
        const int n = httpd_req_recv(req, (char *)buf + received, content_len - received);
        if (n <= 0) {
            free(buf);
            send_json_error(req, 500, "recv failed");
            return ESP_OK;
        }
        received += n;
    }

    const int64_t T0 = esp_timer_get_time();
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Ping-T0", "1");
    esp_err_t err = httpd_resp_send(req, (const char *)buf, received);
    const int64_t T1 = esp_timer_get_time();
    free(buf);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ping send failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "ping: %d bytes, server RTT %lld us", received, (long long)(T1 - T0));
    return ESP_OK;
}

/* ---- Server lifecycle ---- */

esp_err_t bench_server_start(void) {
    if (s_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    /* Large stack for tinfl decompressor (~43 KB) during upload benchmarks. */
    cfg.stack_size = 8 * 1024;   /* 8 KB — no tinfl on CoreS3 */
    cfg.max_uri_handlers = 10;
    cfg.recv_wait_timeout = 30;
    cfg.send_wait_timeout = 30;

    ESP_LOGI(TAG, "starting benchmark server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    /* Static assets */
    httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_get };
    httpd_uri_t app_js = { .uri = "/app.js", .method = HTTP_GET, .handler = app_js_get };

    /* API endpoints */
    httpd_uri_t health = { .uri = "/api/health", .method = HTTP_GET, .handler = health_get };
    httpd_uri_t system_ep = { .uri = "/api/system", .method = HTTP_GET, .handler = system_get };
    httpd_uri_t upload = { .uri = "/api/bench/upload", .method = HTTP_POST, .handler = bench_upload_post };
    httpd_uri_t download = { .uri = "/api/bench/download", .method = HTTP_GET, .handler = bench_download_get };
    httpd_uri_t ping = { .uri = "/api/bench/ping", .method = HTTP_POST, .handler = bench_ping_post };

    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &app_js));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &health));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &system_ep));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &upload));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &download));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &ping));

    ESP_LOGI(TAG, "benchmark server ready");
    return ESP_OK;
}
