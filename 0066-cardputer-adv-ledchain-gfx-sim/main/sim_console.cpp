#include "sim_console.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "sim_console";

static sim_engine_t *s_engine = nullptr;

static const char *pattern_name(led_pattern_type_t t)
{
    switch (t) {
    case LED_PATTERN_OFF:
        return "off";
    case LED_PATTERN_RAINBOW:
        return "rainbow";
    case LED_PATTERN_CHASE:
        return "chase";
    case LED_PATTERN_BREATHING:
        return "breathing";
    case LED_PATTERN_SPARKLE:
        return "sparkle";
    default:
        return "unknown";
    }
}

static bool parse_u32(const char *s, uint32_t *out)
{
    if (!s || !out) return false;
    char *end = nullptr;
    const unsigned long v = strtoul(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (uint32_t)v;
    return true;
}

static bool parse_u8(const char *s, uint8_t *out)
{
    uint32_t v = 0;
    if (!parse_u32(s, &v) || v > 255) return false;
    *out = (uint8_t)v;
    return true;
}

static bool parse_hex_color(const char *s, led_rgb8_t *out)
{
    if (!s || !out) return false;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '#') s++;
    if (strlen(s) != 6) return false;

    char *end = nullptr;
    const unsigned long v = strtoul(s, &end, 16);
    if (!end || *end != '\0' || v > 0xFFFFFFul) return false;

    out->r = (uint8_t)((v >> 16) & 0xFF);
    out->g = (uint8_t)((v >> 8) & 0xFF);
    out->b = (uint8_t)(v & 0xFF);
    return true;
}

static void print_usage(void)
{
    printf("sim: Cardputer LED-chain simulator\n");
    printf("\n");
    printf("usage:\n");
    printf("  sim help\n");
    printf("  sim status\n");
    printf("  sim frame set <ms>\n");
    printf("  sim bri set <1..100>\n");
    printf("  sim pattern set <off|rainbow|chase|breathing|sparkle>\n");
    printf("\n");
    printf("pattern config:\n");
    printf("  sim rainbow set   --speed <0..20> --sat <0..100> --spread <1..50> --bri <1..100>\n");
    printf("  sim chase set     --speed <0..255> --tail <1..255> --gap <0..255> --trains <1..255>\n");
    printf("                  --fg <#RRGGBB> --bg <#RRGGBB> --dir <forward|reverse|bounce> --fade <0|1> --bri <1..100>\n");
    printf("  sim breathing set --speed <0..20> --color <#RRGGBB> --min <0..255> --max <0..255> --curve <sine|linear|ease>\n");
    printf("                  --bri <1..100>\n");
    printf("  sim sparkle set   --speed <0..20> --color <#RRGGBB> --density <0..100> --fade <1..255>\n");
    printf("                  --mode <fixed|random|rainbow> --bg <#RRGGBB> --bri <1..100>\n");
    printf("\n");
}

