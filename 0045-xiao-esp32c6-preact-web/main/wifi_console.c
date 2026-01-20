/*
 * MO-033 / 0044: esp_console REPL for configuring Wi-Fi STA at runtime.
 *
 * Pattern and UX based on 0029-mock-zigbee-http-hub/main/wifi_console.c.
 */

#include "wifi_console.h"

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

#include "wifi_mgr.h"

static const char *TAG = "mo033_wifi_console";

static const char *authmode_to_str(uint8_t authmode_u8)
{
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

static void print_usage(void)
{
    printf("usage:\n");
    printf("  wifi status\n");
    printf("  wifi scan [max]\n");
    printf("  wifi join <index> <password> [--save]\n");
    printf("  wifi set <ssid> <password> [save]\n");
    printf("  wifi set --ssid <ssid> --pass <password> [--save]\n");
    printf("  wifi connect\n");
    printf("  wifi disconnect\n");
    printf("  wifi clear\n");
}

static bool try_parse_int(const char *s, int *out)
{
    if (!s || !*s || !out) return false;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static wifi_mgr_scan_entry_t s_last_scan[20];
static size_t s_last_scan_n = 0;

static int cmd_wifi(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        wifi_mgr_status_t st = {};
        const esp_err_t err = wifi_mgr_get_status(&st);
        if (err != ESP_OK) {
            printf("wifi status: %s\n", esp_err_to_name(err));
            return 1;
        }
        const char *state = "?";
        switch (st.state) {
            case WIFI_MGR_STATE_UNINIT:
                state = "UNINIT";
                break;
            case WIFI_MGR_STATE_IDLE:
                state = "IDLE";
                break;
            case WIFI_MGR_STATE_CONNECTING:
                state = "CONNECTING";
                break;
            case WIFI_MGR_STATE_CONNECTED:
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
        const esp_err_t err = wifi_mgr_connect();
        if (err != ESP_OK) {
            printf("wifi connect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("connect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "disconnect") == 0) {
        const esp_err_t err = wifi_mgr_disconnect();
        if (err != ESP_OK) {
            printf("wifi disconnect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("disconnect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        const esp_err_t err = wifi_mgr_clear_credentials();
        if (err != ESP_OK) {
            printf("wifi clear: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("cleared saved+runtime credentials\n");
        return 0;
    }

    if (strcmp(argv[1], "scan") == 0) {
        int max = (int)(sizeof(s_last_scan) / sizeof(s_last_scan[0]));
        if (argc >= 3) {
            int v = 0;
            if (!try_parse_int(argv[2], &v) || v <= 0 || v > 200) {
                printf("wifi scan: invalid max: %s\n", argv[2]);
                return 1;
            }
            if (v < max) max = v;
        }

        size_t n = 0;
        const esp_err_t err = wifi_mgr_scan(s_last_scan, (size_t)max, &n);
        if (err != ESP_OK) {
            printf("wifi scan: %s\n", esp_err_to_name(err));
            return 1;
        }

        s_last_scan_n = n;
        printf("found %zu networks (showing up to %d):\n", n, max);
        for (size_t i = 0; i < n; i++) {
            printf("%3zu: ch=%u rssi=%d auth=%s ssid=%s\n",
                   i,
                   s_last_scan[i].channel,
                   s_last_scan[i].rssi,
                   authmode_to_str(s_last_scan[i].authmode),
                   s_last_scan[i].ssid[0] ? s_last_scan[i].ssid : "<hidden>");
        }
        return 0;
    }

    if (strcmp(argv[1], "join") == 0) {
        if (argc < 4) {
            printf("wifi join: missing index/password\n");
            print_usage();
            return 1;
        }
        int idx = -1;
        if (!try_parse_int(argv[2], &idx) || idx < 0) {
            printf("wifi join: invalid index: %s\n", argv[2]);
            return 1;
        }
        const char *pass = argv[3];
        bool save = false;
        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "--save") == 0 || strcmp(argv[i], "save") == 0) {
                save = true;
            }
        }
        if (s_last_scan_n == 0) {
            printf("wifi join: no scan results; run `wifi scan` first\n");
            return 1;
        }
        if ((size_t)idx >= s_last_scan_n) {
            printf("wifi join: index out of range (have %zu)\n", s_last_scan_n);
            return 1;
        }
        const char *ssid = s_last_scan[idx].ssid;
        if (!ssid || ssid[0] == '\0') {
            printf("wifi join: selected SSID is empty/hidden; use `wifi set --ssid ...`\n");
            return 1;
        }

        esp_err_t err = wifi_mgr_set_credentials(ssid, pass ? pass : "", save);
        if (err != ESP_OK) {
            printf("wifi join: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("credentials set (ssid=%s%s)\n", ssid, save ? ", saved" : "");

        err = wifi_mgr_connect();
        if (err != ESP_OK) {
            printf("wifi connect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("connect requested\n");
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

        const esp_err_t err = wifi_mgr_set_credentials(ssid, pass, save);
        if (err != ESP_OK) {
            printf("wifi set: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("credentials set (ssid=%s%s)\n", ssid, save ? ", saved" : "");
        return 0;
    }

    print_usage();
    return 1;
}

static void register_commands(void)
{
    esp_console_cmd_t cmd = {0};
    cmd.command = "wifi";
    cmd.help = "Wi-Fi STA config: wifi status|scan|join|set|connect|disconnect|clear";
    cmd.func = &cmd_wifi;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void wifi_console_start(void)
{
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "c6> ";

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

    ESP_LOGI(TAG, "esp_console started over %s (try: help, wifi scan)", backend);
}
