/*
 * HTTP API + WebSocket event stream for tutorial 0029.
 *
 * Design goal: HTTP handlers only parse/validate and post HUB_CMD_* events into the bus.
 * The bus handlers update state and emit notifications.
 */

#include "hub_http.h"

#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "cJSON.h"

#include "hub_bus.h"
#include "hub_registry.h"
#include "hub_types.h"

static const char *TAG = "hub_http_0029";

static httpd_handle_t s_server = NULL;

static esp_err_t device_set_post(httpd_req_t *req);
static esp_err_t device_interview_post(httpd_req_t *req);
static esp_err_t scene_trigger_post(httpd_req_t *req);

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB
static SemaphoreHandle_t s_ws_mu = NULL;
static int s_ws_clients[8];
static size_t s_ws_clients_n = 0;
static volatile size_t s_ws_clients_n_cached = 0;
#endif

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB
static void ws_client_add(int fd) {
    if (!s_ws_mu) return;
    xSemaphoreTake(s_ws_mu, portMAX_DELAY);
    for (size_t i = 0; i < s_ws_clients_n; i++) {
        if (s_ws_clients[i] == fd) {
            xSemaphoreGive(s_ws_mu);
            return;
        }
    }
    if (s_ws_clients_n < (sizeof(s_ws_clients) / sizeof(s_ws_clients[0]))) {
        s_ws_clients[s_ws_clients_n++] = fd;
    }
    s_ws_clients_n_cached = s_ws_clients_n;
    xSemaphoreGive(s_ws_mu);
}

static void ws_client_remove(int fd) {
    if (!s_ws_mu) return;
    xSemaphoreTake(s_ws_mu, portMAX_DELAY);
    for (size_t i = 0; i < s_ws_clients_n; i++) {
        if (s_ws_clients[i] == fd) {
            s_ws_clients[i] = s_ws_clients[s_ws_clients_n - 1];
            s_ws_clients_n--;
            break;
        }
    }
    s_ws_clients_n_cached = s_ws_clients_n;
    xSemaphoreGive(s_ws_mu);
}

static size_t ws_clients_snapshot(int *out, size_t max_out) {
    if (!out || max_out == 0 || !s_ws_mu) return 0;
    xSemaphoreTake(s_ws_mu, portMAX_DELAY);
    const size_t n = (s_ws_clients_n < max_out) ? s_ws_clients_n : max_out;
    for (size_t i = 0; i < n; i++) {
        out[i] = s_ws_clients[i];
    }
    xSemaphoreGive(s_ws_mu);
    return n;
}
#endif

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB
typedef struct {
    atomic_int refcnt;
    size_t len;
    uint8_t data[];
} ws_shared_buf_t;

static void ws_send_free_cb(esp_err_t err, int fd, void *arg) {
    (void)err;
    (void)fd;
    ws_shared_buf_t *b = (ws_shared_buf_t *)arg;
    if (!b) return;
    if (atomic_fetch_sub(&b->refcnt, 1) == 1) {
        free(b);
    }
}
#endif

size_t hub_http_events_client_count(void) {
    if (!s_server) return 0;
#if !CONFIG_TUTORIAL_0029_ENABLE_WS_PB
    return 0;
#else
    return s_ws_clients_n_cached;
#endif
}

esp_err_t hub_http_events_broadcast_pb(const uint8_t *data, size_t len) {
    if (!s_server) return ESP_ERR_INVALID_STATE;
    if (!data || len == 0) return ESP_OK;

#if !CONFIG_TUTORIAL_0029_ENABLE_WS_PB
    (void)data;
    (void)len;
    return ESP_OK; // WS stream disabled
#else
    if (hub_http_events_client_count() == 0) return ESP_OK;

    int fds[8];
    const size_t n = ws_clients_snapshot(fds, sizeof(fds) / sizeof(fds[0]));
    if (n == 0) return ESP_OK;

    ws_shared_buf_t *b = (ws_shared_buf_t *)malloc(sizeof(*b) + len);
    if (!b) return ESP_ERR_NO_MEM;
    memcpy(b->data, data, len);
    b->len = len;
    atomic_init(&b->refcnt, (int)n);

    for (size_t i = 0; i < n; i++) {
        const int fd = fds[i];
        const httpd_ws_client_info_t info = httpd_ws_get_fd_info(s_server, fd);
        if (info == HTTPD_WS_CLIENT_INVALID) {
            ws_client_remove(fd);
            if (atomic_fetch_sub(&b->refcnt, 1) == 1) free(b);
            continue;
        }
        if (info != HTTPD_WS_CLIENT_WEBSOCKET) {
            // Connection exists but isn't upgraded yet (HTTP). Keep it around; it'll become
            // a WebSocket after the handshake completes.
            if (atomic_fetch_sub(&b->refcnt, 1) == 1) free(b);
            continue;
        }

        httpd_ws_frame_t frame = {0};
        frame.type = HTTPD_WS_TYPE_BINARY;
        frame.payload = (uint8_t *)b->data;
        frame.len = len;

        esp_err_t err = httpd_ws_send_data_async(s_server, fd, &frame, ws_send_free_cb, b);
        if (err != ESP_OK) {
            ws_client_remove(fd);
            if (atomic_fetch_sub(&b->refcnt, 1) == 1) free(b);
        }
    }

    return ESP_OK;
#endif
}

