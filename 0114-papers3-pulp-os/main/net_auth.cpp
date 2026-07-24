#include "net_auth.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <strings.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_owner.h"
#include "net_wifi.h"

namespace pulp {
namespace {

constexpr uint32_t kResponseCap = 8192;
constexpr uint32_t kWorkerStack = 7168;
constexpr uint32_t kTimeoutMs = 10000;
constexpr uint32_t kDefaultPollSeconds = 5;
constexpr uint32_t kMaxPollSeconds = 60;
constexpr int32_t kKindDeviceCode = 20;
constexpr int32_t kKindTokenPoll = 21;
const char *kTag = "auth";

extern const uint8_t ca_start[] asm("_binary_pulp_demo_ca_pem_start");

struct AuthData {
    char issuer[192]{};
    char client_id[48]{};
    char scopes[160]{};
    char resource[192]{};
    char socket_resource[192]{};

    char endpoint[240]{};
    char form[1024]{};
    int32_t worker_kind = 0;
    std::atomic<bool> in_flight{false};
    std::atomic<bool> clear_pending{false};
    char *response = nullptr;
    uint32_t response_len = 0;
    bool response_overflow = false;
    int32_t http_status = 0;

    char device_code[256]{};
    char user_code[24]{};
    char verification_uri[256]{};
    char verification_uri_complete[384]{};
    char access_token[2048]{};

    AuthState state = AuthState::Unconfigured;
    char error[40]{};
    int64_t grant_deadline_us = 0;
    int64_t token_deadline_us = 0;
    int64_t next_poll_us = 0;
    uint32_t poll_interval_s = kDefaultPollSeconds;
};

AuthData s;

void SecureZero(char *p, size_t n) {
    volatile char *v = p;
    while (n-- > 0) *v++ = 0;
}

void ResetSecrets() {
    SecureZero(s.device_code, sizeof(s.device_code));
    SecureZero(s.access_token, sizeof(s.access_token));
    s.user_code[0] = '\0';
    s.verification_uri[0] = '\0';
    s.verification_uri_complete[0] = '\0';
    s.grant_deadline_us = 0;
    s.token_deadline_us = 0;
    s.next_poll_us = 0;
    s.poll_interval_s = kDefaultPollSeconds;
}

void SetError(const char *name) {
    snprintf(s.error, sizeof(s.error), "%s", name != nullptr ? name : "error");
    s.state = AuthState::Error;
}

bool EnsureResponse() {
    if (s.response != nullptr) return true;
    s.response = static_cast<char *>(
        heap_caps_malloc(kResponseCap + 1, MALLOC_CAP_SPIRAM));
    return s.response != nullptr;
}

bool CopyJsonString(cJSON *root, const char *key, char *out, size_t cap) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsString(item) || item->valuestring == nullptr ||
        strlen(item->valuestring) >= cap) {
        return false;
    }
    snprintf(out, cap, "%s", item->valuestring);
    return true;
}

int32_t JsonInt(cJSON *root, const char *key, int32_t fallback) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < 0 ||
        item->valuedouble > 2147483647.0) {
        return fallback;
    }
    return static_cast<int32_t>(item->valuedouble);
}

bool AppendEncoded(char *out, size_t cap, size_t *used, const char *value) {
    static const char hex[] = "0123456789ABCDEF";
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(value);
         *p != 0; ++p) {
        const bool plain = (*p >= 'a' && *p <= 'z') ||
                           (*p >= 'A' && *p <= 'Z') ||
                           (*p >= '0' && *p <= '9') || *p == '-' ||
                           *p == '_' || *p == '.' || *p == '~';
        const size_t need = plain ? 1 : 3;
        if (*used + need >= cap) return false;
        if (plain) {
            out[(*used)++] = static_cast<char>(*p);
        } else {
            out[(*used)++] = '%';
            out[(*used)++] = hex[*p >> 4];
            out[(*used)++] = hex[*p & 15];
        }
    }
    out[*used] = '\0';
    return true;
}

