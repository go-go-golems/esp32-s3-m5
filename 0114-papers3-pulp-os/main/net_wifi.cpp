#include "net_wifi.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "app_owner.h"
#include "s3paper_storage/storage.h"

namespace pulp {
namespace {

const char *kTag = "wifi";

constexpr int64_t kJoinTimeoutUs = 15'000'000;
constexpr uint32_t kJoinRetries = 2;

struct ScanEntry {
    char ssid[33];
    int8_t rssi;
    uint8_t secure;
};

// Module state. Writers: owner (verbs) + event task (handlers). The
// atomic `state` publishes handler-side transitions; the scan mailbox is
// published by the ModuleDone queue barrier.
struct WifiState {
    bool inited = false;          // esp_wifi_init done (owner)
    bool started = false;         // esp_wifi_start active (owner)
    std::atomic<uint8_t> state{kWifiOff};
    std::atomic<uint32_t> ip{0};  // network byte order
    char target_ssid[33] = {};    // owner-written before connect
    std::atomic<uint32_t> retries{0};
    int64_t join_deadline_us = 0;  // owner-only
    // joinSaved sequencing (owner-only).
    bool saved_mode = false;
    uint32_t saved_rank = 0;
    // Scan mailbox (event task writes, owner reads after completion).
    ScanEntry scan[kWifiScanMax];
    uint32_t scan_count = 0;
};

WifiState s_state;

void HandlerWifi(void *, esp_event_base_t, int32_t event_id, void *data) {
    // SYSTEM EVENT TASK: POD state + PostModuleDone only.
    switch (event_id) {
        case WIFI_EVENT_SCAN_DONE: {
            uint16_t n = kWifiScanMax;
            wifi_ap_record_t records[kWifiScanMax];
            if (esp_wifi_scan_get_ap_records(&n, records) != ESP_OK) {
                n = 0;
            }
            s_state.scan_count = n;
            for (uint16_t i = 0; i < n; ++i) {
                snprintf(s_state.scan[i].ssid, sizeof(s_state.scan[i].ssid),
                         "%s", reinterpret_cast<const char *>(
                                   records[i].ssid));
                s_state.scan[i].rssi = records[i].rssi;
                s_state.scan[i].secure =
                    records[i].authmode == WIFI_AUTH_OPEN ? 0 : 1;
            }
            s_state.state.store(kWifiIdle, std::memory_order_release);
            (void)PostModuleDone(ModuleId::Wifi, kDoneWifiScan,
                                 static_cast<int32_t>(n), 0);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED: {
            const auto *ev =
                static_cast<wifi_event_sta_disconnected_t *>(data);
            const uint8_t st =
                s_state.state.load(std::memory_order_acquire);
            if (st == kWifiJoining) {
                const uint32_t tried = s_state.retries.fetch_add(1) + 1;
                if (tried <= kJoinRetries) {
                    ESP_LOGW(kTag, "join retry %u (reason=%d)",
                             static_cast<unsigned>(tried),
                             static_cast<int>(ev->reason));
                    esp_wifi_connect();
                } else {
                    s_state.state.store(kWifiIdle,
                                        std::memory_order_release);
                    (void)PostModuleDone(ModuleId::Wifi, kDoneWifiJoin, 0,
                                         static_cast<int32_t>(ev->reason));
                }
            } else if (st == kWifiUp) {
                // Unexpected drop: report as state only (no callback was
                // registered for it); apps poll wifi.status().
                s_state.ip.store(0, std::memory_order_release);
                s_state.state.store(kWifiIdle, std::memory_order_release);
                ESP_LOGW(kTag, "link lost (reason=%d)",
                         static_cast<int>(ev->reason));
            }
            break;
        }
        default:
            break;
    }
}

void HandlerIp(void *, esp_event_base_t, int32_t event_id, void *data) {
    // SYSTEM EVENT TASK.
    if (event_id != IP_EVENT_STA_GOT_IP) {
        return;
    }
    const auto *ev = static_cast<ip_event_got_ip_t *>(data);
    s_state.ip.store(ev->ip_info.ip.addr, std::memory_order_release);
    s_state.state.store(kWifiUp, std::memory_order_release);
    (void)PostModuleDone(ModuleId::Wifi, kDoneWifiJoin, 1, 0);
}

StatusCode EnsureUp() {
    AssertOwner();
    if (!s_state.inited) {
        if (esp_netif_init() != ESP_OK) {
            return StatusCode::Busy;
        }
        const esp_err_t loop = esp_event_loop_create_default();
        if (loop != ESP_OK && loop != ESP_ERR_INVALID_STATE) {
            return StatusCode::Busy;
        }
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        if (esp_wifi_init(&cfg) != ESP_OK) {
            return StatusCode::Busy;
        }
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &HandlerWifi, nullptr, nullptr));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &HandlerIp, nullptr, nullptr));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        s_state.inited = true;
    }
    if (!s_state.started) {
        if (esp_wifi_start() != ESP_OK) {
            return StatusCode::Busy;
        }
        s_state.started = true;
        s_state.state.store(kWifiIdle, std::memory_order_release);
        ESP_LOGI(kTag, "radio up (station)");
    }
    return StatusCode::Ok;
}

