/*
 * ESP32-S3 tutorial 0006:
 * WiFi station + esp_http_client (HTTP GET).
 */

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_http_client";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt == NULL) {
        return ESP_OK;
    }

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGW(TAG, "http event: error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "http event: connected");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "http event: header sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        if (evt->header_key && evt->header_value) {
            ESP_LOGI(TAG, "header: %s: %s", evt->header_key, evt->header_value);
        }
        break;
    case HTTP_EVENT_ON_DATA: {
        const int cap = 256;
        int n = evt->data_len;
        if (n > cap) {
            n = cap;
        }

        // Note: evt->data is not null-terminated. Log only a small prefix.
        if (evt->data && n > 0) {
            ESP_LOGI(TAG, "body chunk: len=%d prefix='%.*s'%s",
                     evt->data_len,
                     n,
                     (const char *)evt->data,
                     (evt->data_len > cap) ? "..." : "");
        } else {
            ESP_LOGI(TAG, "body chunk: len=%d", evt->data_len);
        }
        break;
    }
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "http event: finished");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "http event: disconnected");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "http event: redirect");
        break;
    default:
        break;
    }

    return ESP_OK;
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data) {
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START -> esp_wifi_connect()");
        ESP_ERROR_CHECK(esp_wifi_connect());
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disc = (const wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED reason=%d", disc ? disc->reason : -1);

        if (s_retry_num < CONFIG_TUTORIAL_0006_WIFI_MAX_RETRY) {
            s_retry_num++;
            ESP_LOGI(TAG, "retrying (%d/%d)", s_retry_num, CONFIG_TUTORIAL_0006_WIFI_MAX_RETRY);
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        s_retry_num = 0;
        if (event) {
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: " IPSTR, IP2STR(&event->ip_info.ip));
        } else {
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
        }
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        return;
    }
}

static bool wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "failed to create event group");
        return false;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, CONFIG_TUTORIAL_0006_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, CONFIG_TUTORIAL_0006_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi started; waiting for connect (SSID='%s')", CONFIG_TUTORIAL_0006_WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected");
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "failed to connect after %d retries", CONFIG_TUTORIAL_0006_WIFI_MAX_RETRY);
        return false;
    } else {
        ESP_LOGW(TAG, "unexpected event group bits: 0x%" PRIx32, (uint32_t)bits);
        return false;
    }

    // Note: for a real app, keep handlers registered; this tutorial keeps them to show the event loop.
}

static void http_client_task(void *arg) {
    (void)arg;

    while (1) {
        const char *url = CONFIG_TUTORIAL_0006_HTTP_URL;
        if (url == NULL || strlen(url) == 0) {
            ESP_LOGW(TAG, "HTTP URL is empty; set it via: idf.py menuconfig -> Tutorial 0006: WiFi + HTTP Client");
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0006_HTTP_PERIOD_MS));
            continue;
        }

        ESP_LOGI(TAG, "fetching: %s", url);

        esp_http_client_config_t config = {
            .url = url,
            .event_handler = http_event_handler,
            .timeout_ms = CONFIG_TUTORIAL_0006_HTTP_TIMEOUT_MS,
        };

        // Optional: attach the ESP-IDF certificate bundle for HTTPS URLs.
        // Note: TLS can still fail if the system time is not set (cert validity checks).
        if (strncmp(url, "https://", 8) == 0) {
            config.crt_bundle_attach = esp_crt_bundle_attach;
        }

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "esp_http_client_init failed");
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0006_HTTP_PERIOD_MS));
            continue;
        }

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            int len = esp_http_client_get_content_length(client);
            ESP_LOGI(TAG, "http status=%d content_length=%d", status, len);
        } else {
            ESP_LOGE(TAG, "esp_http_client_perform failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0006_HTTP_PERIOD_MS));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init failed (%s), erasing...", esp_err_to_name(ret));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    if (strlen(CONFIG_TUTORIAL_0006_WIFI_SSID) == 0) {
        ESP_LOGW(TAG, "SSID is empty; set it via: idf.py menuconfig -> Tutorial 0006: WiFi + HTTP Client");
    }

    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "wifi failed; not starting HTTP client");
        return;
    }

    xTaskCreate(http_client_task, "http_client_task", 6144, NULL, 5, NULL);
}
