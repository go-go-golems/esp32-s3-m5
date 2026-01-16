#include "console.h"

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
#include "esp_heap_caps.h"
#if CONFIG_SPIRAM
#include "esp_psram.h"
#endif
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lwip/inet.h"

#include "stream_client.h"
#include "wifi_sta.h"

static const char *TAG = "cam_console";

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

static void print_wifi_usage(void) {
    printf("usage:\n");
    printf("  wifi status\n");
    printf("  wifi scan [max]\n");
    printf("  wifi set <ssid> <password> [save]\n");
    printf("  wifi set --ssid <ssid> --pass <password> [--save]\n");
    printf("  wifi connect\n");
    printf("  wifi disconnect\n");
    printf("  wifi clear\n");
}

static void print_stream_usage(void) {
    printf("usage:\n");
    printf("  stream status\n");
    printf("  stream set host <host> [port] [save]\n");
    printf("  stream set port <port> [save]\n");
    printf("  stream start\n");
    printf("  stream stop\n");
    printf("  stream clear\n");
}

static void print_cam_usage(void) {
    printf("usage:\n");
    printf("  cam power <0|1>  (1=on, 0=off)\n");
    printf("  cam dump\n");
}

static void print_psram_usage(void) {
    printf("usage:\n");
    printf("  psram status\n");
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
        print_wifi_usage();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        cam_wifi_status_t st = {};
        esp_err_t err = cam_wifi_get_status(&st);
        if (err != ESP_OK) {
            printf("wifi status: %s\n", esp_err_to_name(err));
            return 1;
        }
        const char *state = "?";
        switch (st.state) {
            case CAM_WIFI_STATE_UNINIT:
                state = "UNINIT";
                break;
            case CAM_WIFI_STATE_IDLE:
                state = "IDLE";
                break;
            case CAM_WIFI_STATE_CONNECTING:
                state = "CONNECTING";
                break;
            case CAM_WIFI_STATE_CONNECTED:
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
        esp_err_t err = cam_wifi_connect();
        if (err != ESP_OK) {
            printf("wifi connect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("connect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "disconnect") == 0) {
        esp_err_t err = cam_wifi_disconnect();
        if (err != ESP_OK) {
            printf("wifi disconnect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("disconnect requested\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        esp_err_t err = cam_wifi_clear_credentials();
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
            print_wifi_usage();
            return 1;
        }
        if (!pass) pass = "";

        esp_err_t err = cam_wifi_set_credentials(ssid, pass, save);
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

        cam_wifi_scan_entry_t *entries = calloc((size_t)max, sizeof(*entries));
        if (!entries) {
            printf("wifi scan: no mem\n");
            return 1;
        }
        size_t n = 0;
        esp_err_t err = cam_wifi_scan(entries, (size_t)max, &n);
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

    print_wifi_usage();
    return 1;
}

static int cmd_stream(int argc, char **argv) {
    if (argc < 2) {
        print_stream_usage();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        stream_target_t target = {};
        stream_client_get_target(&target);
        printf("host=%s port=%u saved=%s runtime=%s streaming=%s ws=%s\n",
               target.host[0] ? target.host : "-",
               (unsigned)target.port,
               target.has_saved ? "yes" : "no",
               target.has_runtime ? "yes" : "no",
               stream_client_is_running() ? "on" : "off",
               stream_client_is_connected() ? "up" : "down");
        return 0;
    }

    if (strcmp(argv[1], "start") == 0) {
        stream_client_start();
        printf("stream start requested\n");
        return 0;
    }

    if (strcmp(argv[1], "stop") == 0) {
        stream_client_stop();
        printf("stream stopped\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        esp_err_t err = stream_client_clear_target();
        if (err != ESP_OK) {
            printf("stream clear: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("cleared saved stream target\n");
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        const char *host = NULL;
        int port = -1;
        bool save = false;

        if (argc >= 4 && strcmp(argv[2], "host") == 0) {
            host = argv[3];
            for (int i = 4; i < argc; i++) {
                if (strcmp(argv[i], "save") == 0 || strcmp(argv[i], "--save") == 0) {
                    save = true;
                    continue;
                }
                int v = 0;
                if (try_parse_int(argv[i], &v) && v > 0 && v <= 65535) {
                    port = v;
                }
            }
        } else if (argc >= 4 && strcmp(argv[2], "port") == 0) {
            int v = 0;
            if (!try_parse_int(argv[3], &v) || v <= 0 || v > 65535) {
                printf("stream set port: invalid port\n");
                return 1;
            }
            port = v;
            if (argc >= 5 && (strcmp(argv[4], "save") == 0 || strcmp(argv[4], "--save") == 0)) {
                save = true;
            }
        } else {
            print_stream_usage();
            return 1;
        }

        if (host) {
            if (port <= 0) {
                stream_target_t current = {};
                stream_client_get_target(&current);
                port = current.port;
            }
            esp_err_t err = stream_client_set_target(host, (uint16_t)port, save);
            if (err != ESP_OK) {
                printf("stream set: %s\n", esp_err_to_name(err));
                return 1;
            }
            printf("stream target set (host=%s port=%d%s)\n", host, port, save ? ", saved" : "");
            return 0;
        }

        if (port > 0) {
            stream_target_t current = {};
            stream_client_get_target(&current);
            if (!current.has_runtime || current.host[0] == '\0') {
                printf("stream set port: set host first\n");
                return 1;
            }
            esp_err_t err = stream_client_set_target(current.host, (uint16_t)port, save);
            if (err != ESP_OK) {
                printf("stream set: %s\n", esp_err_to_name(err));
                return 1;
            }
            printf("stream port set (port=%d%s)\n", port, save ? ", saved" : "");
            return 0;
        }

        print_stream_usage();
        return 1;
    }

    print_stream_usage();
    return 1;
}

static int cmd_cam(int argc, char **argv) {
    if (argc < 2) {
        print_cam_usage();
        return 1;
    }

    if (strcmp(argv[1], "power") == 0) {
        if (argc < 3) {
            print_cam_usage();
            return 1;
        }
        int level = 0;
        if (!try_parse_int(argv[2], &level) || (level != 0 && level != 1)) {
            printf("cam power: expected 0 or 1\n");
            return 1;
        }
        stream_camera_power_set(level);
        stream_camera_power_dump();
        printf("camera power set to %d\n", level);
        return 0;
    }

    if (strcmp(argv[1], "dump") == 0) {
        stream_camera_power_dump();
        return 0;
    }

    print_cam_usage();
    return 1;
}

static int cmd_psram(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "status") != 0) {
        print_psram_usage();
        return 1;
    }

#if CONFIG_SPIRAM
    bool initialized = esp_psram_is_initialized();
    size_t size = initialized ? esp_psram_get_size() : 0;
    size_t free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    printf("psram init=%s size=%u free=%u largest=%u internal_free=%u internal_largest=%u\n",
           initialized ? "yes" : "no",
           (unsigned)size,
           (unsigned)free,
           (unsigned)largest,
           (unsigned)internal_free,
           (unsigned)internal_largest);
#else
    printf("psram disabled (CONFIG_SPIRAM is not set)\n");
#endif

    return 0;
}

static void register_commands(void) {
    esp_console_cmd_t wifi_cmd = {0};
    wifi_cmd.command = "wifi";
    wifi_cmd.help = "WiFi STA config: wifi status|scan|set|connect|disconnect|clear";
    wifi_cmd.func = &cmd_wifi;
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_cmd));

    esp_console_cmd_t stream_cmd = {0};
    stream_cmd.command = "stream";
    stream_cmd.help = "Stream control: stream status|set|start|stop|clear";
    stream_cmd.func = &cmd_stream;
    ESP_ERROR_CHECK(esp_console_cmd_register(&stream_cmd));

    esp_console_cmd_t cam_cmd = {0};
    cam_cmd.command = "cam";
    cam_cmd.help = "Camera power debug: cam power 0|1 (1=on), cam dump";
    cam_cmd.func = &cmd_cam;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cam_cmd));

    esp_console_cmd_t psram_cmd = {0};
    psram_cmd.command = "psram";
    psram_cmd.help = "PSRAM status: psram status";
    psram_cmd.func = &cmd_psram;
    ESP_ERROR_CHECK(esp_console_cmd_register(&psram_cmd));
}

void cam_console_start(void) {
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "cam> ";

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
        ESP_LOGW(TAG, "esp_console unavailable (backend=%s, err=%s)", backend, esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over %s (try: help, wifi status, stream status)", backend);
}
