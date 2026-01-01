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

#include "bt_decode.h"
#include "bt_host.h"
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
        printf("usage: scan [le|bredr] on [seconds] | scan [le|bredr] off\n");
        return 1;
    }

    int argi = 1;
    bool bredr = false;
    if (argc >= 3 && (strcmp(argv[1], "le") == 0 || strcmp(argv[1], "bredr") == 0)) {
        bredr = (strcmp(argv[1], "bredr") == 0);
        argi++;
    }

    if (strcmp(argv[argi], "on") == 0) {
        uint32_t sec = 30;
        if (argc >= argi + 2) {
            int v = 0;
            if (!try_parse_int(argv[argi + 1], &v) || v <= 0) {
                printf("invalid seconds: %s\n", argv[argi + 1]);
                return 1;
            }
            sec = (uint32_t)v;
        }
        if (bredr) {
            bt_host_scan_bredr_start(sec);
            printf("scan bredr started (%" PRIu32 "s)\n", sec);
        } else {
            bt_host_scan_le_start(sec);
            printf("scan le started (%" PRIu32 "s)\n", sec);
        }
        return 0;
    }

    if (strcmp(argv[argi], "off") == 0) {
        if (bredr) {
            bt_host_scan_bredr_stop();
            printf("scan bredr stop requested\n");
        } else {
            bt_host_scan_le_stop();
            printf("scan le stop requested\n");
        }
        return 0;
    }

    printf("usage: scan [le|bredr] on [seconds] | scan [le|bredr] off\n");
    return 1;
}

static int cmd_devices(int argc, char **argv) {
    int argi = 1;
    bool bredr = false;
    if (argc >= 2 && (strcmp(argv[1], "le") == 0 || strcmp(argv[1], "bredr") == 0)) {
        bredr = (strcmp(argv[1], "bredr") == 0);
        argi++;
    }

    if (argc > argi && strcmp(argv[argi], "clear") == 0) {
        if (bredr) {
            bt_host_devices_bredr_clear();
            printf("devices bredr cleared\n");
        } else {
            bt_host_devices_le_clear();
            printf("devices le cleared\n");
        }
        return 0;
    }

    if (argc > argi && strcmp(argv[argi], "events") == 0) {
        if (argc <= argi + 1) {
            printf("devices %s events: %s\n",
                   bredr ? "bredr" : "le",
                   bredr ? (bt_host_devices_bredr_events_get_enabled() ? "on" : "off")
                         : (bt_host_devices_le_events_get_enabled() ? "on" : "off"));
            return 0;
        }
        if (strcmp(argv[argi + 1], "on") == 0) {
            if (bredr) {
                bt_host_devices_bredr_events_set_enabled(true);
            } else {
                bt_host_devices_le_events_set_enabled(true);
            }
            printf("devices %s events: on\n", bredr ? "bredr" : "le");
            return 0;
        }
        if (strcmp(argv[argi + 1], "off") == 0) {
            if (bredr) {
                bt_host_devices_bredr_events_set_enabled(false);
            } else {
                bt_host_devices_le_events_set_enabled(false);
            }
            printf("devices %s events: off\n", bredr ? "bredr" : "le");
            return 0;
        }
        printf("usage: devices [le|bredr] events on|off\n");
        return 1;
    }

    const char *filter = (argc > argi) ? argv[argi] : NULL;
    if (bredr) {
        bt_host_devices_bredr_print(filter);
    } else {
        bt_host_devices_le_print(filter);
    }
    return 0;
}

