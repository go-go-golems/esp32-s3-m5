#include "bt_console.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"

#include "ble_host.h"
#include "keylog.h"

static const char *TAG = "bt_console";

static bool parse_bda(const char *s, uint8_t out[6]) {
    if (!s || !out) return false;
    unsigned v[6] = {0};
    if (sscanf(s, "%02x:%02x:%02x:%02x:%02x:%02x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; i++) out[i] = (uint8_t)v[i];
    return true;
}

static bool try_parse_int(const char *s, int *out) {
    if (!s || !*s || !out) return false;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static int cmd_scan(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: scan on [seconds] | scan off\n");
        return 1;
    }

    if (strcmp(argv[1], "on") == 0) {
        uint32_t sec = 30;
        if (argc >= 3) {
            int v = 0;
            if (!try_parse_int(argv[2], &v) || v <= 0) {
                printf("invalid seconds: %s\n", argv[2]);
                return 1;
            }
            sec = (uint32_t)v;
        }
        ble_host_scan_start(sec);
        printf("scan started (%" PRIu32 "s)\n", sec);
        return 0;
    }

    if (strcmp(argv[1], "off") == 0) {
        ble_host_scan_stop();
        printf("scan stop requested\n");
        return 0;
    }

    printf("usage: scan on [seconds] | scan off\n");
    return 1;
}

static int cmd_devices(int, char **) {
    ble_host_devices_print();
    return 0;
}

static int cmd_connect(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: connect <index|addr> [pub|rand]\n");
        return 1;
    }

    uint8_t bda[6] = {0};
    uint8_t addr_type = BLE_ADDR_TYPE_PUBLIC;

    int idx = -1;
    ble_host_device_t dev = {0};
    if (try_parse_int(argv[1], &idx)) {
        if (!ble_host_device_get_by_index(idx, &dev)) {
            printf("invalid index: %d\n", idx);
            return 1;
        }
        memcpy(bda, dev.bda, sizeof(bda));
        addr_type = dev.addr_type;
    } else {
        if (!parse_bda(argv[1], bda)) {
            printf("invalid addr: %s\n", argv[1]);
            return 1;
        }
        if (argc >= 3) {
            if (strcmp(argv[2], "pub") == 0) {
                addr_type = BLE_ADDR_TYPE_PUBLIC;
            } else if (strcmp(argv[2], "rand") == 0) {
                addr_type = BLE_ADDR_TYPE_RANDOM;
            } else {
                printf("invalid addr type: %s (expected pub|rand)\n", argv[2]);
                return 1;
            }
        } else {
            addr_type = BLE_ADDR_TYPE_PUBLIC;
        }
    }

    if (!ble_host_connect(bda, addr_type)) {
        printf("connect failed\n");
        return 1;
    }
    printf("connect requested\n");
    return 0;
}

static int cmd_disconnect(int, char **) {
    ble_host_disconnect();
    printf("disconnect requested\n");
    return 0;
}

static int cmd_pair(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: pair <addr>\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    if (!ble_host_pair(bda)) {
        printf("pair failed\n");
        return 1;
    }
    printf("pair/encrypt requested\n");
    return 0;
}

static int cmd_bonds(int, char **) {
    ble_host_bonds_print();
    return 0;
}

static int cmd_unpair(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: unpair <addr>\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    if (!ble_host_unpair(bda)) {
        printf("unpair failed\n");
        return 1;
    }
    printf("unpaired\n");
    return 0;
}

static int cmd_keylog(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: keylog on|off\n");
        return 1;
    }
    if (strcmp(argv[1], "on") == 0) {
        keylog_set_enabled(true);
        printf("keylog enabled\n");
        return 0;
    }
    if (strcmp(argv[1], "off") == 0) {
        keylog_set_enabled(false);
        printf("keylog disabled\n");
        return 0;
    }
    printf("usage: keylog on|off\n");
    return 1;
}

static int cmd_sec_accept(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: sec-accept <addr> yes|no\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    const bool accept = (strcmp(argv[2], "yes") == 0);
    if (!ble_host_sec_accept(bda, accept)) {
        printf("sec-accept failed\n");
        return 1;
    }
    printf("sec-accept sent\n");
    return 0;
}

static int cmd_passkey(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: passkey <addr> <6digits>\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    int v = 0;
    if (!try_parse_int(argv[2], &v) || v < 0 || v > 999999) {
        printf("invalid passkey: %s\n", argv[2]);
        return 1;
    }
    if (!ble_host_passkey_reply(bda, true, (uint32_t)v)) {
        printf("passkey reply failed\n");
        return 1;
    }
    printf("passkey reply sent\n");
    return 0;
}

static int cmd_confirm(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: confirm <addr> yes|no\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    const bool accept = (strcmp(argv[2], "yes") == 0);
    if (!ble_host_confirm_reply(bda, accept)) {
        printf("confirm reply failed\n");
        return 1;
    }
    printf("confirm reply sent\n");
    return 0;
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {};

    cmd = (esp_console_cmd_t){0};
    cmd.command = "scan";
    cmd.help = "scan on [seconds] | scan off";
    cmd.func = &cmd_scan;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "devices";
    cmd.help = "List discovered devices";
    cmd.func = &cmd_devices;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "connect";
    cmd.help = "Connect to device by index or addr: connect <index|addr>";
    cmd.func = &cmd_connect;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "disconnect";
    cmd.help = "Disconnect current device";
    cmd.func = &cmd_disconnect;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "pair";
    cmd.help = "Initiate pairing/encryption: pair <addr>";
    cmd.func = &cmd_pair;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "bonds";
    cmd.help = "List bonded devices";
    cmd.func = &cmd_bonds;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "unpair";
    cmd.help = "Remove bond: unpair <addr>";
    cmd.func = &cmd_unpair;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "keylog";
    cmd.help = "Enable/disable key logging: keylog on|off";
    cmd.func = &cmd_keylog;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "sec-accept";
    cmd.help = "Reply to security request: sec-accept <addr> yes|no";
    cmd.func = &cmd_sec_accept;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "passkey";
    cmd.help = "Reply passkey: passkey <addr> <6digits>";
    cmd.func = &cmd_passkey;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "confirm";
    cmd.help = "Reply numeric comparison: confirm <addr> yes|no";
    cmd.func = &cmd_confirm;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void bt_console_start(void) {
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "ble> ";

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over USB Serial/JTAG");
}
