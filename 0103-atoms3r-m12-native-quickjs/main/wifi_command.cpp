// wifi_command.cpp — console diagnostics/provisioning for native AtomS3R WiFi.
#include "wifi_command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "lwip/inet.h"

#include "wifi_app.h"

namespace {
constexpr const char *kTag = "0103_wifi_cmd";

const char *state_to_str(wifi_app_state_t state)
{
    switch (state) {
        case WIFI_APP_STATE_UNINIT:
            return "UNINIT";
        case WIFI_APP_STATE_IDLE:
            return "IDLE";
        case WIFI_APP_STATE_CONNECTING:
            return "CONNECTING";
        case WIFI_APP_STATE_CONNECTED:
            return "CONNECTED";
        default:
            return "?";
    }
}

const char *authmode_to_str(uint8_t authmode_u8)
{
    const wifi_auth_mode_t mode = (wifi_auth_mode_t)authmode_u8;
    switch (mode) {
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
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3";
        default:
            return "?";
    }
}

void print_ip4_line(const char *label, uint32_t ip4_host_order)
{
    if (ip4_host_order == 0) {
        printf("%s=-\n", label);
        return;
    }
    ip4_addr_t ip = {.addr = htonl(ip4_host_order)};
    printf("%s=" IPSTR "\n", label, IP2STR(&ip));
}

void print_usage()
{
    printf("usage:\n");
    printf("  wifi start\n");
    printf("  wifi status\n");
    printf("  wifi scan [max]\n");
    printf("  wifi set <ssid> <password> [save]\n");
    printf("  wifi set --ssid <ssid> --pass <password> [--save]\n");
    printf("  wifi save\n");
    printf("  wifi connect\n");
    printf("  wifi disconnect\n");
    printf("  wifi clear\n");
}

bool parse_int(const char *s, int *out)
{
    if (!s || !*s || !out) {
        return false;
    }
    char *end = nullptr;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') {
        return false;
    }
    *out = (int)v;
    return true;
}

int cmd_wifi(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "start") == 0) {
        esp_err_t err = wifi_app_start();
        printf("wifi start: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        wifi_app_status_t st = {};
        esp_err_t err = wifi_app_get_status(&st);
        if (err != ESP_OK) {
            printf("wifi status: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("state=%s ssid=%s saved=%s runtime=%s reason=%d\n",
               state_to_str(st.state),
               st.ssid[0] ? st.ssid : "-",
               st.has_saved_creds ? "yes" : "no",
               st.has_runtime_creds ? "yes" : "no",
               st.last_disconnect_reason);
        print_ip4_line("sta_ip", st.sta_ip4);
        print_ip4_line("ap_ip", st.ap_ip4);
        return 0;
    }

    if (strcmp(argv[1], "connect") == 0 || strcmp(argv[1], "join") == 0) {
        esp_err_t err = wifi_app_connect();
        if (err != ESP_OK) {
            printf("wifi connect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("connect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "disconnect") == 0) {
        esp_err_t err = wifi_app_disconnect();
        if (err != ESP_OK) {
            printf("wifi disconnect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("disconnect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        esp_err_t err = wifi_app_clear_credentials();
        if (err != ESP_OK) {
            printf("wifi clear: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("cleared saved+runtime credentials\n");
        return 0;
    }

    if (strcmp(argv[1], "save") == 0) {
        esp_err_t err = wifi_app_save_credentials();
        if (err != ESP_OK) {
            printf("wifi save: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("saved runtime credentials\n");
        (void)wifi_app_connect();
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        const char *ssid = nullptr;
        const char *pass = nullptr;
        bool save = false;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "save") == 0 || strcmp(argv[i], "--save") == 0) {
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
        if (!pass) {
            pass = "";
        }

        esp_err_t err = wifi_app_set_credentials(ssid, pass, save);
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
            int parsed = 0;
            if (!parse_int(argv[2], &parsed) || parsed <= 0 || parsed > 64) {
                printf("wifi scan: invalid max: %s\n", argv[2]);
                return 1;
            }
            max = parsed;
        }
        wifi_scan_entry_t *entries = (wifi_scan_entry_t *)calloc((size_t)max, sizeof(*entries));
        if (!entries) {
            printf("wifi scan: no memory\n");
            return 1;
        }
        size_t n = 0;
        esp_err_t err = wifi_app_scan(entries, (size_t)max, &n);
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
}  // namespace

void register_wifi_commands(void)
{
    esp_console_cmd_t cmd = {};
    cmd.command = "wifi";
    cmd.help = "WiFi STA config: wifi start|status|scan|set|save|connect|disconnect|clear";
    cmd.func = &cmd_wifi;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(kTag, "registered WiFi console commands");
}