// Starts one join attempt against the current target_ssid/pass.
StatusCode StartJoin(const char *ssid, const char *pass) {
    wifi_config_t cfg = {};
    snprintf(reinterpret_cast<char *>(cfg.sta.ssid),
             sizeof(cfg.sta.ssid), "%s", ssid);
    snprintf(reinterpret_cast<char *>(cfg.sta.password),
             sizeof(cfg.sta.password), "%s", pass);
    if (esp_wifi_set_config(WIFI_IF_STA, &cfg) != ESP_OK) {
        return StatusCode::InvalidArgument;
    }
    snprintf(s_state.target_ssid, sizeof(s_state.target_ssid), "%s", ssid);
    s_state.retries.store(0);
    s_state.join_deadline_us = esp_timer_get_time() + kJoinTimeoutUs;
    s_state.state.store(kWifiJoining, std::memory_order_release);
    if (esp_wifi_connect() != ESP_OK) {
        s_state.state.store(kWifiIdle, std::memory_order_release);
        return StatusCode::Busy;
    }
    ESP_LOGI(kTag, "joining \"%s\"", ssid);
    return StatusCode::Ok;
}

// Owner-side: try the next stored credential in the joinSaved sequence.
// Returns Ok when an attempt started, InvalidArgument when exhausted.
StatusCode SavedTryNext() {
    const s3paper_storage::WifiCredential *cred =
        s3paper_storage::WifiCredsGetRanked(s_state.saved_rank);
    if (cred == nullptr) {
        return StatusCode::InvalidArgument;
    }
    s_state.saved_rank++;
    return StartJoin(cred->ssid, cred->pass);
}

bool OpInFlight() {
    const uint8_t st = s_state.state.load(std::memory_order_acquire);
    return st == kWifiScanning || st == kWifiJoining;
}

}  // namespace

StatusCode WifiScan() {
    AssertOwner();
    if (OpInFlight()) {
        return StatusCode::Busy;
    }
    const StatusCode up = EnsureUp();
    if (up != StatusCode::Ok) {
        return up;
    }
    s_state.scan_count = 0;
    s_state.state.store(kWifiScanning, std::memory_order_release);
    if (esp_wifi_scan_start(nullptr, false) != ESP_OK) {
        s_state.state.store(kWifiIdle, std::memory_order_release);
        return StatusCode::Busy;
    }
    return StatusCode::Ok;
}

StatusCode WifiJoin(const char *ssid, const char *pass) {
    AssertOwner();
    if (ssid == nullptr || ssid[0] == '\0' || strlen(ssid) > 32 ||
        (pass != nullptr && strlen(pass) > 64)) {
        return StatusCode::InvalidArgument;
    }
    if (OpInFlight()) {
        return StatusCode::Busy;
    }
    const StatusCode up = EnsureUp();
    if (up != StatusCode::Ok) {
        return up;
    }
    s_state.saved_mode = false;
    return StartJoin(ssid, pass != nullptr ? pass : "");
}