static esp_err_t send_json(httpd_req_t *req, const char *json) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t read_body(httpd_req_t *req, char *buf, size_t cap) {
    if (!req || !buf || cap == 0) return ESP_ERR_INVALID_ARG;
    if (req->content_len <= 0) {
        buf[0] = '\0';
        return ESP_OK;
    }
    if ((size_t)req->content_len >= cap) {
        return ESP_ERR_NO_MEM;
    }

    int remaining = req->content_len;
    size_t off = 0;
    while (remaining > 0) {
        const int n = httpd_req_recv(req, buf + off, remaining);
        if (n <= 0) {
            return ESP_FAIL;
        }
        off += (size_t)n;
        remaining -= n;
    }
    buf[off] = '\0';
    return ESP_OK;
}

static bool parse_u32_path_param(const char *uri, const char *prefix, const char *suffix, uint32_t *out_id) {
    if (!uri || !prefix || !suffix || !out_id) return false;
    const size_t pfx = strlen(prefix);
    const size_t sfx = strlen(suffix);
    const size_t ulen = strlen(uri);
    if (ulen < pfx + sfx + 1) return false;
    if (strncmp(uri, prefix, pfx) != 0) return false;
    if (strcmp(uri + (ulen - sfx), suffix) != 0) return false;

    const char *p = uri + pfx;
    char numbuf[16] = {0};
    const size_t nlen = ulen - pfx - sfx;
    if (nlen == 0 || nlen >= sizeof(numbuf)) return false;
    memcpy(numbuf, p, nlen);
    numbuf[nlen] = '\0';

    char *end = NULL;
    unsigned long v = strtoul(numbuf, &end, 10);
    if (!end || *end != '\0') return false;
    *out_id = (uint32_t)v;
    return true;
}

static hub_device_type_t parse_type(const char *s) {
    if (!s) return 0;
    if (strcmp(s, "plug") == 0) return HUB_DEVICE_PLUG;
    if (strcmp(s, "bulb") == 0) return HUB_DEVICE_BULB;
    if (strcmp(s, "temp_sensor") == 0) return HUB_DEVICE_TEMP_SENSOR;
    return 0;
}

static uint32_t parse_caps(const cJSON *caps) {
    if (!caps || !cJSON_IsArray(caps)) {
        return 0;
    }
    uint32_t mask = 0;
    cJSON *it = NULL;
    cJSON_ArrayForEach(it, caps) {
        if (!cJSON_IsString(it) || !it->valuestring) continue;
        const char *s = it->valuestring;
        if (strcmp(s, "onoff") == 0) mask |= HUB_CAP_ONOFF;
        else if (strcmp(s, "level") == 0) mask |= HUB_CAP_LEVEL;
        else if (strcmp(s, "power") == 0) mask |= HUB_CAP_POWER;
        else if (strcmp(s, "temperature") == 0) mask |= HUB_CAP_TEMPERATURE;
    }
    return mask;
}

static esp_err_t health_get(httpd_req_t *req) {
    char buf[160];
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"uptime_ms\":%" PRIu32 "}", (uint32_t)(esp_timer_get_time() / 1000));
    return send_json(req, buf);
}