bool BuildForm(const char *const keys[], const char *const values[],
               size_t count) {
    size_t used = 0;
    s.form[0] = '\0';
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            if (used + 1 >= sizeof(s.form)) return false;
            s.form[used++] = '&';
            s.form[used] = '\0';
        }
        if (!AppendEncoded(s.form, sizeof(s.form), &used, keys[i])) return false;
        if (used + 1 >= sizeof(s.form)) return false;
        s.form[used++] = '=';
        s.form[used] = '\0';
        if (!AppendEncoded(s.form, sizeof(s.form), &used, values[i])) return false;
    }
    return true;
}

esp_err_t HttpEvent(esp_http_client_event_t *event) {
    if (event->event_id != HTTP_EVENT_ON_DATA || event->data_len <= 0) {
        return ESP_OK;
    }
    const uint32_t room = kResponseCap - s.response_len;
    const uint32_t take = static_cast<uint32_t>(event->data_len) < room
                              ? static_cast<uint32_t>(event->data_len)
                              : room;
    if (take > 0) {
        memcpy(s.response + s.response_len, event->data, take);
        s.response_len += take;
    }
    if (take != static_cast<uint32_t>(event->data_len)) {
        s.response_overflow = true;
    }
    return ESP_OK;
}

void Worker(void *) {
    int32_t transport_err = 0;
    esp_http_client_config_t config{};
    config.url = s.endpoint;
    config.timeout_ms = kTimeoutMs;
    config.cert_pem = PulpDemoCACert();
    config.event_handler = HttpEvent;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        transport_err = -1;
    } else {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type",
                                   "application/x-www-form-urlencoded");
        esp_http_client_set_post_field(client, s.form,
                                       static_cast<int>(strlen(s.form)));
        const esp_err_t performed = esp_http_client_perform(client);
        if (performed != ESP_OK) {
            transport_err = -static_cast<int32_t>(performed);
        } else {
            s.http_status = esp_http_client_get_status_code(client);
        }
        esp_http_client_cleanup(client);
    }
    s.response[s.response_len] = '\0';
    const int32_t kind = s.worker_kind;
    s.in_flight.store(false, std::memory_order_release);
    (void)PostModuleDone(ModuleId::Auth, kind, s.http_status, transport_err);
    vTaskDelete(nullptr);
}

StatusCode StartWorker(int32_t kind, const char *suffix) {
    if (s.in_flight.load(std::memory_order_acquire)) return StatusCode::Busy;
    if (!EnsureResponse()) return StatusCode::OutOfMemory;
    if (snprintf(s.endpoint, sizeof(s.endpoint), "%s/%s", s.issuer, suffix) >=
        static_cast<int>(sizeof(s.endpoint))) {
        return StatusCode::InvalidArgument;
    }
    s.worker_kind = kind;
    s.response_len = 0;
    s.response_overflow = false;
    s.http_status = 0;
    s.in_flight.store(true, std::memory_order_release);
    if (xTaskCreatePinnedToCore(Worker, "auth_http", kWorkerStack, nullptr, 4,
                                nullptr, 0) != pdPASS) {
        s.in_flight.store(false, std::memory_order_release);
        return StatusCode::OutOfMemory;
    }
    return StatusCode::Ok;
}

