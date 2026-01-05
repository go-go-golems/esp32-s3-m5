/*
 * esp_console REPL for configuring WiFi STA credentials at runtime.
 */

#include "wifi_console.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lwip/inet.h"

#include "hub_bus.h"
#include "hub_http.h"
#include "hub_pb.h"
#include "hub_stream.h"
#include "hub_types.h"

#include "wifi_sta.h"

static const char *TAG = "wifi_console_0029";

static const char *authmode_to_str(uint8_t authmode_u8) {
    const wifi_auth_mode_t m = (wifi_auth_mode_t)authmode_u8;
    switch (m) {
        case WIFI_AUTH_OPEN:
            return "OPEN";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2-ENT";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI";
        default:
            return "?";
    }
}

static void print_usage(void) {
    printf("usage:\n");
    printf("  wifi status\n");
    printf("  wifi scan [max]\n");
    printf("  wifi set <ssid> <password> [save]\n");
    printf("  wifi set --ssid <ssid> --pass <password> [--save]\n");
    printf("  wifi connect\n");
    printf("  wifi disconnect\n");
    printf("  wifi clear\n");
}

static bool try_parse_int(const char *s, int *out) {
    if (!s || !*s || !out) return false;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static void hub_print_usage(void) {
    printf("usage:\n");
    printf("  hub seed\n");
    printf("  hub stream status\n");
    printf("  hub pb status\n");
    printf("  hub pb on\n");
    printf("  hub pb off\n");
    printf("  hub pb last\n");
}

static esp_err_t hub_post_add(const char *name, hub_device_type_t type, uint32_t caps, uint32_t *out_id) {
    if (out_id) *out_id = 0;
    esp_event_loop_handle_t loop = hub_bus_get_loop();
    if (!loop) return ESP_ERR_INVALID_STATE;

    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_device_t));
    if (!q) return ESP_ERR_NO_MEM;

    hub_cmd_device_add_t cmd = {
        .hdr = {.req_id = (uint32_t)esp_timer_get_time(), .reply_q = q},
        .type = type,
        .caps = caps,
    };
    strlcpy(cmd.name, name ? name : "", sizeof(cmd.name));

    esp_err_t err = esp_event_post_to(loop, HUB_EVT, HUB_CMD_DEVICE_ADD, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vQueueDelete(q);
        return err;
    }

    hub_reply_device_t rep = {0};
    if (xQueueReceive(q, &rep, pdMS_TO_TICKS(500)) != pdTRUE) {
        vQueueDelete(q);
        return ESP_ERR_TIMEOUT;
    }
    vQueueDelete(q);

    if (rep.status != ESP_OK) return rep.status;
    if (out_id) *out_id = rep.device_id;
    return ESP_OK;
}

static esp_err_t hub_post_interview(uint32_t device_id) {
    esp_event_loop_handle_t loop = hub_bus_get_loop();
    if (!loop) return ESP_ERR_INVALID_STATE;

    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_status_t));
    if (!q) return ESP_ERR_NO_MEM;

    hub_cmd_device_interview_t cmd = {
        .hdr = {.req_id = (uint32_t)esp_timer_get_time(), .reply_q = q},
        .device_id = device_id,
    };

    esp_err_t err = esp_event_post_to(loop, HUB_EVT, HUB_CMD_DEVICE_INTERVIEW, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vQueueDelete(q);
        return err;
    }

    hub_reply_status_t rep = {0};
    if (xQueueReceive(q, &rep, pdMS_TO_TICKS(500)) != pdTRUE) {
        vQueueDelete(q);
        return ESP_ERR_TIMEOUT;
    }
    vQueueDelete(q);
    return rep.status;
}

