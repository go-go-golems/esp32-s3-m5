#include "net_serve.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "app_owner.h"
#include "net_wifi.h"

namespace pulp {
namespace {

const char *kTag = "serve";

constexpr TickType_t kHandoffTimeout = pdMS_TO_TICKS(5000);

struct RouteEntry {
    bool in_use = false;
    char path[kServeMaxPath] = {};
    int32_t cb_id = 0;
};

// The single request/response slot. `busy` is the httpd-side claim;
// `gen` invalidates late owner writes after an httpd-side timeout.
struct RequestSlot {
    std::atomic<bool> busy{false};
    std::atomic<int32_t> gen{0};
    char uri[128];
    char query[kServeMaxQuery];
    // Response (owner writes during dispatch, httpd reads after the give).
    bool resp_filled;
    int32_t resp_status;
    uint8_t resp_type;  // 0 text, 1 json, 2 html
    char resp_body[kServeMaxBody];
    uint32_t resp_len;
};

struct ServeState {
    httpd_handle_t server = nullptr;
    uint16_t port = 0;
    RouteEntry routes[kServeMaxRoutes];
    char static_dir[64] = {};
    bool static_mounted = false;
    RequestSlot slot;
    SemaphoreHandle_t resp_sem = nullptr;
    // Owner-side dispatch context (which generation may respond).
    int32_t dispatch_gen = -1;
    // Counters (owner + httpd; approximate is fine for diagnostics).
    uint32_t requests = 0;
    uint32_t busy_503 = 0;
    uint32_t timeout_503 = 0;
};

ServeState s_state;

const char *ContentTypeName(uint8_t type) {
    switch (type) {
        case 1: return "application/json";
        case 2: return "text/html";
    }
    return "text/plain";
}

const char *MimeFor(const char *path) {
    const char *dot = strrchr(path, '.');
    if (dot == nullptr) {
        return "application/octet-stream";
    }
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
        return "text/html";
    }
    if (strcmp(dot, ".txt") == 0) return "text/plain";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) {
        return "image/jpeg";
    }
    return "application/octet-stream";
}

// IDF's httpd_resp_send_err has no 503 variant; hand-rolled.
esp_err_t Send503(httpd_req_t *req, const char *why) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, why, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