void HandleDeviceResponse(int32_t transport_err) {
    if (transport_err != 0) {
        SetError("transport_error");
        return;
    }
    if (s.response_overflow || s.http_status != 200) {
        SetError(s.response_overflow ? "response_too_large" : "device_request_failed");
        return;
    }
    cJSON *root = cJSON_ParseWithLength(s.response, s.response_len);
    if (root == nullptr) {
        SetError("invalid_json");
        return;
    }
    const int32_t expires = JsonInt(root, "expires_in", 0);
    int32_t interval = JsonInt(root, "interval", kDefaultPollSeconds);
    const bool valid = expires > 0 &&
                       CopyJsonString(root, "device_code", s.device_code,
                                      sizeof(s.device_code)) &&
                       CopyJsonString(root, "user_code", s.user_code,
                                      sizeof(s.user_code)) &&
                       CopyJsonString(root, "verification_uri",
                                      s.verification_uri,
                                      sizeof(s.verification_uri)) &&
                       CopyJsonString(root, "verification_uri_complete",
                                      s.verification_uri_complete,
                                      sizeof(s.verification_uri_complete));
    cJSON_Delete(root);
    if (!valid) {
        ResetSecrets();
        SetError("invalid_device_response");
        return;
    }
    if (interval < 1) interval = kDefaultPollSeconds;
    if (interval > static_cast<int32_t>(kMaxPollSeconds)) interval = kMaxPollSeconds;
    s.poll_interval_s = static_cast<uint32_t>(interval);
    const int64_t now = esp_timer_get_time();
    s.grant_deadline_us = now + static_cast<int64_t>(expires) * 1000000;
    s.next_poll_us = now + static_cast<int64_t>(s.poll_interval_s) * 1000000;
    s.error[0] = '\0';
    s.state = AuthState::WaitingForUser;
}

void HandlePollResponse(int32_t transport_err) {
    const int64_t now = esp_timer_get_time();
    if (transport_err != 0) {
        s.poll_interval_s = s.poll_interval_s < 30 ? s.poll_interval_s * 2 : 30;
        s.next_poll_us = now + static_cast<int64_t>(s.poll_interval_s) * 1000000;
        snprintf(s.error, sizeof(s.error), "transport_retry");
        s.state = AuthState::WaitingForUser;
        return;
    }
    if (s.response_overflow) {
        SetError("response_too_large");
        return;
    }
    cJSON *root = cJSON_ParseWithLength(s.response, s.response_len);
    if (root == nullptr) {
        SetError("invalid_json");
        return;
    }
    if (s.http_status == 200) {
        char token_type[16];
        const int32_t expires = JsonInt(root, "expires_in", 0);
        const bool valid = expires > 0 &&
                           CopyJsonString(root, "access_token", s.access_token,
                                          sizeof(s.access_token)) &&
                           CopyJsonString(root, "token_type", token_type,
                                          sizeof(token_type)) &&
                           (strcasecmp(token_type, "bearer") == 0);
        cJSON_Delete(root);
        if (!valid) {
            ResetSecrets();
            SetError("invalid_token_response");
            return;
        }
        SecureZero(s.device_code, sizeof(s.device_code));
        s.user_code[0] = '\0';
        s.verification_uri[0] = '\0';
        s.verification_uri_complete[0] = '\0';
        s.grant_deadline_us = 0;
        s.next_poll_us = 0;
        s.token_deadline_us = now + static_cast<int64_t>(expires) * 1000000;
        s.error[0] = '\0';
        s.state = AuthState::Authorized;
        ESP_LOGI(kTag, "device authorization complete (token bytes=%u)",
                 static_cast<unsigned>(strlen(s.access_token)));
        return;
    }
    char error[40];
    const bool has_error = CopyJsonString(root, "error", error, sizeof(error));
    cJSON_Delete(root);
    if (!has_error) {
        SetError("token_request_failed");
        return;
    }
    if (strcmp(error, "authorization_pending") == 0) {
        s.next_poll_us = now + static_cast<int64_t>(s.poll_interval_s) * 1000000;
        s.error[0] = '\0';
        s.state = AuthState::WaitingForUser;
    } else if (strcmp(error, "slow_down") == 0) {
        s.poll_interval_s += 5;
        if (s.poll_interval_s > kMaxPollSeconds) s.poll_interval_s = kMaxPollSeconds;
        s.next_poll_us = now + static_cast<int64_t>(s.poll_interval_s) * 1000000;
        snprintf(s.error, sizeof(s.error), "slow_down");
        s.state = AuthState::WaitingForUser;
    } else {
        ResetSecrets();
        SetError(error);
    }
}

