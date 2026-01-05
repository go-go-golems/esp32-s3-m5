/*
 * Minimal WiFi STA bring-up (based on patterns used in tutorial 0017).
 */

#include "wifi_sta.h"

#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_sta_0029";

static EventGroupHandle_t s_ev = NULL;
static const EventBits_t BIT_GOT_IP = (1 << 0);

static int s_retry = 0;

static esp_err_t ensure_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init failed (%s), erasing...", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void ensure_netif_and_loop(void) {
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
}

static void log_sta_ip(esp_netif_t *sta) {
    if (!sta) return;
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(sta, &ip) == ESP_OK) {
        ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&ip.ip));
        ESP_LOGI(TAG, "browse: http://" IPSTR "/ (or /v1/health)", IP2STR(&ip.ip));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    esp_netif_t *sta = (esp_netif_t *)arg;
    (void)data;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA start: connecting...");
        ESP_ERROR_CHECK(esp_wifi_connect());
        return;
    }

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "STA disconnected");
        if (s_retry < CONFIG_TUTORIAL_0029_WIFI_MAX_RETRY) {
            s_retry++;
            ESP_LOGI(TAG, "retrying (%d/%d)", s_retry, CONFIG_TUTORIAL_0029_WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "max retries reached");
        }
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_retry = 0;
        if (s_ev) {
            xEventGroupSetBits(s_ev, BIT_GOT_IP);
        }
        log_sta_ip(sta);
        return;
    }
}

esp_err_t wifi_sta_start(void) {
    if (CONFIG_TUTORIAL_0029_WIFI_SSID[0] == '\0') {
        ESP_LOGE(TAG, "WiFi SSID is empty; set it via menuconfig (Tutorial 0029: Mock Zigbee HTTP hub)");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_ERROR_CHECK(ensure_nvs());
    ensure_netif_and_loop();

    if (!s_ev) {
        s_ev = xEventGroupCreate();
    }

    esp_netif_t *sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, sta));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, sta));

    wifi_config_t sta_cfg = {0};
    strlcpy((char *)sta_cfg.sta.ssid, CONFIG_TUTORIAL_0029_WIFI_SSID, sizeof(sta_cfg.sta.ssid));
    strlcpy((char *)sta_cfg.sta.password, CONFIG_TUTORIAL_0029_WIFI_PASSWORD, sizeof(sta_cfg.sta.password));
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_cfg.sta.pmf_cfg.capable = true;
    sta_cfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    const TickType_t to = pdMS_TO_TICKS(CONFIG_TUTORIAL_0029_WIFI_CONNECT_TIMEOUT_MS);
    const EventBits_t bits = xEventGroupWaitBits(s_ev, BIT_GOT_IP, pdFALSE, pdFALSE, to);
    if ((bits & BIT_GOT_IP) == 0) {
        ESP_LOGW(TAG, "no IP within %d ms (continuing anyway)", CONFIG_TUTORIAL_0029_WIFI_CONNECT_TIMEOUT_MS);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

