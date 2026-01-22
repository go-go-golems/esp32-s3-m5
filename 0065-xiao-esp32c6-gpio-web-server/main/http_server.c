/*
 * MO-065 / 0065: Minimal HTTP server (esp_http_server) serving embedded UI
 * and a small GPIO control API.
 *
 * Patterns reused from:
 * - 0045-xiao-esp32c6-preact-web (embedded assets + /api/status)
 * - 0046-xiao-esp32c6-led-patterns-webui (JSON request parsing with cJSON)
 */

#include "http_server.h"

#include <stdbool.h>
#include <string.h>

#include "sdkconfig.h"

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "httpd_assets_embed.h"

#include "gpio_ctl.h"

static const char *TAG = "mo065_http";

static httpd_handle_t s_server = NULL;

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

static esp_err_t send_gpio_state(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    (void)httpd_resp_set_hdr(req, "cache-control", "no-store");

    bool d2 = false;
    bool d3 = false;
    gpio_ctl_get(&d2, &d3);

    cJSON *root = cJSON_CreateObject();
    if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");

    (void)cJSON_AddBoolToObject(root, "ok", true);
    (void)cJSON_AddBoolToObject(root, "d2", d2);
    (void)cJSON_AddBoolToObject(root, "d3", d3);
    (void)cJSON_AddNumberToObject(root, "d2_gpio", (double)gpio_ctl_d2_gpio_num());
    (void)cJSON_AddNumberToObject(root, "d3_gpio", (double)gpio_ctl_d3_gpio_num());
    (void)cJSON_AddBoolToObject(root, "d2_active_low", gpio_ctl_d2_active_low());
    (void)cJSON_AddBoolToObject(root, "d3_active_low", gpio_ctl_d3_active_low());

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

static esp_err_t gpio_get(httpd_req_t *req)
{
    return send_gpio_state(req);
}

static bool json_item_to_bool(const cJSON *it, bool *out)
{
    if (!it || !out) return false;
    if (cJSON_IsBool(it)) {
        *out = cJSON_IsTrue(it);
        return true;
    }
    if (cJSON_IsNumber(it)) {
        *out = (it->valuedouble != 0);
        return true;
    }
    return false;
}

static esp_err_t gpio_post(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    (void)httpd_resp_set_hdr(req, "cache-control", "no-store");

    char buf[256];
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

    bool has_d2 = false;
    bool has_d3 = false;
    bool d2 = false;
    bool d3 = false;

    const cJSON *it_d2 = cJSON_GetObjectItemCaseSensitive(root, "d2");
    const cJSON *it_d3 = cJSON_GetObjectItemCaseSensitive(root, "d3");

    if (it_d2) {
        if (!json_item_to_bool(it_d2, &d2)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad d2");
        }
        has_d2 = true;
    }
    if (it_d3) {
        if (!json_item_to_bool(it_d3, &d3)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad d3");
        }
        has_d3 = true;
    }

    cJSON_Delete(root);

    if (!has_d2 && !has_d3) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing d2/d3");
    }

    (void)gpio_ctl_set(has_d2, d2, has_d3, d3);
    return send_gpio_state(req);
}

static esp_err_t gpio_d2_toggle_post(httpd_req_t *req)
{
    (void)gpio_ctl_toggle_d2();
    return send_gpio_state(req);
}

static esp_err_t gpio_d3_toggle_post(httpd_req_t *req)
{
    (void)gpio_ctl_toggle_d3();
    return send_gpio_state(req);
}

esp_err_t http_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = NULL;
        return err;
    }

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &root);

    const httpd_uri_t gpio_api_get = {
        .uri = "/api/gpio",
        .method = HTTP_GET,
        .handler = gpio_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &gpio_api_get);

    const httpd_uri_t gpio_api_post = {
        .uri = "/api/gpio",
        .method = HTTP_POST,
        .handler = gpio_post,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &gpio_api_post);

    const httpd_uri_t d2_toggle = {
        .uri = "/api/gpio/d2/toggle",
        .method = HTTP_POST,
        .handler = gpio_d2_toggle_post,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &d2_toggle);

    const httpd_uri_t d3_toggle = {
        .uri = "/api/gpio/d3/toggle",
        .method = HTTP_POST,
        .handler = gpio_d3_toggle_post,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &d3_toggle);

    return ESP_OK;
}