int32_t SecondsLeft(int64_t deadline) {
    if (deadline <= 0) return 0;
    const int64_t left = deadline - esp_timer_get_time();
    if (left <= 0) return 0;
    return static_cast<int32_t>((left + 999999) / 1000000);
}

}  // namespace

const char *PulpDemoCACert() { return reinterpret_cast<const char *>(ca_start); }

StatusCode AuthConfigure(const char *issuer, const char *client_id,
                         const char *scopes, const char *resource) {
    AssertOwner();
    if (s.in_flight.load(std::memory_order_acquire)) return StatusCode::Busy;
    if (issuer == nullptr || client_id == nullptr || scopes == nullptr ||
        resource == nullptr || strncmp(issuer, "https://", 8) != 0 ||
        strncmp(resource, "https://", 8) != 0 ||
        strlen(issuer) >= sizeof(s.issuer) ||
        strlen(client_id) >= sizeof(s.client_id) ||
        strlen(scopes) >= sizeof(s.scopes) ||
        strlen(resource) >= sizeof(s.resource)) {
        return StatusCode::InvalidArgument;
    }
    ResetSecrets();
    snprintf(s.issuer, sizeof(s.issuer), "%s", issuer);
    snprintf(s.client_id, sizeof(s.client_id), "%s", client_id);
    snprintf(s.scopes, sizeof(s.scopes), "%s", scopes);
    snprintf(s.resource, sizeof(s.resource), "%s", resource);
    snprintf(s.socket_resource, sizeof(s.socket_resource), "wss://%s",
             resource + 8);
    s.error[0] = '\0';
    s.state = AuthState::Idle;
    return StatusCode::Ok;
}

StatusCode AuthStart() {
    AssertOwner();
    if (s.state == AuthState::Unconfigured) return StatusCode::InvalidArgument;
    if (s.in_flight.load(std::memory_order_acquire)) return StatusCode::Busy;
    // esp_http_client ultimately posts to lwIP's TCP/IP mailbox. Wi-Fi is
    // initialized lazily in this firmware, so starting before it is up would
    // assert inside lwIP (`Invalid mbox`) instead of returning a safe error.
    if (WifiStatus() != kWifiUp) return StatusCode::InvalidArgument;
    ResetSecrets();
    const char *keys[] = {"client_id", "scope", "resource"};
    const char *values[] = {s.client_id, s.scopes, s.resource};
    if (!BuildForm(keys, values, 3)) return StatusCode::CapacityExceeded;
    const StatusCode status = StartWorker(kKindDeviceCode, "device_authorization");
    if (status == StatusCode::Ok) {
        s.error[0] = '\0';
        s.state = AuthState::RequestingCode;
    }
    return status;
}

void AuthTick(int64_t now_us) {
    AssertOwner();
    if (s.state == AuthState::Authorized && s.token_deadline_us > 0 &&
        now_us >= s.token_deadline_us) {
        ResetSecrets();
        snprintf(s.error, sizeof(s.error), "token_expired");
        s.state = AuthState::Expired;
        return;
    }
    if (s.state != AuthState::WaitingForUser ||
        s.in_flight.load(std::memory_order_acquire)) {
        return;
    }
    if (s.grant_deadline_us > 0 && now_us >= s.grant_deadline_us) {
        ResetSecrets();
        snprintf(s.error, sizeof(s.error), "expired_token");
        s.state = AuthState::Expired;
        return;
    }
    if (now_us < s.next_poll_us || WifiStatus() != kWifiUp) return;
    const char *keys[] = {"grant_type", "client_id", "device_code"};
    const char *values[] = {
        "urn:ietf:params:oauth:grant-type:device_code", s.client_id,
        s.device_code};
    if (!BuildForm(keys, values, 3)) {
        SetError("form_too_large");
        return;
    }
    const StatusCode status = StartWorker(kKindTokenPoll, "token");
    if (status == StatusCode::Ok) {
        s.state = AuthState::PollingToken;
    } else {
        SetError(StatusCodeName(status));
    }
}

