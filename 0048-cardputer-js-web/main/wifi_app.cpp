/*
 * 0048: WiFi bring-up (SoftAP only for MVP).
 *
 * Patterned after: esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp
 */

#include "wifi_app.h"

#include <string.h>

#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char* TAG = "wifi_app_0048";

static bool s_started = false;
static esp_netif_t* s_ap_netif = nullptr;

esp_netif_t* wifi_app_get_ap_netif(void) {
  return s_ap_netif;
}

static esp_err_t ensure_nvs(void) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "nvs_flash_init failed (%s), erasing NVS and retrying", esp_err_to_name(err));
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  return err;
}

static void ensure_netif_and_event_loop(void) {
  esp_err_t err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_ERROR_CHECK(err);
  }
  err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_ERROR_CHECK(err);
  }
}

static void log_netif_ip(const char* label, esp_netif_t* netif) {
  if (!netif) return;
  esp_netif_ip_info_t ip = {};
  if (esp_netif_get_ip_info(netif, &ip) != ESP_OK) return;
  ESP_LOGI(TAG, "%s IP: " IPSTR, label, IP2STR(&ip.ip));
}

static void setup_softap_config(wifi_config_t* out_cfg) {
  wifi_config_t ap_cfg = {};
  const char* ssid = CONFIG_TUTORIAL_0048_WIFI_SOFTAP_SSID;
  const char* pass = CONFIG_TUTORIAL_0048_WIFI_SOFTAP_PASSWORD;

  memset(ap_cfg.ap.ssid, 0, sizeof(ap_cfg.ap.ssid));
  strncpy((char*)ap_cfg.ap.ssid, ssid, sizeof(ap_cfg.ap.ssid) - 1);
  ap_cfg.ap.ssid_len = (uint8_t)strlen((const char*)ap_cfg.ap.ssid);

  memset(ap_cfg.ap.password, 0, sizeof(ap_cfg.ap.password));
  strncpy((char*)ap_cfg.ap.password, pass, sizeof(ap_cfg.ap.password) - 1);
  ap_cfg.ap.channel = (uint8_t)CONFIG_TUTORIAL_0048_WIFI_SOFTAP_CHANNEL;
  ap_cfg.ap.max_connection = (uint8_t)CONFIG_TUTORIAL_0048_WIFI_SOFTAP_MAX_CONN;

  if (!pass || pass[0] == '\0') {
    ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
  } else {
    ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
  }

  *out_cfg = ap_cfg;
}

esp_err_t wifi_app_start(void) {
  if (s_started) return ESP_OK;

  esp_err_t err = ensure_nvs();
  if (err != ESP_OK) return err;

  ensure_netif_and_event_loop();

  s_ap_netif = esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

  wifi_config_t ap_cfg = {};
  setup_softap_config(&ap_cfg);
  ESP_LOGI(TAG, "starting SoftAP: ssid=%s channel=%d max_conn=%d",
           CONFIG_TUTORIAL_0048_WIFI_SOFTAP_SSID,
           CONFIG_TUTORIAL_0048_WIFI_SOFTAP_CHANNEL,
           CONFIG_TUTORIAL_0048_WIFI_SOFTAP_MAX_CONN);

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
  ESP_ERROR_CHECK(esp_wifi_start());

  log_netif_ip("SoftAP", s_ap_netif);
  ESP_LOGI(TAG, "browse (SoftAP): http://" IPSTR "/", 192, 168, 4, 1);

  s_started = true;
  return ESP_OK;
}

