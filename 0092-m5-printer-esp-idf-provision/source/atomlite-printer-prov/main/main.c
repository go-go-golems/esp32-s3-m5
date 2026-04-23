/*
 * M5Stack ATOM Lite ESP-IDF BLE provisioning firmware for the ATOM Thermal Printer Kit.
 *
 * Hardware target:
 * - Controller: ATOM Lite, ESP32-PICO-D4, 4 MB flash
 * - USB: FTDI UART bridge, typically /dev/ttyUSB0
 * - Printer: UART2, TX GPIO23, RX GPIO33, 9600 8N1
 * - Button: GPIO39 active-low, hold five seconds for factory reset
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "protocomm_security.h"
#include "protocomm_security1.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

#include "app_button.h"
#include "app_printer.h"

static const char *TAG = "atomlite-prov";

static const char *PROV_POP = "12345678";
static const char *PROV_PREFIX = "M5PRN_";
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;

static EventGroupHandle_t s_wifi_event_group;

static void factory_reset_task(void *arg)
{
    (void)arg;
    while (true) {
        if (app_button_pressed_for(5000)) {
            ESP_LOGW(TAG, "Factory reset requested: erasing NVS and rebooting");
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP_ERROR_CHECK(nvs_flash_erase());
            esp_restart();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void make_service_name(char *service_name, size_t service_name_len)
{
    uint8_t mac[6] = {0};
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    snprintf(service_name, service_name_len, "%s%02X%02X%02X",
             PROV_PREFIX, mac[3], mac[4], mac[5]);
}

static void log_provisioning_payload(const char *service_name)
{
    ESP_LOGI(TAG, "Provision with Espressif's 'ESP BLE Provisioning' app:");
    ESP_LOGI(TAG, "  Transport : BLE");
    ESP_LOGI(TAG, "  Device    : %s", service_name);
    ESP_LOGI(TAG, "  Security  : Security 1");
    ESP_LOGI(TAG, "  PoP       : %s", PROV_POP);
    ESP_LOGI(TAG, "  QR data   : {\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}",
             service_name, PROV_POP);
}

static void start_wifi_station(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "BLE provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            const wifi_sta_config_t *cfg = (const wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received WiFi credentials for SSID '%s'", (const char *)cfg->ssid);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            const wifi_prov_sta_fail_reason_t *reason = (const wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioned WiFi connection failed: %s",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "authentication failed" : "AP not found");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioned WiFi credentials connected successfully");
            break;
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning ended; releasing provisioning manager resources");
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi station started; connecting");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected; reconnecting");
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            esp_wifi_connect();
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected, IP=" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
        switch (event_id) {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI(TAG, "BLE provisioning client connected");
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            ESP_LOGI(TAG, "BLE provisioning client disconnected");
            break;
        default:
            break;
        }
    } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI(TAG, "Provisioning security session established");
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE(TAG, "Provisioning PoP mismatch");
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGE(TAG, "Provisioning security parameters invalid");
            break;
        default:
            break;
        }
    }
}

static void start_ble_provisioning(void)
{
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (provisioned) {
        ESP_LOGI(TAG, "Device already provisioned; starting station mode");
        wifi_prov_mgr_deinit();
        start_wifi_station();
        return;
    }

    char service_name[16] = {0};
    make_service_name(service_name, sizeof(service_name));

    uint8_t custom_service_uuid[] = {
        0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
    };
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

    const wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    const wifi_prov_security1_params_t *security_params = PROV_POP;
    const char *service_key = NULL;

    ESP_LOGI(TAG, "Device not provisioned; starting BLE provisioning service");
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security,
                                                     (const void *)security_params,
                                                     service_name,
                                                     service_key));
    log_provisioning_payload(service_name);
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "M5 Printer ATOM Lite ESP-IDF BLE Provisioning");
    ESP_LOGI(TAG, "Target: ESP32-PICO-D4 / idf.py set-target esp32");
    ESP_LOGI(TAG, "============================================");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(s_wifi_event_group == NULL ? ESP_ERR_NO_MEM : ESP_OK);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, app_event_handler, NULL));

    ESP_ERROR_CHECK(app_button_init());
    xTaskCreate(factory_reset_task, "factory_reset", 3072, NULL, 5, NULL);

    ESP_ERROR_CHECK(app_printer_init());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));

    start_ble_provisioning();

    bool printed_status = false;
    while (true) {
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(1000));
        if ((bits & WIFI_CONNECTED_BIT) != 0 && !printed_status) {
            esp_netif_ip_info_t ip_info;
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            char ip_str[16] = "unknown";
            if (netif != NULL && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
            }
            ESP_LOGI(TAG, "Printing WiFi status receipt");
            if (app_printer_print_wifi_status(ip_str) != ESP_OK) {
                ESP_LOGW(TAG, "Status receipt print failed; continuing firmware loop");
            }
            printed_status = true;
        }

        if ((bits & WIFI_CONNECTED_BIT) == 0) {
            printed_status = false;
        }
    }
}