static int cmd_connect(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: connect [le] <index|addr> [pub|rand] | connect bredr <index|addr>\n");
        return 1;
    }

    uint8_t bda[6] = {0};
    uint8_t addr_type = BLE_ADDR_TYPE_PUBLIC;

    int argi = 1;
    bool bredr = false;
    if (argc >= 3 && (strcmp(argv[1], "le") == 0 || strcmp(argv[1], "bredr") == 0)) {
        bredr = (strcmp(argv[1], "bredr") == 0);
        argi++;
    }

    int idx = -1;
    if (try_parse_int(argv[argi], &idx)) {
        if (bredr) {
            bt_bredr_device_t dev = {0};
            if (!bt_host_bredr_device_get_by_index(idx, &dev)) {
                printf("invalid bredr index: %d\n", idx);
                return 1;
            }
            memcpy(bda, dev.bda, sizeof(bda));
        } else {
            bt_le_device_t dev = {0};
            if (!bt_host_le_device_get_by_index(idx, &dev)) {
                printf("invalid le index: %d\n", idx);
                return 1;
            }
            memcpy(bda, dev.bda, sizeof(bda));
            addr_type = dev.addr_type;
        }
    } else {
        if (!parse_bda(argv[argi], bda)) {
            printf("invalid addr: %s\n", argv[argi]);
            return 1;
        }
        if (!bredr) {
            if (argc >= argi + 2) {
                if (strcmp(argv[argi + 1], "pub") == 0) {
                    addr_type = BLE_ADDR_TYPE_PUBLIC;
                } else if (strcmp(argv[argi + 1], "rand") == 0) {
                    addr_type = BLE_ADDR_TYPE_RANDOM;
                } else {
                    printf("invalid addr type: %s (expected pub|rand)\n", argv[argi + 1]);
                    return 1;
                }
            } else {
                bt_le_device_t found = {0};
                if (bt_host_le_device_get_by_bda(bda, &found)) {
                    addr_type = found.addr_type;
                } else {
                    addr_type = BLE_ADDR_TYPE_PUBLIC;
                }
            }
        }
    }

    if (bredr) {
        if (!bt_host_bredr_connect(bda)) {
            printf("connect bredr failed\n");
            return 1;
        }
        printf("connect bredr requested\n");
    } else {
        if (!bt_host_le_connect(bda, addr_type)) {
            printf("connect le failed\n");
            return 1;
        }
        printf("connect le requested\n");
    }
    return 0;
}

static int cmd_disconnect(int, char **) {
    bt_host_disconnect();
    printf("disconnect requested\n");
    return 0;
}

static int cmd_pair(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: pair [le] <index|addr> [pub|rand] | pair bredr <index|addr>\n");
        return 1;
    }

    uint8_t bda[6] = {0};
    uint8_t addr_type = BLE_ADDR_TYPE_PUBLIC;

    int argi = 1;
    bool bredr = false;
    if (argc >= 3 && (strcmp(argv[1], "le") == 0 || strcmp(argv[1], "bredr") == 0)) {
        bredr = (strcmp(argv[1], "bredr") == 0);
        argi++;
    }

    int idx = -1;
    if (try_parse_int(argv[argi], &idx)) {
        if (bredr) {
            bt_bredr_device_t dev = {0};
            if (!bt_host_bredr_device_get_by_index(idx, &dev)) {
                printf("invalid bredr index: %d\n", idx);
                return 1;
            }
            memcpy(bda, dev.bda, sizeof(bda));
        } else {
            bt_le_device_t dev = {0};
            if (!bt_host_le_device_get_by_index(idx, &dev)) {
                printf("invalid le index: %d\n", idx);
                return 1;
            }
            memcpy(bda, dev.bda, sizeof(bda));
            addr_type = dev.addr_type;
        }
    } else {
        if (!parse_bda(argv[argi], bda)) {
            printf("invalid addr: %s\n", argv[argi]);
            return 1;
        }
        if (!bredr) {
            if (argc >= argi + 2) {
                if (strcmp(argv[argi + 1], "pub") == 0) {
                    addr_type = BLE_ADDR_TYPE_PUBLIC;
                } else if (strcmp(argv[argi + 1], "rand") == 0) {
                    addr_type = BLE_ADDR_TYPE_RANDOM;
                } else {
                    printf("invalid addr type: %s (expected pub|rand)\n", argv[argi + 1]);
                    return 1;
                }
            } else {
                bt_le_device_t found = {0};
                if (bt_host_le_device_get_by_bda(bda, &found)) {
                    addr_type = found.addr_type;
                } else {
                    addr_type = BLE_ADDR_TYPE_PUBLIC;
                }
            }
        }
    }

    if (bredr) {
        if (!bt_host_bredr_pair(bda)) {
            printf("pair bredr failed\n");
            return 1;
        }
        printf("pair/connect bredr requested\n");
    } else {
        if (!bt_host_le_pair(bda, addr_type)) {
            printf("pair le failed\n");
            return 1;
        }
        printf("pair/encrypt le requested\n");
    }
    return 0;
}

static int cmd_bonds(int, char **) {
    bt_host_le_bonds_print();
    bt_host_bredr_bonds_print();
    return 0;
}