int FindRoute(const char *uri) {
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        if (s_state.routes[i].in_use &&
            strcmp(s_state.routes[i].path, uri) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// HTTPD TASK: streams a file from the static mount. The one sanctioned
// off-owner storage access (plain VFS reads).
esp_err_t ServeStatic(httpd_req_t *req, const char *uri) {
    if (strstr(uri, "..") != nullptr) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no");
        return ESP_OK;
    }
    char path[160];
    snprintf(path, sizeof(path), "%s%s%s", s_state.static_dir, uri,
             uri[strlen(uri) - 1] == '/' ? "index.html" : "");
    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    FILE *f = fopen(path, "rb");
    if (f == nullptr) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    httpd_resp_set_type(req, MimeFor(path));
    static char chunk[1024];  // httpd is single-worker: one buffer is safe
    size_t got;
    while ((got = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (httpd_resp_send_chunk(req, chunk, got) != ESP_OK) {
            break;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, nullptr, 0);
    return ESP_OK;
}

// HTTPD TASK: the JS-route handoff.
esp_err_t ServeJsRoute(httpd_req_t *req, int route_index) {
    RequestSlot &slot = s_state.slot;
    bool expected = false;
    if (!slot.busy.compare_exchange_strong(expected, true)) {
        s_state.busy_503++;
        return Send503(req, "busy");
    }
    snprintf(slot.uri, sizeof(slot.uri), "%.127s", req->uri);
    slot.query[0] = '\0';
    (void)httpd_req_get_url_query_str(req, slot.query,
                                      sizeof(slot.query));
    slot.resp_filled = false;
    slot.resp_status = 500;
    slot.resp_type = 0;
    slot.resp_len = 0;
    const int32_t gen = slot.gen.fetch_add(1) + 1;
    // Drain a stale give (an earlier owner response that lost its race
    // against our timeout) so the take below waits for THIS request.
    (void)xSemaphoreTake(s_state.resp_sem, 0);
    if (PostModuleDone(ModuleId::Serve, kDoneServeRequest, route_index,
                       gen) != StatusCode::Ok) {
        slot.busy.store(false);
        return Send503(req, "queue");
    }
    if (xSemaphoreTake(s_state.resp_sem, kHandoffTimeout) != pdTRUE) {
        // Owner wedged: invalidate the generation so a late ServeRespond
        // is dropped, then fail this request.
        slot.gen.fetch_add(1);
        s_state.timeout_503++;
        slot.busy.store(false);
        return Send503(req, "owner timeout");
    }
    if (!slot.resp_filled) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "route returned nothing");
        slot.busy.store(false);
        return ESP_OK;
    }
    char status_line[16];
    snprintf(status_line, sizeof(status_line), "%d",
             static_cast<int>(slot.resp_status));
    httpd_resp_set_status(req, status_line);
    httpd_resp_set_type(req, ContentTypeName(slot.resp_type));
    httpd_resp_send(req, slot.resp_body, slot.resp_len);
    slot.busy.store(false);
    return ESP_OK;
}

esp_err_t Handler(httpd_req_t *req) {
    // HTTPD TASK entry point for every GET.
    s_state.requests++;
    char uri[128];
    snprintf(uri, sizeof(uri), "%.127s", req->uri);
    char *qmark = strchr(uri, '?');
    if (qmark != nullptr) {
        *qmark = '\0';
    }
    const int route = FindRoute(uri);
    if (route >= 0) {
        return ServeJsRoute(req, route);
    }
    if (s_state.static_mounted) {
        return ServeStatic(req, uri);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_OK;
}

const char kDefaultIndex[] =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<title>PaperS3</title></head><body style='font-family:monospace'>"
    "<h1>PULP OS</h1><p>This page is served from /sdcard/www.</p>"
    "<p><a href='/status'>/status</a> (JS route, when an app registers "
    "one)</p></body></html>\n";

}  // namespace

StatusCode ServeRouteAdd(const char *path, int32_t cb_id) {
    AssertOwner();
    if (path == nullptr || path[0] != '/' ||
        strlen(path) >= kServeMaxPath || cb_id <= 0) {
        return StatusCode::InvalidArgument;
    }
    const int existing = FindRoute(path);
    if (existing >= 0) {
        s_state.routes[existing].cb_id = cb_id;  // re-register
        return StatusCode::Ok;
    }
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        if (!s_state.routes[i].in_use) {
            snprintf(s_state.routes[i].path, sizeof(s_state.routes[i].path),
                     "%s", path);
            s_state.routes[i].cb_id = cb_id;
            s_state.routes[i].in_use = true;
            return StatusCode::Ok;
        }
    }
    return StatusCode::CapacityExceeded;
}

void ServeRoutesClear() {
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        s_state.routes[i].in_use = false;
        s_state.routes[i].cb_id = 0;
    }
}

StatusCode ServeFilesMount(const char *url_prefix, const char *dir) {
    AssertOwner();
    if (url_prefix == nullptr || strcmp(url_prefix, "/") != 0 ||
        dir == nullptr || strlen(dir) >= sizeof(s_state.static_dir)) {
        return StatusCode::InvalidArgument;
    }
    snprintf(s_state.static_dir, sizeof(s_state.static_dir), "%s", dir);
    mkdir(dir, 0775);
    char index_path[96];
    snprintf(index_path, sizeof(index_path), "%s/index.html", dir);
    struct stat st;
    if (stat(index_path, &st) != 0) {
        FILE *f = fopen(index_path, "wb");
        if (f != nullptr) {
            fwrite(kDefaultIndex, 1, sizeof(kDefaultIndex) - 1, f);
            fclose(f);
            ESP_LOGI(kTag, "wrote default %s", index_path);
        }
    }
    s_state.static_mounted = true;
    return StatusCode::Ok;
}

