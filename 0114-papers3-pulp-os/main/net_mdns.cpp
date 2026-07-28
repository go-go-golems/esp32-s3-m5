#include "net_mdns.h"

#include <cstring>

#include "esp_log.h"

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

void FillMdnsSnapshot(MdnsSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->announced = s_state.announced ? 1 : 0;
    out->port = s_state.port;
    MdnsHost(out->host, sizeof(out->host));
    MdnsUrl(out->url, sizeof(out->url));
}

}  // namespace pulp
