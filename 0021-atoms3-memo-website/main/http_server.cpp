/*
 * Ticket CLINTS-MEMO-WEBSITE: minimal esp_http_server baseline.
 *
 * - Serves embedded index.html at GET /
 * - Exposes MVP status endpoint at GET /api/v1/status
 * - Recording endpoints:
 *   - POST /api/v1/recordings/start
 *   - POST /api/v1/recordings/stop
 *   - GET  /api/v1/recordings
 *   - GET  /api/v1/recordings/<name>
 *   - DELETE /api/v1/recordings/<name>
 * - Waveform endpoint:
 *   - GET /api/v1/waveform
 */

#include "http_server.h"

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "esp_http_server.h"
#include "esp_log.h"

#include "recorder.h"
#include "storage_spiffs.h"

static const char *TAG = "atoms3_memo_website_0021";

static httpd_handle_t s_server = nullptr;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");

static bool is_safe_basename(const std::string &name) {
    if (name.empty()) return false;
    for (const char c : name) {
        const bool ok =
            (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' ||
            c == '.';
        if (!ok) return false;
    }
    if (name.find("..") != std::string::npos) return false;
    return true;
}

static void json_send_error(httpd_req_t *req, int code, const char *msg) {
    httpd_resp_set_status(req, (code == 400)   ? "400 Bad Request"
                              : (code == 404) ? "404 Not Found"
                              : (code == 409) ? "409 Conflict"
                                              : "500 Internal Server Error");
    httpd_resp_set_type(req, "application/json");
    const std::string json = std::string("{\"ok\":false,\"error\":\"") + msg + "\"}";
    (void)httpd_resp_send(req, json.c_str(), (ssize_t)json.size());
}

static esp_err_t root_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    return httpd_resp_send(req, (const char *)assets_index_html_start, (ssize_t)len);
}

static esp_err_t api_v1_status_get(httpd_req_t *req) {
    const RecorderStatus st = recorder_get_status();
    std::string json;
    json.reserve(256);
    json += "{";
    json += "\"ok\":true,";
    json += "\"recording\":";
    json += st.recording ? "true" : "false";
    json += ",";
    json += "\"filename\":\"";
    json += st.filename;
    json += "\",";
    json += "\"bytes_written\":";
    json += std::to_string(st.bytes_written);
    json += ",";
    json += "\"sample_rate_hz\":";
    json += std::to_string(st.sample_rate_hz);
    json += ",";
    json += "\"channels\":";
    json += std::to_string(st.channels);
    json += ",";
    json += "\"bits_per_sample\":";
    json += std::to_string(st.bits_per_sample);
    json += "}";
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), (ssize_t)json.size());
}

static esp_err_t api_v1_waveform_get(httpd_req_t *req) {
    const std::string json = recorder_get_waveform_json();
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), (ssize_t)json.size());
}

static esp_err_t api_v1_recordings_start_post(httpd_req_t *req) {
    (void)req;
    std::string filename;
    const esp_err_t err = recorder_start(filename);
    if (err == ESP_ERR_INVALID_STATE) {
        json_send_error(req, 409, "already recording");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        json_send_error(req, 500, "start failed");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "application/json");
    const std::string json = std::string("{\"ok\":true,\"filename\":\"") + filename + "\"}";
    return httpd_resp_send(req, json.c_str(), (ssize_t)json.size());
}

static esp_err_t api_v1_recordings_stop_post(httpd_req_t *req) {
    (void)req;
    std::string filename;
    const esp_err_t err = recorder_stop(filename);
    if (err == ESP_ERR_INVALID_STATE) {
        json_send_error(req, 409, "not recording");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        json_send_error(req, 500, "stop failed");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "application/json");
    const std::string json = std::string("{\"ok\":true,\"filename\":\"") + filename + "\"}";
    return httpd_resp_send(req, json.c_str(), (ssize_t)json.size());
}

