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
#include "esp_wifi.h"
#include "lwip/inet.h"

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