StatusCode ServeStart(uint16_t port) {
    AssertOwner();
    if (s_state.server != nullptr) {
        return StatusCode::Busy;
    }
    if (s_state.resp_sem == nullptr) {
        s_state.resp_sem = xSemaphoreCreateBinary();
        if (s_state.resp_sem == nullptr) {
            return StatusCode::OutOfMemory;
        }
    }
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = port == 0 ? 80 : port;
    cfg.max_uri_handlers = 1;
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.lru_purge_enable = true;
    if (httpd_start(&s_state.server, &cfg) != ESP_OK) {
        s_state.server = nullptr;
        return StatusCode::Busy;
    }
    static const httpd_uri_t all_get = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = &Handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_state.server, &all_get);
    s_state.port = cfg.server_port;
    ESP_LOGI(kTag, "listening on :%u",
             static_cast<unsigned>(s_state.port));
    return StatusCode::Ok;
}

StatusCode ServeStop() {
    AssertOwner();
    if (s_state.server == nullptr) {
        return StatusCode::Ok;
    }
    httpd_stop(s_state.server);
    s_state.server = nullptr;
    s_state.port = 0;
    ESP_LOGI(kTag, "stopped");
    return StatusCode::Ok;
}

bool ServeRunning() { return s_state.server != nullptr; }

void ServeUrl(char *out, size_t cap) {
    char ip[16];
    WifiIp(ip, sizeof(ip));
    if (!ServeRunning() || ip[0] == '\0') {
        snprintf(out, cap, "%s", "");
        return;
    }
    if (s_state.port == 80) {
        snprintf(out, cap, "http://%s", ip);
    } else {
        snprintf(out, cap, "http://%s:%u", ip,
                 static_cast<unsigned>(s_state.port));
    }
}

void ServeOwnerDispatch(int32_t route_index, int32_t gen) {
    AssertOwner();
    if (route_index < 0 ||
        route_index >= static_cast<int32_t>(kServeMaxRoutes) ||
        !s_state.routes[route_index].in_use) {
        // Route vanished (resetTree between claim and dispatch): release
        // the waiter with an unfilled slot -> it answers 500.
        if (s_state.slot.gen.load() == gen) {
            xSemaphoreGive(s_state.resp_sem);
        }
        return;
    }
    s_state.dispatch_gen = gen;
    extern void JsServeInvokeRoute(int32_t cb_id);  // js_serve.cpp
    JsServeInvokeRoute(s_state.routes[route_index].cb_id);
    s_state.dispatch_gen = -1;
    if (s_state.slot.gen.load() == gen) {
        xSemaphoreGive(s_state.resp_sem);
    }
    // else: httpd timed out and moved on; the give is skipped so the
    // semaphore cannot carry a stale count into the next request.
}

bool ServeRespond(int32_t status, uint8_t content_type, const char *body,
                  uint32_t len) {
    AssertOwner();
    RequestSlot &slot = s_state.slot;
    if (s_state.dispatch_gen < 0 ||
        slot.gen.load() != s_state.dispatch_gen) {
        return false;  // late write after httpd timeout: dropped
    }
    if (len > kServeMaxBody) {
        len = kServeMaxBody;
    }
    std::memcpy(slot.resp_body, body, len);
    slot.resp_len = len;
    slot.resp_status = status;
    slot.resp_type = content_type;
    slot.resp_filled = true;
    return true;
}

const char *ServeRequestQuery() { return s_state.slot.query; }

const char *ServeRequestUri() { return s_state.slot.uri; }

void FillServeSnapshot(ServeSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->running = ServeRunning() ? 1 : 0;
    out->port = s_state.port;
    uint32_t routes = 0;
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        routes += s_state.routes[i].in_use ? 1 : 0;
    }
    out->routes = routes;
    out->static_mounted = s_state.static_mounted ? 1 : 0;
    out->requests = s_state.requests;
    out->busy_503 = s_state.busy_503;
    out->timeout_503 = s_state.timeout_503;
    ServeUrl(out->url, sizeof(out->url));
}

}  // namespace pulp
