#include "net_socket.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "app_owner.h"
#include "net_auth.h"

namespace pulp {
namespace {

constexpr uint32_t kRingCapacity = 64;
constexpr uint32_t kMessageCapacity = 512;
const char *kTag = "socket";

struct Message {
    uint64_t seq;
    uint16_t len;
    char data[kMessageCapacity + 1];
};

struct SocketData {
    char url[256]{};
    char authorization[2300]{};
    char headers[2350]{};
    bool slot_open = false;
    bool use_bearer = false;
    esp_websocket_client_handle_t client = nullptr;
    SemaphoreHandle_t mutex = nullptr;
    Message *ring = nullptr;
    uint32_t head = 0;
    uint32_t count = 0;
    uint64_t next_seq = 1;
    std::atomic<uint32_t> received{0};
    std::atomic<uint32_t> dropped{0};
    std::atomic<SocketState> state{SocketState::Idle};
    char last_error[48]{};
    char assembly[kMessageCapacity + 1]{};
    uint32_t assembly_total = 0;
    uint32_t assembly_used = 0;
    bool assembly_active = false;
};

SocketData s;

void Lock() {
    if (s.mutex != nullptr) xSemaphoreTake(s.mutex, portMAX_DELAY);
}
void Unlock() {
    if (s.mutex != nullptr) xSemaphoreGive(s.mutex);
}

bool EnsureStorage() {
    if (s.mutex == nullptr) s.mutex = xSemaphoreCreateMutex();
    if (s.ring == nullptr) {
        s.ring = static_cast<Message *>(heap_caps_calloc(
            kRingCapacity, sizeof(Message), MALLOC_CAP_SPIRAM));
    }
    return s.mutex != nullptr && s.ring != nullptr;
}

void SetError(const char *error) {
    Lock();
    snprintf(s.last_error, sizeof(s.last_error), "%s",
             error != nullptr ? error : "error");
    Unlock();
    s.state.store(SocketState::Error, std::memory_order_release);
}

void StoreMessage(const char *data, uint32_t len) {
    if (s.ring == nullptr || len > kMessageCapacity) {
        s.dropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    Lock();
    const uint32_t index = (s.head + s.count) % kRingCapacity;
    uint32_t target = index;
    if (s.count == kRingCapacity) {
        target = s.head;
        s.head = (s.head + 1) % kRingCapacity;
        s.dropped.fetch_add(1, std::memory_order_relaxed);
    } else {
        s.count++;
    }
    Message &message = s.ring[target];
    message.seq = s.next_seq++;
    message.len = static_cast<uint16_t>(len);
    memcpy(message.data, data, len);
    message.data[len] = '\0';
    Unlock();
    s.received.fetch_add(1, std::memory_order_relaxed);
}

void HandleData(const esp_websocket_event_data_t *data) {
    if (data == nullptr || data->data_ptr == nullptr || data->data_len < 0 ||
        data->payload_len < 0 || data->payload_offset < 0) {
        s.dropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    const uint32_t total = static_cast<uint32_t>(data->payload_len);
    const uint32_t offset = static_cast<uint32_t>(data->payload_offset);
    const uint32_t len = static_cast<uint32_t>(data->data_len);
    if (offset == 0) {
        s.assembly_active = (data->op_code == 1 && total <= kMessageCapacity);
        s.assembly_total = total;
        s.assembly_used = 0;
        if (!s.assembly_active) {
            s.dropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
    if (!s.assembly_active || total != s.assembly_total ||
        offset != s.assembly_used || offset + len > s.assembly_total ||
        offset + len > kMessageCapacity) {
        s.assembly_active = false;
        s.dropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    memcpy(s.assembly + offset, data->data_ptr, len);
    s.assembly_used += len;
    if (s.assembly_used == s.assembly_total) {
        s.assembly[s.assembly_used] = '\0';
        StoreMessage(s.assembly, s.assembly_used);
        s.assembly_active = false;
    }
}

void EventHandler(void *, esp_event_base_t, int32_t event_id,
                  void *event_data) {
    auto *data = static_cast<esp_websocket_event_data_t *>(event_data);
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            s.state.store(SocketState::Open, std::memory_order_release);
            Lock();
            s.last_error[0] = '\0';
            Unlock();
            ESP_LOGI(kTag, "connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            s.state.store(SocketState::Connecting, std::memory_order_release);
            break;
        case WEBSOCKET_EVENT_DATA:
            HandleData(data);
            break;
        case WEBSOCKET_EVENT_ERROR: {
            int status = data != nullptr
                             ? data->error_handle.esp_ws_handshake_status_code
                             : 0;
            char error[48];
            snprintf(error, sizeof(error), "websocket_error_http_%d", status);
            SetError(error);
            break;
        }
        default:
            break;
    }
}

}  // namespace

StatusCode SocketBegin(const char *url) {
    AssertOwner();
    if (s.client != nullptr) return StatusCode::Busy;
    if (url == nullptr || strlen(url) >= sizeof(s.url) ||
        strncmp(url, "wss://", 6) != 0 || !AuthTrustedSocketUrl(url)) {
        return StatusCode::InvalidArgument;
    }
    if (!EnsureStorage()) return StatusCode::OutOfMemory;
    snprintf(s.url, sizeof(s.url), "%s", url);
    Lock();
    s.head = 0;
    s.count = 0;
    Unlock();
    s.authorization[0] = '\0';
    s.headers[0] = '\0';
    s.use_bearer = false;
    s.slot_open = true;
    return StatusCode::Ok;
}

StatusCode SocketBearer() {
    AssertOwner();
    if (!s.slot_open) return StatusCode::InvalidArgument;
    char api_url[256];
    if (snprintf(api_url, sizeof(api_url), "https://%s", s.url + 6) >=
        static_cast<int>(sizeof(api_url))) {
        return StatusCode::InvalidArgument;
    }
    const StatusCode status =
        AuthCopyAuthorization(api_url, s.authorization,
                              sizeof(s.authorization));
    // Auth validates the HTTPS API origin/path; the token never enters JS.
    if (status == StatusCode::Ok) {
        s.use_bearer = true;
    }
    return status;
}

StatusCode SocketStart() {
    AssertOwner();
    if (!s.slot_open || s.client != nullptr || !s.use_bearer ||
        !AuthAuthorized()) {
        return StatusCode::InvalidArgument;
    }
    char api_url[256];
    if (snprintf(api_url, sizeof(api_url), "https://%s", s.url + 6) >=
        static_cast<int>(sizeof(api_url))) {
        return StatusCode::InvalidArgument;
    }
    const StatusCode auth = AuthCopyAuthorization(
        api_url, s.authorization, sizeof(s.authorization));
    if (auth != StatusCode::Ok) return auth;
    snprintf(s.headers, sizeof(s.headers), "Authorization: %s\r\n",
             s.authorization);

    esp_websocket_client_config_t config{};
    config.uri = s.url;
    config.headers = s.headers;
    config.cert_pem = PulpDemoCACert();
    config.network_timeout_ms = 5000;
    config.reconnect_timeout_ms = 2000;
    config.disable_auto_reconnect = false;
    config.task_stack = 8192;
    config.task_name = "pulp_socket";
    s.client = esp_websocket_client_init(&config);
    if (s.client == nullptr) return StatusCode::OutOfMemory;
    esp_websocket_register_events(s.client, WEBSOCKET_EVENT_ANY, EventHandler,
                                  nullptr);
    const esp_err_t started = esp_websocket_client_start(s.client);
    if (started != ESP_OK) {
        esp_websocket_client_destroy(s.client);
        s.client = nullptr;
        SetError(esp_err_to_name(started));
        return StatusCode::Busy;
    }
    s.slot_open = false;
    s.state.store(SocketState::Connecting, std::memory_order_release);
    // Header has been copied into the client configuration.
    memset(s.authorization, 0, sizeof(s.authorization));
    return StatusCode::Ok;
}

void SocketStop() {
    AssertOwner();
    if (s.client != nullptr) {
        esp_websocket_client_stop(s.client);
        esp_websocket_client_destroy(s.client);
        s.client = nullptr;
    }
    memset(s.authorization, 0, sizeof(s.authorization));
    memset(s.headers, 0, sizeof(s.headers));
    s.slot_open = false;
    s.use_bearer = false;
    s.state.store(SocketState::Idle, std::memory_order_release);
}

void SocketTick() {
    AssertOwner();
    if (s.client != nullptr && !AuthAuthorized()) SocketStop();
}

int32_t SocketStatus() {
    return static_cast<int32_t>(s.state.load(std::memory_order_acquire));
}

const char *SocketStateName() {
    switch (s.state.load(std::memory_order_acquire)) {
        case SocketState::Idle: return "idle";
        case SocketState::Connecting: return "connecting";
        case SocketState::Open: return "open";
        case SocketState::Error: return "error";
    }
    return "unknown";
}

uint32_t SocketMessageCount() {
    Lock();
    const uint32_t count = s.count;
    Unlock();
    return count;
}

bool SocketCopyMessage(uint32_t index, char *out, size_t cap, uint64_t *seq) {
    if (out == nullptr || cap == 0 || seq == nullptr) return false;
    Lock();
    if (index >= s.count) {
        Unlock();
        return false;
    }
    const Message &message = s.ring[(s.head + index) % kRingCapacity];
    if (message.len + 1 > cap) {
        Unlock();
        return false;
    }
    memcpy(out, message.data, message.len + 1);
    *seq = message.seq;
    Unlock();
    return true;
}

uint32_t SocketDropped() {
    return s.dropped.load(std::memory_order_relaxed);
}
uint32_t SocketReceived() {
    return s.received.load(std::memory_order_relaxed);
}
const char *SocketLastError() { return s.last_error; }

void FillSocketSnapshot(SocketSnapshot *out) {
    memset(out, 0, sizeof(*out));
    out->state = static_cast<uint8_t>(SocketStatus());
    out->received = SocketReceived();
    out->dropped = SocketDropped();
    Lock();
    out->ring_count = s.count;
    snprintf(out->error, sizeof(out->error), "%s", s.last_error);
    Unlock();
}

}  // namespace pulp