static esp_err_t index_get(httpd_req_t *req) {
    static const char *body =
        "mock_zigbee_http_hub\n"
        "\n"
        "Endpoints:\n"
        "  GET  /v1/health\n"
        "  GET  /v1/devices\n"
        "  POST /v1/devices\n"
        "  GET  /v1/devices/{id}\n"
        "  POST /v1/devices/{id}/set\n"
        "  POST /v1/devices/{id}/interview\n"
        "  POST /v1/scenes/{id}/trigger\n"
        "  WS   /v1/events/ws  (protobuf binary frames)\n";

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static cJSON *device_to_json(const hub_device_t *d) {
    cJSON *o = cJSON_CreateObject();
    cJSON_AddNumberToObject(o, "id", (double)d->id);
    const char *type = "unknown";
    if (d->type == HUB_DEVICE_PLUG) type = "plug";
    else if (d->type == HUB_DEVICE_BULB) type = "bulb";
    else if (d->type == HUB_DEVICE_TEMP_SENSOR) type = "temp_sensor";
    cJSON_AddStringToObject(o, "type", type);
    cJSON_AddStringToObject(o, "name", d->name);

    cJSON *caps = cJSON_AddArrayToObject(o, "caps");
    if (d->caps & HUB_CAP_ONOFF) cJSON_AddItemToArray(caps, cJSON_CreateString("onoff"));
    if (d->caps & HUB_CAP_LEVEL) cJSON_AddItemToArray(caps, cJSON_CreateString("level"));
    if (d->caps & HUB_CAP_POWER) cJSON_AddItemToArray(caps, cJSON_CreateString("power"));
    if (d->caps & HUB_CAP_TEMPERATURE) cJSON_AddItemToArray(caps, cJSON_CreateString("temperature"));

    cJSON *state = cJSON_AddObjectToObject(o, "state");
    if (d->caps & HUB_CAP_ONOFF) cJSON_AddBoolToObject(state, "on", d->on);
    if (d->caps & HUB_CAP_LEVEL) cJSON_AddNumberToObject(state, "level", (double)d->level);
    if (d->caps & HUB_CAP_POWER) cJSON_AddNumberToObject(state, "power_w", (double)d->power_w);
    if (d->caps & HUB_CAP_TEMPERATURE) cJSON_AddNumberToObject(state, "temperature_c", (double)d->temperature_c);
    return o;
}

static esp_err_t devices_list_get(httpd_req_t *req) {
    hub_device_t snap[32];
    size_t n = 0;
    (void)hub_registry_snapshot(snap, sizeof(snap) / sizeof(snap[0]), &n);

    cJSON *arr = cJSON_CreateArray();
    for (size_t i = 0; i < n; i++) {
        cJSON_AddItemToArray(arr, device_to_json(&snap[i]));
    }

    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    if (!json) {
        return send_json(req, "[]");
    }
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return err;
}

static esp_err_t devices_get(httpd_req_t *req) {
    uint32_t id = 0;
    if (!parse_u32_path_param(req->uri, "/v1/devices/", "", &id)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad id");
        return ESP_OK;
    }
    hub_device_t d = {0};
    if (hub_registry_get(id, &d) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }

    cJSON *o = device_to_json(&d);
    char *json = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    if (!json) {
        return send_json(req, "{\"ok\":false}");
    }
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return err;
}

static esp_err_t devices_post_subroute(httpd_req_t *req) {
    if (!req) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "bad request");
        return ESP_OK;
    }
    const char *uri = req->uri;
    const size_t ulen = strlen(uri);

    const char *suffix_set = "/set";
    const size_t set_len = strlen(suffix_set);

    const char *suffix_interview = "/interview";
    const size_t interview_len = strlen(suffix_interview);

    // The wildcard match function used by ESP-IDF doesn't reliably match
    // patterns like "/v1/devices/*/set", so register "/v1/devices/*" once for
    // POST and dispatch based on suffix.
    if (ulen >= set_len && strcmp(uri + (ulen - set_len), suffix_set) == 0) {
        return device_set_post(req);
    }
    if (ulen >= interview_len && strcmp(uri + (ulen - interview_len), suffix_interview) == 0) {
        return device_interview_post(req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "unknown device action");
    return ESP_OK;
}

static esp_err_t devices_post(httpd_req_t *req) {
    char body[768];
    esp_err_t err = read_body(req, body, sizeof(body));
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "body too large");
        return ESP_OK;
    }

    cJSON *root = cJSON_Parse(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
        return ESP_OK;
    }

    const cJSON *type = cJSON_GetObjectItem(root, "type");
    const cJSON *name = cJSON_GetObjectItem(root, "name");
    const cJSON *caps = cJSON_GetObjectItem(root, "caps");
    if (!cJSON_IsString(type) || !cJSON_IsString(name)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing type/name");
        return ESP_OK;
    }

    hub_device_type_t t = parse_type(type->valuestring);
    if (!t) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "unknown type");
        return ESP_OK;
    }
    const uint32_t cap_mask = parse_caps(caps);

    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_device_t));
    if (!q) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no mem");
        return ESP_OK;
    }

    hub_cmd_device_add_t cmd = {
        .hdr = {.req_id = (uint32_t)esp_timer_get_time(), .reply_q = q},
        .type = t,
        .caps = cap_mask,
        .name = {0},
    };
    strncpy(cmd.name, name->valuestring, sizeof(cmd.name) - 1);
    cJSON_Delete(root);

    esp_event_loop_handle_t loop = hub_bus_get_loop();
    err = esp_event_post_to(loop, HUB_EVT, HUB_CMD_DEVICE_ADD, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "bus busy");
        return ESP_OK;
    }

    hub_reply_device_t rep = {0};
    if (xQueueReceive(q, &rep, pdMS_TO_TICKS(250)) != pdTRUE) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "no reply");
        return ESP_OK;
    }
    vQueueDelete(q);

    if (rep.status != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "add failed");
        return ESP_OK;
    }

    cJSON *o = device_to_json(&rep.device);
    char *json = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    if (!json) {
        return send_json(req, "{\"ok\":true}");
    }
    httpd_resp_set_type(req, "application/json");
    err = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    return err;
}

