#include "wifi_mgr.h"

#include <atomic>
#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

namespace {
const char *TAG = "wifi_mgr";
constexpr int kMaxRetries = 6;

std::atomic<WifiState> s_state{WifiState::kIdle};
std::atomic<uint32_t> s_sta_ip{0};
int s_retries = 0;
esp_netif_t *s_sta_netif = nullptr;
esp_netif_t *s_ap_netif = nullptr;

void start_ap_mode() {
    ESP_LOGW(TAG, "starting provisioning AP %s", kApSsid);
    wifi_config_t ap_cfg = {};
    strlcpy(reinterpret_cast<char *>(ap_cfg.ap.ssid), kApSsid, sizeof(ap_cfg.ap.ssid));
    strlcpy(reinterpret_cast<char *>(ap_cfg.ap.password), kApPass, sizeof(ap_cfg.ap.password));
    ap_cfg.ap.ssid_len = strlen(kApSsid);
    ap_cfg.ap.channel = 1;
    ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_cfg.ap.max_connection = 4;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    s_state = WifiState::kApMode;
}

void on_wifi_event(void *, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_sta_ip = 0;
        if (s_state == WifiState::kApMode) return;
        if (s_retries < kMaxRetries) {
            s_retries++;
            ESP_LOGI(TAG, "STA disconnected, retry %d/%d", s_retries, kMaxRetries);
            esp_wifi_connect();
        } else {
            start_ap_mode();
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto *ev = static_cast<ip_event_got_ip_t *>(data);
        s_sta_ip = ev->ip_info.ip.addr;
        s_retries = 0;
        s_state = WifiState::kConnected;
        ESP_LOGI(TAG, "connected, ip=" IPSTR, IP2STR(&ev->ip_info.ip));
    }
}
} // namespace

bool wifi_mgr_start(const char *ssid, const char *pass) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, nullptr));

    if (ssid == nullptr || ssid[0] == '\0') {
        start_ap_mode();
        ESP_ERROR_CHECK(esp_wifi_start());
        return true;
    }

    wifi_config_t sta_cfg = {};
    strlcpy(reinterpret_cast<char *>(sta_cfg.sta.ssid), ssid, sizeof(sta_cfg.sta.ssid));
    strlcpy(reinterpret_cast<char *>(sta_cfg.sta.password), pass ? pass : "",
            sizeof(sta_cfg.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    s_state = WifiState::kConnecting;
    ESP_ERROR_CHECK(esp_wifi_start());
    return true;
}

WifiState wifi_mgr_state() { return s_state; }

uint32_t wifi_mgr_sta_ip() { return s_sta_ip; }