static int cmd_codes(int, char **) {
    bt_decode_print_all();
    return 0;
}

static int cmd_unpair(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: unpair <addr> | unpair bredr <addr>\n");
        return 1;
    }
    int argi = 1;
    bool bredr = false;
    if (argc >= 3 && strcmp(argv[1], "bredr") == 0) {
        bredr = true;
        argi++;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[argi], bda)) {
        printf("invalid addr: %s\n", argv[argi]);
        return 1;
    }
    if (bredr) {
        if (!bt_host_bredr_unpair(bda)) {
            printf("unpair bredr failed\n");
            return 1;
        }
        printf("unpaired bredr\n");
    } else {
        if (!bt_host_le_unpair(bda)) {
            printf("unpair le failed\n");
            return 1;
        }
        printf("unpaired le\n");
    }
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
    if (!bt_host_le_sec_accept(bda, accept)) {
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
    if (!bt_host_le_passkey_reply(bda, true, (uint32_t)v)) {
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
    if (!bt_host_le_confirm_reply(bda, accept)) {
        printf("confirm reply failed\n");
        return 1;
    }
    printf("confirm reply sent\n");
    return 0;
}

static int cmd_bt_pin(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: bt-pin <addr> <pin|->\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    const bool accept = (strcmp(argv[2], "-") != 0);
    const char *pin = accept ? argv[2] : NULL;
    if (!bt_host_bredr_pin_reply(bda, accept, pin)) {
        printf("bt-pin failed\n");
        return 1;
    }
    printf("bt-pin sent\n");
    return 0;
}

static int cmd_bt_passkey(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: bt-passkey <addr> <6digits>\n");
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
    if (!bt_host_bredr_ssp_passkey_reply(bda, true, (uint32_t)v)) {
        printf("bt-passkey failed\n");
        return 1;
    }
    printf("bt-passkey sent\n");
    return 0;
}

static int cmd_bt_confirm(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: bt-confirm <addr> yes|no\n");
        return 1;
    }
    uint8_t bda[6] = {0};
    if (!parse_bda(argv[1], bda)) {
        printf("invalid addr: %s\n", argv[1]);
        return 1;
    }
    const bool accept = (strcmp(argv[2], "yes") == 0);
    if (!bt_host_bredr_ssp_confirm_reply(bda, accept)) {
        printf("bt-confirm failed\n");
        return 1;
    }
    printf("bt-confirm sent\n");
    return 0;
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {};

    cmd = (esp_console_cmd_t){0};
    cmd.command = "scan";
    cmd.help = "scan [le|bredr] on [seconds] | scan [le|bredr] off";
    cmd.func = &cmd_scan;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "devices";
    cmd.help = "List/clear devices: devices [le|bredr] [substr] | devices [le|bredr] clear | devices [le|bredr] events on|off";
    cmd.func = &cmd_devices;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "connect";
    cmd.help = "Connect to device: connect [le] <index|addr> | connect bredr <index|addr>";
    cmd.func = &cmd_connect;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "disconnect";
    cmd.help = "Disconnect current device";
    cmd.func = &cmd_disconnect;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "pair";
    cmd.help = "Initiate pairing: pair [le] <index|addr> [pub|rand] | pair bredr <index|addr>";
    cmd.func = &cmd_pair;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "bonds";
    cmd.help = "List bonded devices (both LE and BR/EDR)";
    cmd.func = &cmd_bonds;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "codes";
    cmd.help = "Print decoded BLE/GATT status codes";
    cmd.func = &cmd_codes;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "unpair";
    cmd.help = "Remove bond: unpair <addr> | unpair bredr <addr>";
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

    cmd = (esp_console_cmd_t){0};
    cmd.command = "bt-pin";
    cmd.help = "Reply to Classic PIN request: bt-pin <addr> <pin|->";
    cmd.func = &cmd_bt_pin;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "bt-passkey";
    cmd.help = "Reply Classic SSP passkey: bt-passkey <addr> <6digits>";
    cmd.func = &cmd_bt_passkey;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = (esp_console_cmd_t){0};
    cmd.command = "bt-confirm";
    cmd.help = "Reply Classic numeric comparison: bt-confirm <addr> yes|no";
    cmd.func = &cmd_bt_confirm;
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
