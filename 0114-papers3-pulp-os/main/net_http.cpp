#include "net_http.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_owner.h"
#include "net_auth.h"

namespace pulp {
namespace {

const char *kTag = "http";

constexpr uint32_t kTimeoutMs = 10000;
constexpr int kMaxRedirects = 3;
constexpr uint32_t kWorkerStack = 6144;

struct HeaderSlot {
    char key[32];
    char value[64];
};

struct LineRef {
    uint16_t start;
    uint16_t len;
};

// Request slot: owner writes while idle, worker reads while in flight.
// The in_flight atomic is the ownership token.
struct HttpState {
    char url[kHttpMaxUrl];
    HeaderSlot headers[kHttpMaxHeaders];
    uint32_t header_count = 0;
    uint32_t limit = kHttpDefaultLimit;
    bool slot_open = false;  // Begin called, Send not yet
    bool use_bearer = false;
    char authorization[2300]{};

    std::atomic<bool> in_flight{false};
    std::atomic<bool> abort{false};

    // Response mailbox (worker writes, owner reads after ModuleDone).
    char *body = nullptr;  // PSRAM, kHttpMaxBody
    uint32_t body_len = 0;
    int32_t status = 0;
    LineRef lines[kHttpMaxLines];
    uint32_t line_count = 0;
};

HttpState s_state;

bool EnsureBody() {
    if (s_state.body != nullptr) {
        return true;
    }
    s_state.body = static_cast<char *>(
        heap_caps_malloc(kHttpMaxBody, MALLOC_CAP_SPIRAM));
    return s_state.body != nullptr;
}

void IndexLines() {
    s_state.line_count = 0;
    uint32_t start = 0;
    for (uint32_t i = 0; i <= s_state.body_len; ++i) {
        const bool at_end = (i == s_state.body_len);
        if (!at_end && s_state.body[i] != '\n') {
            continue;
        }
        if (at_end && i == start) {
            break;
        }
        if (s_state.line_count >= kHttpMaxLines) {
            break;
        }
        LineRef &ref = s_state.lines[s_state.line_count++];
        ref.start = static_cast<uint16_t>(start);
        ref.len = static_cast<uint16_t>(i - start);
        start = i + 1;
    }
}

// WORKER TASK: collects body bytes as they stream in. perform() decodes
// chunked transfers, which open/fetch_headers/read does not (zenquotes
// regression: 200 with 0 bytes captured). Returning ESP_FAIL aborts.
esp_err_t HttpEvent(esp_http_client_event_t *evt) {
    if (evt->event_id != HTTP_EVENT_ON_DATA) {
        return ESP_OK;
    }
    if (s_state.abort.load(std::memory_order_acquire)) {
        return ESP_FAIL;
    }
    const uint32_t room = s_state.limit > s_state.body_len
                              ? s_state.limit - s_state.body_len
                              : 0;
    const uint32_t take = evt->data_len > static_cast<int>(room)
                              ? room
                              : static_cast<uint32_t>(evt->data_len);
    if (take > 0) {
        std::memcpy(s_state.body + s_state.body_len, evt->data, take);
        s_state.body_len += take;
    }
    return ESP_OK;  // excess beyond the limit is drained and dropped
}

void HttpWorker(void *) {
    // WORKER TASK: module mailbox + PostModuleDone only. No JS, no arena,
    // no storage — the single most tempting place to violate the rule.
    int32_t status = 0;
    int32_t err_or_len = 0;
    esp_http_client_config_t cfg = {};
    cfg.url = s_state.url;
    cfg.timeout_ms = kTimeoutMs;
    cfg.max_redirection_count = kMaxRedirects;
    if (AuthTrustedApiUrl(s_state.url)) {
        cfg.cert_pem = PulpDemoCACert();
    } else {
        cfg.crt_bundle_attach = esp_crt_bundle_attach;
    }
    cfg.disable_auto_redirect = false;
    cfg.event_handler = &HttpEvent;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        err_or_len = -1;
    } else {
        for (uint32_t i = 0; i < s_state.header_count; ++i) {
            esp_http_client_set_header(client, s_state.headers[i].key,
                                       s_state.headers[i].value);
        }
        if (s_state.use_bearer) {
            esp_http_client_set_header(client, "Authorization",
                                       s_state.authorization);
        }
        const esp_err_t performed = esp_http_client_perform(client);
        if (performed != ESP_OK) {
            err_or_len = -static_cast<int32_t>(performed);
        } else {
            status = esp_http_client_get_status_code(client);
            err_or_len = static_cast<int32_t>(s_state.body_len);
        }
        esp_http_client_cleanup(client);
    }
    volatile char *secret = s_state.authorization;
    for (size_t i = 0; i < sizeof(s_state.authorization); ++i) secret[i] = 0;
    s_state.use_bearer = false;
    s_state.status = status;
    IndexLines();
    ESP_LOGI(kTag, "done status=%d captured=%u",
             static_cast<int>(status),
             static_cast<unsigned>(s_state.body_len));
    // Mailbox complete BEFORE the flag drop + event post.
    s_state.in_flight.store(false, std::memory_order_release);
    (void)PostModuleDone(ModuleId::Http, kDoneHttp, status, err_or_len);
    vTaskDelete(nullptr);
}

}  // namespace