static esp_err_t api_v1_recordings_list_get(httpd_req_t *req) {
    const char *dir = storage_recordings_dir();
    DIR *d = opendir(dir);
    if (!d) {
        json_send_error(req, 500, "recordings dir unavailable");
        return ESP_OK;
    }

    std::string json;
    json.reserve(1024);
    json += "{\"ok\":true,\"recordings\":[";

    bool first = true;
    for (struct dirent *e = readdir(d); e; e = readdir(d)) {
        if (e->d_type != DT_REG && e->d_type != DT_UNKNOWN) continue;
        const char *name = e->d_name;
        const size_t nlen = strlen(name);
        if (nlen < 4 || strcmp(name + (nlen - 4), ".wav") != 0) continue;

        const std::string path = std::string(dir) + "/" + name;
        struct stat st = {};
        if (stat(path.c_str(), &st) != 0) continue;

        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"";
        json += name;
        json += "\",\"size_bytes\":";
        json += std::to_string((uint32_t)st.st_size);
        json += "}";
    }
    closedir(d);

    json += "]}";
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), (ssize_t)json.size());
}

static bool uri_to_recording_name(httpd_req_t *req, std::string &out_name) {
    const char *prefix = "/api/v1/recordings/";
    const size_t prefix_len = strlen(prefix);
    if (strncmp(req->uri, prefix, prefix_len) != 0) return false;
    const char *name = req->uri + prefix_len;
    if (!name || name[0] == '\0') return false;
    out_name = name;
    return is_safe_basename(out_name);
}

static esp_err_t api_v1_recordings_download_get(httpd_req_t *req) {
    std::string name;
    if (!uri_to_recording_name(req, name)) {
        json_send_error(req, 400, "invalid filename");
        return ESP_OK;
    }

    const std::string path = std::string(storage_recordings_dir()) + "/" + name;

    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        json_send_error(req, 404, "not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "audio/wav");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    uint8_t buf[4096];
    while (true) {
        const size_t n = fread(buf, 1, sizeof(buf), fp);
        if (n > 0) {
            const esp_err_t err = httpd_resp_send_chunk(req, (const char *)buf, (ssize_t)n);
            if (err != ESP_OK) {
                fclose(fp);
                return err;
            }
        }
        if (n < sizeof(buf)) break;
    }
    fclose(fp);
    return httpd_resp_send_chunk(req, nullptr, 0);
}

static esp_err_t api_v1_recordings_delete(httpd_req_t *req) {
    std::string name;
    if (!uri_to_recording_name(req, name)) {
        json_send_error(req, 400, "invalid filename");
        return ESP_OK;
    }

    const std::string path = std::string(storage_recordings_dir()) + "/" + name;
    if (unlink(path.c_str()) != 0) {
        json_send_error(req, 404, "not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    const char *json = "{\"ok\":true}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t http_server_start(void) {
    if (s_server) return ESP_OK;

    ESP_ERROR_CHECK(recorder_init());

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    cfg.max_uri_handlers = 24;
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t root = {};
    root.uri = "/";
    root.method = HTTP_GET;
    root.handler = root_get;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &root));

    httpd_uri_t status = {};
    status.uri = "/api/v1/status";
    status.method = HTTP_GET;
    status.handler = api_v1_status_get;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &status));

    httpd_uri_t waveform = {};
    waveform.uri = "/api/v1/waveform";
    waveform.method = HTTP_GET;
    waveform.handler = api_v1_waveform_get;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &waveform));

    httpd_uri_t rec_start = {};
    rec_start.uri = "/api/v1/recordings/start";
    rec_start.method = HTTP_POST;
    rec_start.handler = api_v1_recordings_start_post;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &rec_start));

    httpd_uri_t rec_stop = {};
    rec_stop.uri = "/api/v1/recordings/stop";
    rec_stop.method = HTTP_POST;
    rec_stop.handler = api_v1_recordings_stop_post;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &rec_stop));

    httpd_uri_t rec_list = {};
    rec_list.uri = "/api/v1/recordings";
    rec_list.method = HTTP_GET;
    rec_list.handler = api_v1_recordings_list_get;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &rec_list));

    httpd_uri_t rec_get = {};
    rec_get.uri = "/api/v1/recordings/*";
    rec_get.method = HTTP_GET;
    rec_get.handler = api_v1_recordings_download_get;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &rec_get));

    httpd_uri_t rec_del = {};
    rec_del.uri = "/api/v1/recordings/*";
    rec_del.method = HTTP_DELETE;
    rec_del.handler = api_v1_recordings_delete;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &rec_del));

    return ESP_OK;
}
