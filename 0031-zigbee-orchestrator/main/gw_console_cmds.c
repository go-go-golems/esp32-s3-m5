#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_app_desc.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gw_bus.h"
#include "gw_console_cmds.h"
#include "gw_req_id.h"
#include "gw_registry.h"
#include "gw_zb.h"

static void print_usage_gw(void) {
    printf("gw commands:\n");
    printf("  gw status\n");
    printf("  gw devices\n");
    printf("  gw post permit_join <seconds> [req_id]   (seconds=0 closes)\n");
    printf("  gw post onoff <short_addr> <ep> <on|off|toggle> [req_id]\n");
    printf("  gw demo start|stop\n");
}

static void print_usage_monitor(void) {
    printf("monitor commands:\n");
    printf("  monitor on\n");
    printf("  monitor off\n");
    printf("  monitor status\n");
}

static void print_usage_zb(void) {
    printf("zb commands:\n");
    printf("  zb status\n");
    printf("  zb ch <20|25>\n");
    printf("  zb mask <hex_u32>\n");
    printf("  zb reboot\n");
    printf("\n");
    printf("Notes:\n");
    printf("  - The primary channel mask only reliably affects the next network formation.\n");
    printf("  - After changing it, reboot the Cardputer and power-cycle/reset the H2, then re-pair devices.\n");
}

static void print_usage_znsp(void) {
    printf("znsp commands:\n");
    printf("  znsp req <id> [byte0 byte1 ...]\n");
    printf("    - id: uint16 (e.g. 0x0005)\n");
    printf("    - bytes: hex bytes (e.g. 0xaa or aa)\n");
}

static bool parse_u8_hex(const char *s, uint8_t *out) {
    if (!s || !out) return false;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if (!end || *end != '\0') return false;
    if (v > 0xFF) return false;
    *out = (uint8_t)v;
    return true;
}

