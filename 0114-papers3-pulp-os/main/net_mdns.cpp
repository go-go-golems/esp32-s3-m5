#include "net_mdns.h"

#include <atomic>
#include <cstring>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mdns.h"

#include "app_owner.h"
#include "net_wifi.h"

namespace pulp {
namespace {

const char *kTag = "mdns";

struct MdnsState {
    bool inited = false;      // mdns_init done (owner)
    bool announced = false;   // service added (owner)
    uint16_t port = 0;        // announced port
};

MdnsState s_state;

// ESP-58 browse state. Result mailbox: worker writes while in flight,
// owner/JS read after ModuleDone — the net_http mailbox contract.
struct BrowseResult {
    char name[32];
    // Sized for the worst assembled URL: "http://" + host(63) + ":65535"
    // + path(47) + NUL = 124.
    char url[128];
};

std::atomic<bool> s_browse_in_flight{false};
bool s_stop_deferred = false;   // owner-only flag (set/read on owner)
BrowseResult s_browse[kMdnsMaxResults];
uint32_t s_browse_count = 0;

// Copies a TXT value by key into out; returns false if the key is absent
// or the value empty/oversized-truncated-to-empty.
bool TxtGet(const mdns_result_t *r, const char *key, char *out,
            size_t cap) {
    for (size_t i = 0; i < r->txt_count; ++i) {
        if (r->txt[i].key != nullptr && strcmp(r->txt[i].key, key) == 0 &&
            r->txt[i].value != nullptr && r->txt[i].value[0] != '\0') {
            snprintf(out, cap, "%s", r->txt[i].value);
            return true;
        }
    }
    return false;
}

void BrowseWorker(void *) {
    mdns_result_t *results = nullptr;
    const esp_err_t rc = mdns_query_ptr(kMdnsAppSvc, "_tcp",
                                        kMdnsBrowseWindowMs,
                                        kMdnsMaxResults, &results);
    uint32_t n = 0;
    if (rc == ESP_OK) {
        for (const mdns_result_t *r = results;
             r != nullptr && n < kMdnsMaxResults; r = r->next) {
            // Dial target: prefer a resolved IPv4 (works even when the
            // resolver on the other side is flaky); fall back to
            // <hostname>.local; skip results with neither.
            char host[64] = "";
            for (const mdns_ip_addr_t *a = r->addr; a != nullptr;
                 a = a->next) {
                if (a->addr.type == ESP_IPADDR_TYPE_V4) {
                    snprintf(host, sizeof(host), IPSTR,
                             IP2STR(&a->addr.u_addr.ip4));
                    break;
                }
            }
            if (host[0] == '\0' && r->hostname != nullptr &&
                r->hostname[0] != '\0') {
                snprintf(host, sizeof(host), "%s.local", r->hostname);
            }
            if (host[0] == '\0') {
                continue;
            }
            BrowseResult &out = s_browse[n];
            if (!TxtGet(r, "name", out.name, sizeof(out.name))) {
                snprintf(out.name, sizeof(out.name), "%.31s",
                         r->instance_name != nullptr ? r->instance_name
                                                     : host);
            }
            char path[48];
            if (!TxtGet(r, "path", path, sizeof(path)) || path[0] != '/') {
                snprintf(path, sizeof(path), "%s", kMdnsDefaultIndexPath);
            }
            const uint16_t port = r->port != 0 ? r->port : 80;
            if (port == 80) {
                snprintf(out.url, sizeof(out.url), "http://%s%s", host,
                         path);
            } else {
                snprintf(out.url, sizeof(out.url), "http://%s:%u%s", host,
                         static_cast<unsigned>(port), path);
            }
            n++;
        }
        mdns_query_results_free(results);
    } else {
        ESP_LOGW(kTag, "browse query failed: %d", static_cast<int>(rc));
    }
    s_browse_count = n;
    s_browse_in_flight.store(false, std::memory_order_release);
    ESP_LOGI(kTag, "browse done: %u server(s)", static_cast<unsigned>(n));
    // Queue reservation (ESP-57) keeps completion posts from losing the
    // tick race; a failure here is logged by PostModuleDone's caller.
    if (PostModuleDone(ModuleId::Mdns, kDoneMdnsBrowse,
                       static_cast<int32_t>(n),
                       rc == ESP_OK ? 0 : 2) != StatusCode::Ok) {
        ESP_LOGW(kTag, "browse completion post failed (cb stranded)");
    }
    vTaskDelete(nullptr);
}

}  // namespace

StatusCode MdnsInit() {
    AssertOwner();
    if (s_state.inited) {
        return StatusCode::Ok;
    }
    if (mdns_init() != ESP_OK) {
        ESP_LOGW(kTag, "mdns_init failed");
        return StatusCode::Busy;
    }
    if (mdns_hostname_set(kMdnsHost) != ESP_OK) {
        ESP_LOGW(kTag, "mdns_hostname_set failed");
        return StatusCode::Busy;
    }
    mdns_instance_name_set("PULP OS");
    s_state.inited = true;
    ESP_LOGI(kTag, "hostname \"%s\" set (pulp.local)", kMdnsHost);
    return StatusCode::Ok;
}

StatusCode MdnsAnnounce(uint16_t port) {
    AssertOwner();
    if (port == 0) {
        port = 80;
    }
    const StatusCode init = MdnsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    // Only advertise once WiFi is up: pulp.local is useless (and a stale
    // record is harmful) without a link. The caller (serve.start) runs on
    // the owner after httpd_start, so WifiStatus() is a cheap precheck.
    if (WifiStatus() != kWifiUp) {
        ESP_LOGI(kTag, "announce deferred (wifi not up)");
        return StatusCode::Ok;
    }
    if (s_state.announced) {
        if (s_state.port == port) {
            return StatusCode::Ok;  // already announced on this port
        }
        // Port changed: remove the old service before re-adding.
        mdns_service_remove("_http", "_tcp");
        s_state.announced = false;
    }
    if (mdns_service_add(nullptr, "_http", "_tcp", port, nullptr, 0) != ESP_OK) {
        ESP_LOGW(kTag, "mdns_service_add failed");
        return StatusCode::Busy;
    }
    s_state.announced = true;
    s_state.port = port;
    ESP_LOGI(kTag, "announced _http._tcp :%u (pulp.local)", port);
    return StatusCode::Ok;
}

StatusCode MdnsStop() {
    AssertOwner();
    if (s_browse_in_flight.load(std::memory_order_acquire)) {
        // A live mdns_query_ptr must not have the component freed under
        // it. Defer: MdnsOnBrowseDone applies the stop when the
        // completion is processed (worst case one browse window later).
        s_stop_deferred = true;
        ESP_LOGI(kTag, "stop deferred (browse in flight)");
        return StatusCode::Ok;
    }
    if (!s_state.inited) {
        return StatusCode::Ok;
    }
    if (s_state.announced) {
        mdns_service_remove("_http", "_tcp");
        s_state.announced = false;
    }
    mdns_free();
    s_state.inited = false;
    s_state.port = 0;
    ESP_LOGI(kTag, "stopped (pulp.local withdrawn)");
    return StatusCode::Ok;
}

uint8_t MdnsStatus() {
    return s_state.announced ? 1 : 0;
}

void MdnsHost(char *out, size_t cap) {
    snprintf(out, cap, "%s", kMdnsHost);
}

void MdnsUrl(char *out, size_t cap) {
    if (!s_state.announced) {
        snprintf(out, cap, "%s", "");
        return;
    }
    if (s_state.port == 80) {
        snprintf(out, cap, "http://%s.local", kMdnsHost);
    } else {
        snprintf(out, cap, "http://%s.local:%u", kMdnsHost,
                 static_cast<unsigned>(s_state.port));
    }
}

StatusCode MdnsBrowse() {
    AssertOwner();
    if (s_browse_in_flight.load(std::memory_order_acquire)) {
        return StatusCode::Busy;
    }
    // WiFi gate BEFORE MdnsInit: mdns_init itself fails without a live
    // netif, and the JS contract for a down link is a deterministic
    // immediate completion (count=0, err=1), not an init error.
    if (WifiStatus() != kWifiUp) {
        s_browse_count = 0;
        return PostModuleDone(ModuleId::Mdns, kDoneMdnsBrowse, 0, 1);
    }
    const StatusCode init = MdnsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    s_browse_count = 0;
    s_browse_in_flight.store(true, std::memory_order_release);
    // Priority below the owner (5), like http_worker; internal stack.
    if (xTaskCreatePinnedToCore(BrowseWorker, "mdns_browse", 3072, nullptr,
                                4, nullptr, 0) != pdPASS) {
        s_browse_in_flight.store(false, std::memory_order_release);
        return StatusCode::OutOfMemory;
    }
    ESP_LOGI(kTag, "browse %s._tcp (%u ms window)", kMdnsAppSvc,
             static_cast<unsigned>(kMdnsBrowseWindowMs));
    return StatusCode::Ok;
}

uint32_t MdnsResultCount() {
    return s_browse_in_flight.load(std::memory_order_acquire)
               ? 0
               : s_browse_count;
}

const char *MdnsResultName(uint32_t i) {
    return i < MdnsResultCount() ? s_browse[i].name : "";
}

const char *MdnsResultIndexUrl(uint32_t i) {
    return i < MdnsResultCount() ? s_browse[i].url : "";
}

void MdnsOnBrowseDone() {
    AssertOwner();
    if (s_stop_deferred) {
        s_stop_deferred = false;
        (void)MdnsStop();
    }
}

void FillMdnsSnapshot(MdnsSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->announced = s_state.announced ? 1 : 0;
    out->port = s_state.port;
    MdnsHost(out->host, sizeof(out->host));
    MdnsUrl(out->url, sizeof(out->url));
}

}  // namespace pulp