static int cmd_hub(int argc, char **argv) {
    if (argc < 2) {
        hub_print_usage();
        return 1;
    }

    if (strcmp(argv[1], "seed") == 0) {
        uint32_t plug_id = 0;
        uint32_t bulb_id = 0;
        uint32_t temp_id = 0;

        esp_err_t err = hub_post_add("desk", HUB_DEVICE_PLUG, 0, &plug_id);
        if (err == ESP_OK) err = hub_post_add("lamp", HUB_DEVICE_BULB, 0, &bulb_id);
        if (err == ESP_OK) err = hub_post_add("t1", HUB_DEVICE_TEMP_SENSOR, 0, &temp_id);
        if (err != ESP_OK) {
            printf("seed failed: %s\n", esp_err_to_name(err));
            return 1;
        }

        // Interview to populate type-based caps and trigger more event traffic.
        (void)hub_post_interview(plug_id);
        (void)hub_post_interview(bulb_id);
        (void)hub_post_interview(temp_id);

        printf("seeded devices: plug=%" PRIu32 " bulb=%" PRIu32 " temp=%" PRIu32 "\n", plug_id, bulb_id, temp_id);
        return 0;
    }

    if (strcmp(argv[1], "stream") == 0) {
        if (argc >= 3 && strcmp(argv[2], "status") == 0) {
            uint32_t drops = 0;
            uint32_t enc_fail = 0;
            uint32_t send_fail = 0;
            hub_stream_get_stats(&drops, &enc_fail, &send_fail);
            printf("clients=%zu drops=%" PRIu32 " enc_fail=%" PRIu32 " send_fail=%" PRIu32 "\n",
                   hub_http_events_client_count(),
                   drops,
                   enc_fail,
                   send_fail);
            return 0;
        }
        hub_print_usage();
        return 1;
    }

    if (strcmp(argv[1], "pb") == 0) {
        if (argc < 3 || strcmp(argv[2], "status") == 0) {
            bool enabled = false;
            bool have_last = false;
            hub_pb_get_status(&enabled, &have_last);
            printf("pb_capture=%s last=%s\n", enabled ? "on" : "off", have_last ? "yes" : "no");
            return 0;
        }
        if (strcmp(argv[2], "on") == 0) {
            hub_pb_set_capture(true);
            printf("ok\n");
            return 0;
        }
        if (strcmp(argv[2], "off") == 0) {
            hub_pb_set_capture(false);
            printf("ok\n");
            return 0;
        }
        if (strcmp(argv[2], "last") == 0) {
            uint8_t buf[512];
            size_t n = 0;
            esp_err_t err = hub_pb_encode_last(buf, sizeof(buf), &n);
            if (err == ESP_ERR_INVALID_STATE) {
                printf("no last protobuf event (turn on capture: hub pb on)\n");
                return 1;
            }
            if (err == ESP_ERR_NO_MEM) {
                printf("event too large for buffer (%zu)\n", sizeof(buf));
                return 1;
            }
            if (err != ESP_OK) {
                printf("encode failed: %s\n", esp_err_to_name(err));
                return 1;
            }

            printf("len=%zu\n", n);
            for (size_t i = 0; i < n; i++) {
                if (i % 16 == 0) printf("%04zx: ", i);
                printf("%02x ", buf[i]);
                if (i % 16 == 15 || i + 1 == n) printf("\n");
            }
            return 0;
        }

        hub_print_usage();
        return 1;
    }

    hub_print_usage();
    return 1;
}