StatusCode HttpBegin(const char *url) {
    AssertOwner();
    if (s_state.in_flight.load(std::memory_order_acquire)) {
        return StatusCode::Busy;
    }
    if (url == nullptr || strlen(url) >= kHttpMaxUrl ||
        (strncmp(url, "http://", 7) != 0 &&
         strncmp(url, "https://", 8) != 0)) {
        return StatusCode::InvalidArgument;
    }
    std::memset(s_state.url, 0, sizeof(s_state.url));
    snprintf(s_state.url, sizeof(s_state.url), "%s", url);
    s_state.header_count = 0;
    s_state.limit = kHttpDefaultLimit;
    s_state.use_bearer = false;
    std::memset(s_state.authorization, 0, sizeof(s_state.authorization));
    s_state.slot_open = true;
    return StatusCode::Ok;
}

StatusCode HttpHeader(const char *key, const char *value) {
    AssertOwner();
    if (!s_state.slot_open) {
        return StatusCode::InvalidArgument;
    }
    if (s_state.header_count >= kHttpMaxHeaders) {
        return StatusCode::CapacityExceeded;
    }
    HeaderSlot &slot = s_state.headers[s_state.header_count];
    if (key == nullptr || value == nullptr ||
        strlen(key) >= sizeof(slot.key) ||
        strlen(value) >= sizeof(slot.value)) {
        return StatusCode::InvalidArgument;
    }
    snprintf(slot.key, sizeof(slot.key), "%s", key);
    snprintf(slot.value, sizeof(slot.value), "%s", value);
    s_state.header_count++;
    return StatusCode::Ok;
}

StatusCode HttpBearer() {
    AssertOwner();
    if (!s_state.slot_open ||
        s_state.in_flight.load(std::memory_order_acquire)) {
        return StatusCode::InvalidArgument;
    }
    const StatusCode status = AuthCopyAuthorization(
        s_state.url, s_state.authorization, sizeof(s_state.authorization));
    if (status == StatusCode::Ok) s_state.use_bearer = true;
    return status;
}

StatusCode HttpLimit(uint32_t limit) {
    AssertOwner();
    if (!s_state.slot_open) {
        return StatusCode::InvalidArgument;
    }
    if (limit == 0 || limit > kHttpMaxBody) {
        return StatusCode::InvalidArgument;
    }
    s_state.limit = limit;
    return StatusCode::Ok;
}

StatusCode HttpSend() {
    AssertOwner();
    if (s_state.in_flight.load(std::memory_order_acquire)) {
        return StatusCode::Busy;
    }
    if (!s_state.slot_open) {
        return StatusCode::InvalidArgument;
    }
    if (!EnsureBody()) {
        return StatusCode::OutOfMemory;
    }
    s_state.slot_open = false;
    s_state.abort.store(false, std::memory_order_release);
    s_state.body_len = 0;
    s_state.status = 0;
    s_state.line_count = 0;
    s_state.in_flight.store(true, std::memory_order_release);
    // Priority below the owner (5); internal stack (network driver calls).
    if (xTaskCreatePinnedToCore(HttpWorker, "http_worker", kWorkerStack,
                                nullptr, 4, nullptr, 0) != pdPASS) {
        s_state.in_flight.store(false, std::memory_order_release);
        return StatusCode::OutOfMemory;
    }
    ESP_LOGI(kTag, "GET %s (limit=%u)", s_state.url,
             static_cast<unsigned>(s_state.limit));
    return StatusCode::Ok;
}

void HttpAbort() {
    s_state.abort.store(true, std::memory_order_release);
}

bool HttpInFlight() {
    return s_state.in_flight.load(std::memory_order_acquire);
}

int32_t HttpStatusCode() { return s_state.status; }

uint32_t HttpLength() { return s_state.body_len; }

const char *HttpBody(uint32_t *len) {
    if (s_state.body == nullptr) {
        *len = 0;
        return nullptr;
    }
    *len = s_state.body_len;
    return s_state.body;
}

uint32_t HttpLineCount() { return s_state.line_count; }

const char *HttpLine(uint32_t i, uint32_t *len) {
    if (i >= s_state.line_count || s_state.body == nullptr) {
        return nullptr;
    }
    const LineRef &ref = s_state.lines[i];
    uint32_t n = ref.len;
    if (n > 0 && s_state.body[ref.start + n - 1] == '\r') {
        n--;
    }
    *len = n;
    return s_state.body + ref.start;
}

void FillHttpSnapshot(HttpSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->in_flight = HttpInFlight() ? 1 : 0;
    out->status = s_state.status;
    out->length = s_state.body_len;
    out->limit = s_state.limit;
    snprintf(out->url, sizeof(out->url), "%.63s", s_state.url);
}

}  // namespace pulp