static void print_status(const led_pattern_cfg_t &cfg)
{
    printf("pattern=%s bri=%u%% frame_ms=%u\n",
           pattern_name(cfg.type),
           (unsigned)cfg.global_brightness_pct,
           (unsigned)sim_engine_get_frame_ms(s_engine));

    switch (cfg.type) {
    case LED_PATTERN_RAINBOW:
        printf("  rainbow: speed=%u sat=%u spread_x10=%u\n",
               (unsigned)cfg.u.rainbow.speed,
               (unsigned)cfg.u.rainbow.saturation,
               (unsigned)cfg.u.rainbow.spread_x10);
        break;
    case LED_PATTERN_CHASE:
        printf("  chase: speed=%u tail=%u gap=%u trains=%u fg=(%u,%u,%u) bg=(%u,%u,%u) dir=%u fade=%u\n",
               (unsigned)cfg.u.chase.speed,
               (unsigned)cfg.u.chase.tail_len,
               (unsigned)cfg.u.chase.gap_len,
               (unsigned)cfg.u.chase.trains,
               (unsigned)cfg.u.chase.fg.r,
               (unsigned)cfg.u.chase.fg.g,
               (unsigned)cfg.u.chase.fg.b,
               (unsigned)cfg.u.chase.bg.r,
               (unsigned)cfg.u.chase.bg.g,
               (unsigned)cfg.u.chase.bg.b,
               (unsigned)cfg.u.chase.dir,
               cfg.u.chase.fade_tail ? 1u : 0u);
        break;
    case LED_PATTERN_BREATHING:
        printf("  breathing: speed=%u color=(%u,%u,%u) min=%u max=%u curve=%u\n",
               (unsigned)cfg.u.breathing.speed,
               (unsigned)cfg.u.breathing.color.r,
               (unsigned)cfg.u.breathing.color.g,
               (unsigned)cfg.u.breathing.color.b,
               (unsigned)cfg.u.breathing.min_bri,
               (unsigned)cfg.u.breathing.max_bri,
               (unsigned)cfg.u.breathing.curve);
        break;
    case LED_PATTERN_SPARKLE:
        printf("  sparkle: speed=%u color=(%u,%u,%u) density=%u fade=%u mode=%u bg=(%u,%u,%u)\n",
               (unsigned)cfg.u.sparkle.speed,
               (unsigned)cfg.u.sparkle.color.r,
               (unsigned)cfg.u.sparkle.color.g,
               (unsigned)cfg.u.sparkle.color.b,
               (unsigned)cfg.u.sparkle.density_pct,
               (unsigned)cfg.u.sparkle.fade_speed,
               (unsigned)cfg.u.sparkle.color_mode,
               (unsigned)cfg.u.sparkle.background.r,
               (unsigned)cfg.u.sparkle.background.g,
               (unsigned)cfg.u.sparkle.background.b);
        break;
    case LED_PATTERN_OFF:
    default:
        break;
    }
}