static esp_err_t device_set_post(httpd_req_t *req) {
    uint32_t id = 0;
    if (!parse_u32_path_param(req->uri, "/v1/devices/", "/set", &id)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad id");
        return ESP_OK;
    }

    char body[512];
    esp_err_t err = read_body(req, body, sizeof(body));
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "body too large");
        return ESP_OK;
    }

    cJSON *root = cJSON_Parse(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
        return ESP_OK;
    }

    const cJSON *on = cJSON_GetObjectItem(root, "on");
    const cJSON *level = cJSON_GetObjectItem(root, "level");

    hub_cmd_device_set_t cmd = {0};
    cmd.hdr.req_id = (uint32_t)esp_timer_get_time();
    cmd.device_id = id;
    if (cJSON_IsBool(on)) {
        cmd.has_on = true;
        cmd.on = cJSON_IsTrue(on);
    }
    if (cJSON_IsNumber(level)) {
        cmd.has_level = true;
        cmd.level = (uint8_t)level->valuedouble;
    }
    cJSON_Delete(root);

    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_status_t));
    if (!q) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no mem");
        return ESP_OK;
    }
    cmd.hdr.reply_q = q;

    esp_event_loop_handle_t loop = hub_bus_get_loop();
    err = esp_event_post_to(loop, HUB_EVT, HUB_CMD_DEVICE_SET, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "bus busy");
        return ESP_OK;
    }

    hub_reply_status_t rep = {0};
    if (xQueueReceive(q, &rep, pdMS_TO_TICKS(250)) != pdTRUE) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "no reply");
        return ESP_OK;
    }
    vQueueDelete(q);

    if (rep.status == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    if (rep.status != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "set failed");
        return ESP_OK;
    }

    return send_json(req, "{\"ok\":true}");
}

static esp_err_t device_interview_post(httpd_req_t *req) {
    uint32_t id = 0;
    if (!parse_u32_path_param(req->uri, "/v1/devices/", "/interview", &id)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad id");
        return ESP_OK;
    }

    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_status_t));
    if (!q) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no mem");
        return ESP_OK;
    }

    hub_cmd_device_interview_t cmd = {
        .hdr = {.req_id = (uint32_t)esp_timer_get_time(), .reply_q = q},
        .device_id = id,
    };

    esp_event_loop_handle_t loop = hub_bus_get_loop();
    esp_err_t err = esp_event_post_to(loop, HUB_EVT, HUB_CMD_DEVICE_INTERVIEW, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "bus busy");
        return ESP_OK;
    }

    hub_reply_status_t rep = {0};
    if (xQueueReceive(q, &rep, pdMS_TO_TICKS(250)) != pdTRUE) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "no reply");
        return ESP_OK;
    }
    vQueueDelete(q);

    if (rep.status == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    if (rep.status != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "interview failed");
        return ESP_OK;
    }
    return send_json(req, "{\"ok\":true}");
}

static esp_err_t scenes_post_subroute(httpd_req_t *req) {
    if (!req) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "bad request");
        return ESP_OK;
    }

    const char *uri = req->uri;
    const char *suffix = "/trigger";
    const size_t ulen = strlen(uri);
    const size_t sfx = strlen(suffix);
    if (ulen >= sfx && strcmp(uri + (ulen - sfx), suffix) == 0) {
        return scene_trigger_post(req);
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Nothing matches the given URI");
    return ESP_OK;
}