static int cmd_wifi(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        wifi_sta_status_t st = {};
        esp_err_t err = hub_wifi_get_status(&st);
        if (err != ESP_OK) {
            printf("wifi status: %s\n", esp_err_to_name(err));
            return 1;
        }
        const char *state = "?";
        switch (st.state) {
            case WIFI_STA_STATE_UNINIT:
                state = "UNINIT";
                break;
            case WIFI_STA_STATE_IDLE:
                state = "IDLE";
                break;
            case WIFI_STA_STATE_CONNECTING:
                state = "CONNECTING";
                break;
            case WIFI_STA_STATE_CONNECTED:
                state = "CONNECTED";
                break;
        }
        printf("state=%s ssid=%s saved=%s runtime=%s reason=%d ip=",
               state,
               st.ssid[0] ? st.ssid : "-",
               st.has_saved_creds ? "yes" : "no",
               st.has_runtime_creds ? "yes" : "no",
               st.last_disconnect_reason);
        if (st.ip4) {
            ip4_addr_t ip = {.addr = htonl(st.ip4)};
            printf(IPSTR "\n", IP2STR(&ip));
        } else {
            printf("-\n");
        }
        return 0;
    }

    if (strcmp(argv[1], "connect") == 0) {
        esp_err_t err = hub_wifi_connect();
        if (err != ESP_OK) {
            printf("wifi connect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("connect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "disconnect") == 0) {
        esp_err_t err = hub_wifi_disconnect();
        if (err != ESP_OK) {
            printf("wifi disconnect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("disconnect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        esp_err_t err = hub_wifi_clear_credentials();
        if (err != ESP_OK) {
            printf("wifi clear: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("cleared saved+runtime credentials\n");
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        const char *ssid = NULL;
        const char *pass = NULL;
        bool save = false;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--save") == 0 || strcmp(argv[i], "save") == 0) {
                save = true;
                continue;
            }
            if (strcmp(argv[i], "--ssid") == 0 && i + 1 < argc) {
                ssid = argv[++i];
                continue;
            }
            if (strcmp(argv[i], "--pass") == 0 && i + 1 < argc) {
                pass = argv[++i];
                continue;
            }
            if (!ssid) {
                ssid = argv[i];
                continue;
            }
            if (!pass) {
                pass = argv[i];
                continue;
            }
        }

        if (!ssid || ssid[0] == '\0') {
            printf("wifi set: missing ssid\n");
            print_usage();
            return 1;
        }
        if (!pass) pass = "";

        esp_err_t err = hub_wifi_set_credentials(ssid, pass, save);
        if (err != ESP_OK) {
            printf("wifi set: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("credentials set (ssid=%s%s)\n", ssid, save ? ", saved" : "");
        return 0;
    }

    if (strcmp(argv[1], "scan") == 0) {
        int max = 20;
        if (argc >= 3) {
            int v = 0;
            if (!try_parse_int(argv[2], &v) || v <= 0 || v > 200) {
                printf("wifi scan: invalid max: %s\n", argv[2]);
                return 1;
            }
            max = v;
        }

        wifi_sta_scan_entry_t *entries = calloc((size_t)max, sizeof(*entries));
        if (!entries) {
            printf("wifi scan: no mem\n");
            return 1;
        }
        size_t n = 0;
        esp_err_t err = hub_wifi_scan(entries, (size_t)max, &n);
        if (err != ESP_OK) {
            free(entries);
            printf("wifi scan: %s\n", esp_err_to_name(err));
            return 1;
        }

        printf("found %zu networks (showing up to %d):\n", n, max);
        for (size_t i = 0; i < n; i++) {
            printf("%3zu: ch=%u rssi=%d auth=%s ssid=%s\n",
                   i,
                   entries[i].channel,
                   entries[i].rssi,
                   authmode_to_str(entries[i].authmode),
                   entries[i].ssid[0] ? entries[i].ssid : "<hidden>");
        }
        free(entries);
        return 0;
    }

    print_usage();
    return 1;
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {0};
    cmd.command = "wifi";
    cmd.help = "WiFi STA config: wifi status|scan|set|connect|disconnect|clear";
    cmd.func = &cmd_wifi;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    esp_console_cmd_t hub_cmd = {0};
    hub_cmd.command = "hub";
    hub_cmd.help = "Hub debug: hub seed, hub stream status, hub pb on|off|status|last";
    hub_cmd.func = &cmd_hub;
    ESP_ERROR_CHECK(esp_console_cmd_register(&hub_cmd));
}

void wifi_console_start(void) {
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "hub> ";

    esp_err_t err = ESP_ERR_NOT_SUPPORTED;

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    const char *backend = "USB Serial/JTAG";
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl);
    const char *backend = "USB CDC";
#elif CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    err = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    const char *backend = "UART";
#else
    const char *backend = "none";
#endif

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_console is not available (backend=%s, err=%s)", backend, esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over %s (try: help, wifi status)", backend);

#if CONFIG_TUTORIAL_0029_QUIET_LOGS_WHILE_CONSOLE
    // Keep the interactive prompt usable by reducing non-console logging noise.
    esp_log_level_set("hub_http_0029", ESP_LOG_WARN);
    esp_log_level_set("hub_bus_0029", ESP_LOG_WARN);
    esp_log_level_set("hub_registry_0029", ESP_LOG_WARN);
    esp_log_level_set("hub_sim_0029", ESP_LOG_WARN);
#endif
}