StatusCode WifiJoinSaved() {
    AssertOwner();
    if (OpInFlight()) {
        return StatusCode::Busy;
    }
    if (s3paper_storage::WifiCredsCount() == 0) {
        return StatusCode::InvalidArgument;
    }
    const StatusCode up = EnsureUp();
    if (up != StatusCode::Ok) {
        return up;
    }
    s_state.saved_mode = true;
    s_state.saved_rank = 0;
    const StatusCode started = SavedTryNext();
    if (started != StatusCode::Ok) {
        s_state.saved_mode = false;
    }
    return started;
}

StatusCode WifiOff() {
    AssertOwner();
    if (!s_state.started) {
        return StatusCode::Ok;
    }
    esp_wifi_disconnect();
    esp_wifi_stop();
    s_state.started = false;
    s_state.saved_mode = false;
    s_state.ip.store(0, std::memory_order_release);
    s_state.state.store(kWifiOff, std::memory_order_release);
    ESP_LOGI(kTag, "radio down");
    return StatusCode::Ok;
}

void WifiTick(int64_t now_us) {
    if (s_state.state.load(std::memory_order_acquire) != kWifiJoining ||
        s_state.join_deadline_us == 0 ||
        now_us < s_state.join_deadline_us) {
        return;
    }
    s_state.join_deadline_us = 0;
    ESP_LOGW(kTag, "join timeout for \"%s\"", s_state.target_ssid);
    s_state.state.store(kWifiIdle, std::memory_order_release);
    esp_wifi_disconnect();  // stops handler-side retries
    (void)PostModuleDone(ModuleId::Wifi, kDoneWifiJoin, 0, -1);
}

bool WifiOwnerOnModuleDone(int32_t kind, int32_t *value, int32_t *err) {
    AssertOwner();
    if (kind != kDoneWifiJoin) {
        return true;
    }
    s_state.join_deadline_us = 0;
    if (*value == 1) {
        // Success: remember which network works.
        s_state.saved_mode = false;
        s3paper_storage::WifiCredsMarkOk(s_state.target_ssid);
        return true;
    }
    if (s_state.saved_mode) {
        // Failure inside the joinSaved sequence: quietly try the next
        // stored network; deliver only when the list is exhausted.
        if (SavedTryNext() == StatusCode::Ok) {
            return false;
        }
        s_state.saved_mode = false;
    }
    return true;
}

uint8_t WifiStatus() {
    return s_state.state.load(std::memory_order_acquire);
}

void WifiIp(char *out, size_t cap) {
    const uint32_t ip = s_state.ip.load(std::memory_order_acquire);
    if (ip == 0 ||
        s_state.state.load(std::memory_order_acquire) != kWifiUp) {
        snprintf(out, cap, "%s", "");
        return;
    }
    snprintf(out, cap, "%u.%u.%u.%u", static_cast<unsigned>(ip & 0xFF),
             static_cast<unsigned>((ip >> 8) & 0xFF),
             static_cast<unsigned>((ip >> 16) & 0xFF),
             static_cast<unsigned>((ip >> 24) & 0xFF));
}

const char *WifiSsidCurrent() {
    return s_state.state.load(std::memory_order_acquire) == kWifiUp
               ? s_state.target_ssid
               : "";
}

int32_t WifiRssiCurrent() {
    if (s_state.state.load(std::memory_order_acquire) != kWifiUp) {
        return 0;
    }
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return 0;
    }
    return ap.rssi;
}

uint32_t WifiScanCount() { return s_state.scan_count; }

const char *WifiScanSsid(uint32_t i) {
    return i < s_state.scan_count ? s_state.scan[i].ssid : nullptr;
}

int32_t WifiScanRssi(uint32_t i) {
    return i < s_state.scan_count ? s_state.scan[i].rssi : 0;
}

int32_t WifiScanSecure(uint32_t i) {
    return i < s_state.scan_count ? s_state.scan[i].secure : -1;
}

void FillNetSnapshot(NetSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->state = WifiStatus();
    WifiIp(out->ip, sizeof(out->ip));
    snprintf(out->ssid, sizeof(out->ssid), "%s", WifiSsidCurrent());
    out->rssi = WifiRssiCurrent();
    out->scan_count = s_state.scan_count;
    out->saved_count = s3paper_storage::WifiCredsCount();
}

}  // namespace pulp
