#include "esp_log.h"

#include "esp_zigbee_core.h"
#include "zdo/esp_zigbee_zdo_common.h"

#include "gw_bus.h"
#include "gw_types.h"

static const char *TAG = "gw_zb_sig_0031";

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    if (!signal_struct || !signal_struct->p_app_signal) return;

    const uint32_t *p = signal_struct->p_app_signal;
    const esp_err_t err_status = signal_struct->esp_err_status;
    const esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)(*p);

    switch (sig_type) {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            ESP_LOGI(TAG, "ZB signal: SKIP_STARTUP status=%s", esp_err_to_name(err_status));
            break;

        case ESP_ZB_BDB_SIGNAL_FORMATION:
            ESP_LOGI(TAG, "ZB signal: FORMATION status=%s", esp_err_to_name(err_status));
            break;

        case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
            const esp_zb_zdo_signal_device_annce_params_t *ann =
                (const esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params((uint32_t *)p);
            if (!ann) break;

            gw_evt_zb_device_annce_t ev = {0};
            ev.short_addr = ann->device_short_addr;
            ev.capability = ann->capability;
            for (int i = 0; i < 8; i++) ev.ieee_addr[i] = ann->ieee_addr[i];

            esp_event_loop_handle_t loop = gw_bus_get_loop();
            if (loop) {
                (void)esp_event_post_to(loop, GW_EVT, GW_EVT_ZB_DEVICE_ANNCE, &ev, sizeof(ev), 0);
            }
            ESP_LOGI(TAG, "ZB device announce: short=0x%04x cap=0x%02x",
                     (unsigned)ann->device_short_addr,
                     (unsigned)ann->capability);
            break;
        }

        case ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION: {
            const esp_zb_zdo_signal_leave_indication_params_t *params =
                (const esp_zb_zdo_signal_leave_indication_params_t *)esp_zb_app_signal_get_params((uint32_t *)p);
            if (!params) break;
            ESP_LOGI(TAG, "ZB leave indication: short=0x%04x rejoin=%u ieee=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                     (unsigned)params->short_addr,
                     (unsigned)params->rejoin,
                     params->device_addr[7],
                     params->device_addr[6],
                     params->device_addr[5],
                     params->device_addr[4],
                     params->device_addr[3],
                     params->device_addr[2],
                     params->device_addr[1],
                     params->device_addr[0]);
            break;
        }

        case ESP_ZB_ZDO_SIGNAL_DEVICE_UPDATE: {
            const esp_zb_zdo_signal_device_update_params_t *u =
                (const esp_zb_zdo_signal_device_update_params_t *)esp_zb_app_signal_get_params((uint32_t *)p);
            if (!u) break;
            ESP_LOGI(TAG, "ZB device update: status=0x%02x short=0x%04x ieee=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                     (unsigned)u->status,
                     (unsigned)u->short_addr,
                     u->long_addr[7],
                     u->long_addr[6],
                     u->long_addr[5],
                     u->long_addr[4],
                     u->long_addr[3],
                     u->long_addr[2],
                     u->long_addr[1],
                     u->long_addr[0]);
            break;
        }

        case ESP_ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED: {
            const esp_zb_zdo_signal_device_authorized_params_t *a =
                (const esp_zb_zdo_signal_device_authorized_params_t *)esp_zb_app_signal_get_params((uint32_t *)p);
            if (!a) break;
            ESP_LOGI(TAG,
                     "ZB device authorized: type=0x%02x status=0x%02x short=0x%04x ieee=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                     (unsigned)a->authorization_type,
                     (unsigned)a->authorization_status,
                     (unsigned)a->short_addr,
                     a->long_addr[7],
                     a->long_addr[6],
                     a->long_addr[5],
                     a->long_addr[4],
                     a->long_addr[3],
                     a->long_addr[2],
                     a->long_addr[1],
                     a->long_addr[0]);
            break;
        }

        case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS: {
            const uint8_t *sec = (const uint8_t *)esp_zb_app_signal_get_params((uint32_t *)p);
            if (!sec) break;
            const gw_evt_zb_permit_join_status_t ev = {.seconds = *sec};
            esp_event_loop_handle_t loop = gw_bus_get_loop();
            if (loop) {
                (void)esp_event_post_to(loop, GW_EVT, GW_EVT_ZB_PERMIT_JOIN_STATUS, &ev, sizeof(ev), 0);
            }
            ESP_LOGI(TAG, "ZB permit-join status: seconds=%u", (unsigned)*sec);
            break;
        }

        default:
            ESP_LOGI(TAG, "ZB signal: 0x%02x status=%s", (unsigned)sig_type, esp_err_to_name(err_status));
            break;
    }
}