void AuthOwnerOnModuleDone(int32_t kind, int32_t, int32_t err) {
    AssertOwner();
    if (s.clear_pending.exchange(false, std::memory_order_acq_rel)) {
        ResetSecrets();
        s.error[0] = '\0';
        s.state = AuthState::Idle;
        return;
    }
    if (kind == kKindDeviceCode) {
        HandleDeviceResponse(err);
    } else if (kind == kKindTokenPoll) {
        HandlePollResponse(err);
    }
}

void AuthClear() {
    AssertOwner();
    if (s.in_flight.load(std::memory_order_acquire)) {
        s.clear_pending.store(true, std::memory_order_release);
        s.state = AuthState::Idle;
        return;
    }
    ResetSecrets();
    s.error[0] = '\0';
    s.state = s.issuer[0] == '\0' ? AuthState::Unconfigured : AuthState::Idle;
}

int32_t AuthStatus() { return static_cast<int32_t>(s.state); }

const char *AuthStateName() {
    switch (s.state) {
        case AuthState::Unconfigured: return "unconfigured";
        case AuthState::Idle: return "idle";
        case AuthState::RequestingCode: return "requesting";
        case AuthState::WaitingForUser: return "waiting";
        case AuthState::PollingToken: return "polling";
        case AuthState::Authorized: return "authorized";
        case AuthState::Expired: return "expired";
        case AuthState::Error: return "error";
    }
    return "unknown";
}

const char *AuthUserCode() { return s.user_code; }
const char *AuthVerificationUri() { return s.verification_uri; }
const char *AuthVerificationUriComplete() { return s.verification_uri_complete; }
const char *AuthErrorName() { return s.error; }
int32_t AuthGrantSecondsLeft() { return SecondsLeft(s.grant_deadline_us); }
int32_t AuthTokenSecondsLeft() { return SecondsLeft(s.token_deadline_us); }
int32_t AuthPollSecondsLeft() { return SecondsLeft(s.next_poll_us); }
bool AuthAuthorized() {
    return s.state == AuthState::Authorized && AuthTokenSecondsLeft() > 0;
}

bool AuthTrustedApiUrl(const char *url) {
    if (url == nullptr || s.resource[0] == '\0') return false;
    const size_t n = strlen(s.resource);
    return strncmp(url, s.resource, n) == 0 && url[n] == '/';
}

bool AuthTrustedSocketUrl(const char *url) {
    if (url == nullptr || s.socket_resource[0] == '\0') return false;
    const size_t n = strlen(s.socket_resource);
    return strncmp(url, s.socket_resource, n) == 0 && url[n] == '/';
}

StatusCode AuthCopyAuthorization(const char *url, char *out, size_t cap) {
    AssertOwner();
    if (!AuthAuthorized() || !AuthTrustedApiUrl(url) || out == nullptr ||
        cap <= strlen(s.access_token) + 7) {
        return StatusCode::InvalidArgument;
    }
    snprintf(out, cap, "Bearer %s", s.access_token);
    return StatusCode::Ok;
}

void FillAuthSnapshot(AuthSnapshot *out) {
    memset(out, 0, sizeof(*out));
    out->state = static_cast<uint8_t>(s.state);
    out->in_flight = s.in_flight.load(std::memory_order_acquire) ? 1 : 0;
    out->grant_left = AuthGrantSecondsLeft();
    out->token_left = AuthTokenSecondsLeft();
    out->poll_left = AuthPollSecondsLeft();
    out->token_len = static_cast<uint16_t>(strlen(s.access_token));
    snprintf(out->user_code, sizeof(out->user_code), "%s", s.user_code);
    snprintf(out->error, sizeof(out->error), "%s", s.error);
}

}  // namespace pulp