static esp_err_t scene_trigger_post(httpd_req_t *req) {
    uint32_t id = 0;
    if (!parse_u32_path_param(req->uri, "/v1/scenes/", "/trigger", &id)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad scene id");
        return ESP_OK;
    }

    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_status_t));
    if (!q) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no mem");
        return ESP_OK;
    }

    hub_cmd_scene_trigger_t cmd = {
        .hdr = {.req_id = (uint32_t)esp_timer_get_time(), .reply_q = q},
        .scene_id = id,
    };

    esp_event_loop_handle_t loop = hub_bus_get_loop();
    esp_err_t err = esp_event_post_to(loop, HUB_EVT, HUB_CMD_SCENE_TRIGGER, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "bus busy");
        return ESP_OK;
    }

    hub_reply_status_t rep = {0};
    if (xQueueReceive(q, &rep, pdMS_TO_TICKS(500)) != pdTRUE) {
        vQueueDelete(q);
        httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "no reply");
        return ESP_OK;
    }
    vQueueDelete(q);

    if (rep.status == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "unknown scene");
        return ESP_OK;
    }
    if (rep.status != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "scene failed");
        return ESP_OK;
    }
    return send_json(req, "{\"ok\":true}");
}

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB
static esp_err_t events_ws_handler(httpd_req_t *req) {
    const int fd = httpd_req_to_sockfd(req);
    if (req->method == HTTP_GET) {
        ws_client_add(fd);
        ESP_LOGI(TAG, "ws connected fd=%d", fd);
        return ESP_OK;
    }

    // Called for websocket DATA frames (and optionally control frames, depending on registration flags).
    // We keep this endpoint read-only, but we still drain incoming frames so well-behaved clients
    // (that send pings or other frames) don't back up the socket receive buffer.
    ws_client_add(fd);

    httpd_ws_frame_t frame = {0};
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
    if (err != ESP_OK) {
        if (err == ESP_FAIL && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return ESP_OK;
        }
        ws_client_remove(fd);
        return ESP_OK;
    }

    uint8_t tmp[64];
    if (frame.len > sizeof(tmp)) {
        ESP_LOGW(TAG, "ws frame too large (fd=%d len=%u)", fd, (unsigned)frame.len);
        ws_client_remove(fd);
        return ESP_OK;
    }
    frame.payload = tmp;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) {
        if (err == ESP_FAIL && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return ESP_OK;
        }
        ws_client_remove(fd);
        return ESP_OK;
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "ws closed fd=%d", fd);
        ws_client_remove(fd);
        return ESP_OK;
    }

    // MVP: ignore incoming frames (read-only stream).
    return ESP_OK;
}
#endif

esp_err_t hub_http_start(void) {
    if (s_server) {
        return ESP_OK;
    }

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB
    if (!s_ws_mu) {
        s_ws_mu = xSemaphoreCreateMutex();
    }
    if (!s_ws_mu) {
        return ESP_ERR_NO_MEM;
    }
    s_ws_clients_n = 0;
    s_ws_clients_n_cached = 0;
#endif

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    // cJSON printing can be stack-hungry; keep some headroom to avoid
    // hard-to-debug crashes in the httpd task.
    cfg.stack_size = 8192;

    ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        s_server = NULL;
        return err;
    }

    httpd_uri_t health = {.uri = "/v1/health", .method = HTTP_GET, .handler = health_get, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &health);

    httpd_uri_t index = {.uri = "/", .method = HTTP_GET, .handler = index_get, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &index);

    httpd_uri_t devices_list = {.uri = "/v1/devices", .method = HTTP_GET, .handler = devices_list_get, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &devices_list);

    httpd_uri_t devices_post_u = {.uri = "/v1/devices", .method = HTTP_POST, .handler = devices_post, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &devices_post_u);

    httpd_uri_t device_post_u = {.uri = "/v1/devices/*", .method = HTTP_POST, .handler = devices_post_subroute, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &device_post_u);

    httpd_uri_t device_get_u = {.uri = "/v1/devices/*", .method = HTTP_GET, .handler = devices_get, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &device_get_u);

    httpd_uri_t scene_u = {.uri = "/v1/scenes/*", .method = HTTP_POST, .handler = scenes_post_subroute, .user_ctx = NULL};
    httpd_register_uri_handler(s_server, &scene_u);

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB
    httpd_uri_t ws = {
        .uri = "/v1/events/ws",
        .method = HTTP_GET,
        .handler = events_ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        // Keep this endpoint read-only; let the HTTP server handle control frames internally.
        // Passing control frames to the handler can trigger noisy recv errors/timeouts when the
        // client is idle, depending on socket timing.
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL,
    };
    httpd_register_uri_handler(s_server, &ws);
#endif

    return ESP_OK;
}

void hub_http_stop(void) {
    if (!s_server) {
        return;
    }
    httpd_stop(s_server);
    s_server = NULL;
}