static int cmd_sim(int argc, char **argv)
{
    if (!s_engine) {
        printf("sim engine not ready\n");
        return 1;
    }
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        led_pattern_cfg_t cfg;
        sim_engine_get_cfg(s_engine, &cfg);
        print_status(cfg);
        return 0;
    }

    if (strcmp(argv[1], "frame") == 0 && argc >= 4 && strcmp(argv[2], "set") == 0) {
        uint32_t ms = 0;
        if (!parse_u32(argv[3], &ms) || ms == 0 || ms > 1000) {
            printf("invalid frame ms\n");
            return 1;
        }
        sim_engine_set_frame_ms(s_engine, ms);
        return 0;
    }

    if (strcmp(argv[1], "bri") == 0 && argc >= 4 && strcmp(argv[2], "set") == 0) {
        uint8_t pct = 0;
        if (!parse_u8(argv[3], &pct) || pct < 1 || pct > 100) {
            printf("invalid brightness pct (expected 1..100)\n");
            return 1;
        }
        led_pattern_cfg_t cfg;
        sim_engine_get_cfg(s_engine, &cfg);
        cfg.global_brightness_pct = pct;
        sim_engine_set_cfg(s_engine, &cfg);
        return 0;
    }

    if (strcmp(argv[1], "pattern") == 0 && argc >= 4 && strcmp(argv[2], "set") == 0) {
        led_pattern_cfg_t cfg;
        sim_engine_get_cfg(s_engine, &cfg);
        const char *p = argv[3];

        if (strcmp(p, "off") == 0) {
            cfg.type = LED_PATTERN_OFF;
        } else if (strcmp(p, "rainbow") == 0) {
            cfg.type = LED_PATTERN_RAINBOW;
            cfg.u.rainbow = (led_rainbow_cfg_t){.speed = 5, .saturation = 100, .spread_x10 = 10};
        } else if (strcmp(p, "chase") == 0) {
            cfg.type = LED_PATTERN_CHASE;
            cfg.u.chase = (led_chase_cfg_t){
                .speed = 30,
                .tail_len = 5,
                .gap_len = 10,
                .trains = 1,
                .fg = (led_rgb8_t){255, 255, 255},
                .bg = (led_rgb8_t){0, 0, 0},
                .dir = LED_DIR_FORWARD,
                .fade_tail = true,
            };
        } else if (strcmp(p, "breathing") == 0) {
            cfg.type = LED_PATTERN_BREATHING;
            cfg.u.breathing = (led_breathing_cfg_t){
                .speed = 6,
                .color = (led_rgb8_t){255, 255, 255},
                .min_bri = 10,
                .max_bri = 255,
                .curve = LED_CURVE_SINE,
            };
        } else if (strcmp(p, "sparkle") == 0) {
            cfg.type = LED_PATTERN_SPARKLE;
            cfg.u.sparkle = (led_sparkle_cfg_t){
                .speed = 10,
                .color = (led_rgb8_t){255, 255, 255},
                .density_pct = 30,
                .fade_speed = 50,
                .color_mode = LED_SPARKLE_FIXED,
                .background = (led_rgb8_t){0, 0, 0},
            };
        } else {
            printf("invalid pattern\n");
            return 1;
        }

        sim_engine_set_cfg(s_engine, &cfg);
        return 0;
    }

    if ((strcmp(argv[1], "rainbow") == 0 || strcmp(argv[1], "chase") == 0 || strcmp(argv[1], "breathing") == 0 || strcmp(argv[1], "sparkle") == 0) &&
        argc >= 3 && strcmp(argv[2], "set") == 0) {
        led_pattern_cfg_t cfg;
        sim_engine_get_cfg(s_engine, &cfg);

        if (strcmp(argv[1], "rainbow") == 0) {
            cfg.type = LED_PATTERN_RAINBOW;
            led_rainbow_cfg_t c = cfg.u.rainbow;
            for (int i = 3; i + 1 < argc; i += 2) {
                const char *k = argv[i];
                const char *v = argv[i + 1];
                uint8_t u8 = 0;
                if (strcmp(k, "--speed") == 0 && parse_u8(v, &u8)) {
                    c.speed = u8;
                } else if (strcmp(k, "--sat") == 0 && parse_u8(v, &u8)) {
                    c.saturation = u8;
                } else if (strcmp(k, "--spread") == 0 && parse_u8(v, &u8)) {
                    c.spread_x10 = u8;
                } else if (strcmp(k, "--bri") == 0 && parse_u8(v, &u8)) {
                    cfg.global_brightness_pct = u8;
                } else {
                    printf("unknown arg: %s\n", k);
                    return 1;
                }
            }
            cfg.u.rainbow = c;
            sim_engine_set_cfg(s_engine, &cfg);
            return 0;
        }

        if (strcmp(argv[1], "chase") == 0) {
            cfg.type = LED_PATTERN_CHASE;
            led_chase_cfg_t c = cfg.u.chase;
            for (int i = 3; i + 1 < argc; i += 2) {
                const char *k = argv[i];
                const char *v = argv[i + 1];
                uint8_t u8 = 0;
                if (strcmp(k, "--speed") == 0 && parse_u8(v, &u8)) {
                    c.speed = u8;
                } else if (strcmp(k, "--tail") == 0 && parse_u8(v, &u8)) {
                    c.tail_len = u8;
                } else if (strcmp(k, "--gap") == 0 && parse_u8(v, &u8)) {
                    c.gap_len = u8;
                } else if (strcmp(k, "--trains") == 0 && parse_u8(v, &u8)) {
                    c.trains = u8;
                } else if (strcmp(k, "--fg") == 0) {
                    if (!parse_hex_color(v, &c.fg)) {
                        printf("invalid fg color\n");
                        return 1;
                    }
                } else if (strcmp(k, "--bg") == 0) {
                    if (!parse_hex_color(v, &c.bg)) {
                        printf("invalid bg color\n");
                        return 1;
                    }
                } else if (strcmp(k, "--dir") == 0) {
                    if (strcmp(v, "forward") == 0) c.dir = LED_DIR_FORWARD;
                    else if (strcmp(v, "reverse") == 0) c.dir = LED_DIR_REVERSE;
                    else if (strcmp(v, "bounce") == 0) c.dir = LED_DIR_BOUNCE;
                    else {
                        printf("invalid dir\n");
                        return 1;
                    }
                } else if (strcmp(k, "--fade") == 0 && parse_u8(v, &u8)) {
                    c.fade_tail = u8 ? true : false;
                } else if (strcmp(k, "--bri") == 0 && parse_u8(v, &u8)) {
                    cfg.global_brightness_pct = u8;
                } else {
                    printf("unknown arg: %s\n", k);
                    return 1;
                }
            }
            cfg.u.chase = c;
            sim_engine_set_cfg(s_engine, &cfg);
            return 0;
        }

        if (strcmp(argv[1], "breathing") == 0) {
            cfg.type = LED_PATTERN_BREATHING;
            led_breathing_cfg_t c = cfg.u.breathing;
            for (int i = 3; i + 1 < argc; i += 2) {
                const char *k = argv[i];
                const char *v = argv[i + 1];
                uint8_t u8 = 0;
                if (strcmp(k, "--speed") == 0 && parse_u8(v, &u8)) {
                    c.speed = u8;
                } else if (strcmp(k, "--color") == 0) {
                    if (!parse_hex_color(v, &c.color)) {
                        printf("invalid color\n");
                        return 1;
                    }
                } else if (strcmp(k, "--min") == 0 && parse_u8(v, &u8)) {
                    c.min_bri = u8;
                } else if (strcmp(k, "--max") == 0 && parse_u8(v, &u8)) {
                    c.max_bri = u8;
                } else if (strcmp(k, "--curve") == 0) {
                    if (strcmp(v, "sine") == 0) c.curve = LED_CURVE_SINE;
                    else if (strcmp(v, "linear") == 0) c.curve = LED_CURVE_LINEAR;
                    else if (strcmp(v, "ease") == 0) c.curve = LED_CURVE_EASE_IN_OUT;
                    else {
                        printf("invalid curve\n");
                        return 1;
                    }
                } else if (strcmp(k, "--bri") == 0 && parse_u8(v, &u8)) {
                    cfg.global_brightness_pct = u8;
                } else {
                    printf("unknown arg: %s\n", k);
                    return 1;
                }
            }
            cfg.u.breathing = c;
            sim_engine_set_cfg(s_engine, &cfg);
            return 0;
        }

        if (strcmp(argv[1], "sparkle") == 0) {
            cfg.type = LED_PATTERN_SPARKLE;
            led_sparkle_cfg_t c = cfg.u.sparkle;
            for (int i = 3; i + 1 < argc; i += 2) {
                const char *k = argv[i];
                const char *v = argv[i + 1];
                uint8_t u8 = 0;
                if (strcmp(k, "--speed") == 0 && parse_u8(v, &u8)) {
                    c.speed = u8;
                } else if (strcmp(k, "--color") == 0) {
                    if (!parse_hex_color(v, &c.color)) {
                        printf("invalid color\n");
                        return 1;
                    }
                } else if (strcmp(k, "--density") == 0 && parse_u8(v, &u8)) {
                    c.density_pct = u8;
                } else if (strcmp(k, "--fade") == 0 && parse_u8(v, &u8)) {
                    c.fade_speed = u8;
                } else if (strcmp(k, "--mode") == 0) {
                    if (strcmp(v, "fixed") == 0) c.color_mode = LED_SPARKLE_FIXED;
                    else if (strcmp(v, "random") == 0) c.color_mode = LED_SPARKLE_RANDOM;
                    else if (strcmp(v, "rainbow") == 0) c.color_mode = LED_SPARKLE_RAINBOW;
                    else {
                        printf("invalid mode\n");
                        return 1;
                    }
                } else if (strcmp(k, "--bg") == 0) {
                    if (!parse_hex_color(v, &c.background)) {
                        printf("invalid bg color\n");
                        return 1;
                    }
                } else if (strcmp(k, "--bri") == 0 && parse_u8(v, &u8)) {
                    cfg.global_brightness_pct = u8;
                } else {
                    printf("unknown arg: %s\n", k);
                    return 1;
                }
            }
            cfg.u.sparkle = c;
            sim_engine_set_cfg(s_engine, &cfg);
            return 0;
        }
    }

    print_usage();
    return 1;
}

void sim_console_register_commands(sim_engine_t *engine)
{
    s_engine = engine;

    esp_console_cmd_t cmd = {};
    cmd.command = "sim";
    cmd.help = "LED-chain simulator control (try: sim help, sim status)";
    cmd.func = &cmd_sim;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void sim_console_start(sim_engine_t *engine)
{
    sim_console_register_commands(engine);

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "sim> ";

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
        ESP_LOGW(TAG, "esp_console not started (backend=%s, err=%s)", backend, esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over %s (try: help, sim status)", backend);
}