static int cmd_znsp(int argc, char **argv) {
    if (argc < 3) {
        print_usage_znsp();
        return 1;
    }
    if (strcmp(argv[1], "req") != 0) {
        print_usage_znsp();
        return 1;
    }

    char *end = NULL;
    const unsigned long id_ul = strtoul(argv[2], &end, 0);
    if (!end || *end != '\0' || id_ul > 0xFFFF) {
        printf("invalid id: %s\n", argv[2]);
        return 1;
    }
    const uint16_t id = (uint16_t)id_ul;

    uint8_t in[128];
    uint16_t inlen = 0;
    for (int i = 3; i < argc; i++) {
        if (inlen >= sizeof(in)) {
            printf("too many bytes (max %u)\n", (unsigned)sizeof(in));
            return 1;
        }
        uint8_t b = 0;
        if (!parse_u8_hex(argv[i], &b)) {
            printf("invalid byte: %s\n", argv[i]);
            return 1;
        }
        in[inlen++] = b;
    }

    uint8_t out[256];
    uint16_t outlen = sizeof(out);
    const esp_err_t err = gw_zb_znsp_request(id, inlen ? in : NULL, inlen, out, &outlen, pdMS_TO_TICKS(5000));
    if (err != ESP_OK) {
        printf("znsp req 0x%04x: %s\n", (unsigned)id, esp_err_to_name(err));
        return 1;
    }
    printf("znsp rsp id=0x%04x len=%u:", (unsigned)id, (unsigned)outlen);
    for (uint16_t i = 0; i < outlen; i++) {
        printf(" %02x", out[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_version(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const esp_app_desc_t *app = esp_app_get_description();
    printf("project=%s version=%s idf=%s\n", app->project_name, app->version, app->idf_ver);
    printf("date=%s time=%s\n", app->date, app->time);
    return 0;
}

static int cmd_monitor(int argc, char **argv) {
    if (argc != 2) {
        print_usage_monitor();
        return 1;
    }
    if (strcmp(argv[1], "on") == 0) {
        gw_bus_set_monitor_enabled(true);
        printf("monitor: on\n");
        return 0;
    }
    if (strcmp(argv[1], "off") == 0) {
        gw_bus_set_monitor_enabled(false);
        printf("monitor: off\n");
        return 0;
    }
    if (strcmp(argv[1], "status") == 0) {
        printf("monitor: %s dropped=%" PRIu64 "\n", gw_bus_monitor_enabled() ? "on" : "off", gw_bus_monitor_dropped());
        return 0;
    }
    print_usage_monitor();
    return 1;
}

static int cmd_zb(int argc, char **argv) {
    if (argc < 2) {
        print_usage_zb();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        const uint32_t mask = gw_zb_get_primary_channel_mask();
        printf("zb: primary_channel_mask=0x%08" PRIx32 "\n", mask);
        return 0;
    }

    if (strcmp(argv[1], "ch") == 0) {
        if (argc != 3) {
            print_usage_zb();
            return 1;
        }
        const int ch = atoi(argv[2]);
        if (ch != 20 && ch != 25) {
            printf("unsupported channel: %d (use 20 or 25)\n", ch);
            return 1;
        }
        const uint32_t mask = 1UL << ch;
        const esp_err_t err = gw_zb_set_primary_channel_mask(mask, /*persist=*/true, /*apply_now=*/true, pdMS_TO_TICKS(3000));
        printf("zb: set channel=%d mask=0x%08" PRIx32 " -> %s\n", ch, mask, esp_err_to_name(err));
        return (err == ESP_OK) ? 0 : 1;
    }

    if (strcmp(argv[1], "mask") == 0) {
        if (argc != 3) {
            print_usage_zb();
            return 1;
        }
        char *end = NULL;
        const unsigned long v = strtoul(argv[2], &end, 0);
        if (!end || *end != '\0' || v > 0xFFFFFFFFUL) {
            printf("invalid mask: %s\n", argv[2]);
            return 1;
        }
        const uint32_t mask = (uint32_t)v;
        const esp_err_t err = gw_zb_set_primary_channel_mask(mask, /*persist=*/true, /*apply_now=*/true, pdMS_TO_TICKS(3000));
        printf("zb: set mask=0x%08" PRIx32 " -> %s\n", mask, esp_err_to_name(err));
        return (err == ESP_OK) ? 0 : 1;
    }

    if (strcmp(argv[1], "reboot") == 0) {
        printf("rebooting...\n");
        fflush(stdout);
        esp_restart();
        return 0;
    }

    print_usage_zb();
    return 1;
}

static volatile bool s_demo_running = false;

static void demo_task(void *arg) {
    (void)arg;
    while (s_demo_running) {
        const uint64_t req_id = gw_req_id_next();
        (void)gw_bus_post_permit_join(60, req_id, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    vTaskDelete(NULL);
}

static int cmd_gw(int argc, char **argv) {
    if (argc < 2) {
        print_usage_gw();
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        printf("bus: loop=%p monitor=%s dropped=%" PRIu64 "\n",
               (void *)gw_bus_get_loop(),
               gw_bus_monitor_enabled() ? "on" : "off",
               gw_bus_monitor_dropped());
        return 0;
    }

    if (strcmp(argv[1], "devices") == 0) {
        gw_registry_device_t snap[32];
        const size_t n = gw_registry_snapshot(snap, sizeof(snap) / sizeof(snap[0]));
        printf("devices: %u\n", (unsigned)n);
        for (size_t i = 0; i < n; i++) {
            const gw_registry_device_t *d = &snap[i];
            printf("  short=0x%04x ieee=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x cap=0x%02x seen=%u\n",
                   (unsigned)d->short_addr,
                   d->ieee_addr[7],
                   d->ieee_addr[6],
                   d->ieee_addr[5],
                   d->ieee_addr[4],
                   d->ieee_addr[3],
                   d->ieee_addr[2],
                   d->ieee_addr[1],
                   d->ieee_addr[0],
                   (unsigned)d->capability,
                   (unsigned)d->seen_count);
        }
        return 0;
    }

    if (strcmp(argv[1], "post") == 0) {
        if (argc < 4) {
            print_usage_gw();
            return 1;
        }
        if (strcmp(argv[2], "permit_join") == 0) {
            const int seconds_i = atoi(argv[3]);
            if (seconds_i < 0 || seconds_i > 3600) {
                printf("invalid seconds: %d\n", seconds_i);
                return 1;
            }
            uint64_t req_id = 0;
            if (argc >= 5) {
                req_id = (uint64_t)strtoull(argv[4], NULL, 10);
            }
            if (req_id == 0) req_id = gw_req_id_next();

            esp_err_t err = gw_bus_post_permit_join((uint16_t)seconds_i, req_id, pdMS_TO_TICKS(200));
            if (err != ESP_OK) {
                printf("gw post permit_join: %s\n", esp_err_to_name(err));
                return 1;
            }
            printf("posted GW_CMD_PERMIT_JOIN req_id=%" PRIu64 "\n", req_id);
            return 0;
        }

        if (strcmp(argv[2], "onoff") == 0) {
            if (argc < 6) {
                print_usage_gw();
                return 1;
            }
            const unsigned long short_ul = strtoul(argv[3], NULL, 0);
            if (short_ul > 0xFFFF) {
                printf("invalid short_addr: %s\n", argv[3]);
                return 1;
            }
            const unsigned long ep_ul = strtoul(argv[4], NULL, 0);
            if (ep_ul == 0 || ep_ul > 240) {
                printf("invalid endpoint: %s\n", argv[4]);
                return 1;
            }
            uint8_t cmd_id = 0xFF;
            if (strcmp(argv[5], "off") == 0) cmd_id = 0;
            if (strcmp(argv[5], "on") == 0) cmd_id = 1;
            if (strcmp(argv[5], "toggle") == 0) cmd_id = 2;
            if (cmd_id == 0xFF) {
                printf("invalid onoff cmd: %s\n", argv[5]);
                return 1;
            }

            uint64_t req_id = 0;
            if (argc >= 7) {
                req_id = (uint64_t)strtoull(argv[6], NULL, 10);
            }
            if (req_id == 0) req_id = gw_req_id_next();

            const esp_err_t err =
                gw_bus_post_onoff((uint16_t)short_ul, (uint8_t)ep_ul, cmd_id, req_id, pdMS_TO_TICKS(200));
            if (err != ESP_OK) {
                printf("gw post onoff: %s\n", esp_err_to_name(err));
                return 1;
            }
            printf("posted GW_CMD_ONOFF req_id=%" PRIu64 "\n", req_id);
            return 0;
        }

        print_usage_gw();
        return 1;
    }

    if (strcmp(argv[1], "demo") == 0) {
        if (argc != 3) {
            print_usage_gw();
            return 1;
        }
        if (strcmp(argv[2], "start") == 0) {
            if (s_demo_running) {
                printf("demo: already running\n");
                return 0;
            }
            s_demo_running = true;
            xTaskCreate(&demo_task, "gw_demo", 4096, NULL, 5, NULL);
            printf("demo: started\n");
            return 0;
        }
        if (strcmp(argv[2], "stop") == 0) {
            s_demo_running = false;
            printf("demo: stopping\n");
            return 0;
        }
        print_usage_gw();
        return 1;
    }

    print_usage_gw();
    return 1;
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {0};
    cmd.command = "version";
    cmd.help = "Print build/version info";
    cmd.func = &cmd_version;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    esp_console_cmd_t monitor_cmd = {0};
    monitor_cmd.command = "monitor";
    monitor_cmd.help = "Monitor bus events: monitor on|off|status";
    monitor_cmd.func = &cmd_monitor;
    ESP_ERROR_CHECK(esp_console_cmd_register(&monitor_cmd));

    esp_console_cmd_t zb_cmd = {0};
    zb_cmd.command = "zb";
    zb_cmd.help = "Zigbee controls: zb status | zb ch 20|25 | zb mask <hex> | zb reboot";
    zb_cmd.func = &cmd_zb;
    ESP_ERROR_CHECK(esp_console_cmd_register(&zb_cmd));

    esp_console_cmd_t znsp_cmd = {0};
    znsp_cmd.command = "znsp";
    znsp_cmd.help = "Low-level host↔NCP requests (debug): znsp req <id> [bytes...]";
    znsp_cmd.func = &cmd_znsp;
    ESP_ERROR_CHECK(esp_console_cmd_register(&znsp_cmd));

    esp_console_cmd_t gw_cmd = {0};
    gw_cmd.command = "gw";
    gw_cmd.help = "Gateway shell: gw status | gw post ... | gw demo start|stop";
    gw_cmd.func = &cmd_gw;
    ESP_ERROR_CHECK(esp_console_cmd_register(&gw_cmd));
}

void gw_console_cmds_init(void) {
    register_commands();
}
