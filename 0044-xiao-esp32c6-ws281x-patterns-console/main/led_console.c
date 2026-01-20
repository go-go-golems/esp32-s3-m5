#include "led_console.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "led_task.h"

static const char *TAG = "led_console";

static void print_usage(void)
{
    printf("usage:\n");
    printf("  led help\n");
    printf("  led status\n");
    printf("  led log on|off|status\n");
    printf("  led pause | led resume | led clear\n");
    printf("  led brightness <1..100>\n");
    printf("  led frame <ms>\n");
    printf("  led pattern set <off|rainbow|chase|breathing|sparkle>\n");
    printf("  led rainbow set [--speed N] [--sat N] [--spread X10]\n");
    printf("  led chase set [--speed N] [--tail N] [--fg #RRGGBB] [--bg #RRGGBB] [--dir forward|reverse|bounce] [--fade 0|1]\n");
    printf("  led breathing set [--speed N] [--color #RRGGBB] [--min 0..255] [--max 0..255] [--curve sine|linear|ease]\n");
    printf("  led sparkle set [--speed N] [--color #RRGGBB] [--density 0..100] [--fade 1..255] [--mode fixed|random|rainbow] [--bg #RRGGBB]\n");
    printf("  led ws status\n");
    printf("  led ws set gpio <n>\n");
    printf("  led ws set count <n>\n");
    printf("  led ws set order <grb|rgb>\n");
    printf("  led ws set timing [--t0h ns] [--t0l ns] [--t1h ns] [--t1l ns] [--reset us] [--res hz]\n");
    printf("  led ws apply\n");
}

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

static void print_rgb_hex(led_rgb8_t c)
{
    printf("#%02x%02x%02x", (unsigned)c.r, (unsigned)c.g, (unsigned)c.b);
}

static void print_help(void)
{
    printf("led command help\n\n");
    printf("Core:\n");
    printf("  led status\n");
    printf("    Shows driver config, animation state, and the active pattern parameters.\n");
    printf("  led log on|off|status\n");
    printf("    Controls periodic ESP_LOGI status output from the LED task (default: off).\n");
    printf("  led brightness <1..100>\n");
    printf("    Sets global brightness scaling (applied after pattern math).\n");
    printf("  led frame <ms>\n");
    printf("    Sets the animation frame period (task vTaskDelayUntil cadence).\n");
    printf("  led pause | led resume | led clear\n");
    printf("    Pause/resume animation, or clear pixels immediately.\n");
    printf("\nPatterns:\n");
    printf("  Pattern selection:\n");
    printf("    led pattern set <off|rainbow|chase|breathing|sparkle>\n");
    printf("\n  Rainbow:\n");
    printf("    led rainbow set [--speed 1..255] [--sat 0..100] [--spread 1..50]\n");
    printf("      --speed  Higher = hue rotates faster over time.\n");
    printf("      --sat    Saturation percentage.\n");
    printf("      --spread Hue range across strip, in tenths of 360 degrees.\n");
    printf("\n  Chase:\n");
    printf("    led chase set [--speed 1..255] [--tail 1..255] [--fg #RRGGBB] [--bg #RRGGBB]\n");
    printf("                 [--dir forward|reverse|bounce] [--fade 0|1]\n");
    printf("      --speed  Higher = head advances more often (clamped by frame_ms).\n");
    printf("      --tail   Number of LEDs in the tail.\n");
    printf("      --fade   1 enables tail fade (head bright, tail dim).\n");
    printf("\n  Breathing:\n");
    printf("    led breathing set [--speed 1..255] [--color #RRGGBB] [--min 0..255] [--max 0..255]\n");
    printf("                    [--curve sine|linear|ease]\n");
    printf("      --speed  Higher = shorter breathe period (min period is clamped).\n");
    printf("\n  Sparkle:\n");
    printf("    led sparkle set [--speed 1..255] [--color #RRGGBB] [--density 0..100] [--fade 1..255]\n");
    printf("                  [--mode fixed|random|rainbow] [--bg #RRGGBB]\n");
    printf("      --density  Percent chance per LED per frame to spawn a sparkle.\n");
    printf("      --fade     Fade amount per frame.\n");
    printf("      --speed    Multiplier for spawn/fade dynamics (10 keeps base behavior).\n");
    printf("\nWS281x driver:\n");
    printf("  led ws status\n");
    printf("    Shows current applied driver configuration.\n");
    printf("  led ws set gpio <n> | count <n> | order <grb|rgb>\n");
    printf("  led ws set timing [--t0h ns] [--t0l ns] [--t1h ns] [--t1l ns] [--reset us] [--res hz]\n");
    printf("  led ws apply\n");
    printf("    Applies the staged driver config (reinitializes the RMT driver).\n");
}

static void print_pattern_status(const led_pattern_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }

    printf("pattern=%s\n", pattern_name(cfg->type));
    switch (cfg->type) {
    case LED_PATTERN_OFF:
        break;
    case LED_PATTERN_RAINBOW: {
        const led_rainbow_cfg_t *c = &cfg->u.rainbow;
        printf("rainbow: speed=%u sat=%u spread_x10=%u\n", (unsigned)c->speed, (unsigned)c->saturation, (unsigned)c->spread_x10);
        break;
    }
    case LED_PATTERN_CHASE: {
        const led_chase_cfg_t *c = &cfg->u.chase;
        printf("chase: speed=%u tail=%u fg=", (unsigned)c->speed, (unsigned)c->tail_len);
        print_rgb_hex(c->fg);
        printf(" bg=");
        print_rgb_hex(c->bg);
        printf(" dir=%s fade=%u\n",
               (c->dir == LED_DIR_REVERSE) ? "reverse" : (c->dir == LED_DIR_BOUNCE) ? "bounce"
                                                                                    : "forward",
               (unsigned)c->fade_tail);
        break;
    }
    case LED_PATTERN_BREATHING: {
        const led_breathing_cfg_t *c = &cfg->u.breathing;
        printf("breathing: speed=%u color=", (unsigned)c->speed);
        print_rgb_hex(c->color);
        printf(" min=%u max=%u curve=%s\n",
               (unsigned)c->min_bri,
               (unsigned)c->max_bri,
               (c->curve == LED_CURVE_LINEAR) ? "linear" : (c->curve == LED_CURVE_EASE_IN_OUT) ? "ease"
                                                                                               : "sine");
        break;
    }
    case LED_PATTERN_SPARKLE: {
        const led_sparkle_cfg_t *c = &cfg->u.sparkle;
        printf("sparkle: speed=%u color=", (unsigned)c->speed);
        print_rgb_hex(c->color);
        printf(" density=%u fade=%u mode=%s bg=",
               (unsigned)c->density_pct,
               (unsigned)c->fade_speed,
               (c->color_mode == LED_SPARKLE_RANDOM) ? "random" : (c->color_mode == LED_SPARKLE_RAINBOW) ? "rainbow"
                                                                                                         : "fixed");
        print_rgb_hex(c->background);
        printf("\n");
        break;
    }
    default:
        break;
    }
}

static bool parse_u32(const char *s, uint32_t *out)
{
    if (!s || !*s) {
        return false;
    }
    char *end = NULL;
    const unsigned long v = strtoul(s, &end, 10);
    if (!end || *end != '\0') {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

static bool parse_u8(const char *s, uint8_t *out)
{
    uint32_t v = 0;
    if (!parse_u32(s, &v) || v > 255) {
        return false;
    }
    *out = (uint8_t)v;
    return true;
}

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return -1;
}

static bool parse_hex_rgb(const char *s, led_rgb8_t *out)
{
    if (!s || !out) {
        return false;
    }
    if (s[0] == '#') {
        s++;
    }
    if (strlen(s) != 6) {
        return false;
    }
    const int r0 = hex_nibble(s[0]);
    const int r1 = hex_nibble(s[1]);
    const int g0 = hex_nibble(s[2]);
    const int g1 = hex_nibble(s[3]);
    const int b0 = hex_nibble(s[4]);
    const int b1 = hex_nibble(s[5]);
    if (r0 < 0 || r1 < 0 || g0 < 0 || g1 < 0 || b0 < 0 || b1 < 0) {
        return false;
    }
    out->r = (uint8_t)((r0 << 4) | r1);
    out->g = (uint8_t)((g0 << 4) | g1);
    out->b = (uint8_t)((b0 << 4) | b1);
    return true;
}

static led_pattern_type_t parse_pattern(const char *s, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    if (!s) {
        if (ok) {
            *ok = false;
        }
        return LED_PATTERN_OFF;
    }
    if (strcmp(s, "off") == 0) {
        return LED_PATTERN_OFF;
    }
    if (strcmp(s, "rainbow") == 0) {
        return LED_PATTERN_RAINBOW;
    }
    if (strcmp(s, "chase") == 0) {
        return LED_PATTERN_CHASE;
    }
    if (strcmp(s, "breathing") == 0) {
        return LED_PATTERN_BREATHING;
    }
    if (strcmp(s, "sparkle") == 0) {
        return LED_PATTERN_SPARKLE;
    }
    if (ok) {
        *ok = false;
    }
    return LED_PATTERN_OFF;
}

static led_direction_t parse_dir(const char *s, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    if (!s) {
        if (ok) {
            *ok = false;
        }
        return LED_DIR_FORWARD;
    }
    if (strcmp(s, "forward") == 0) {
        return LED_DIR_FORWARD;
    }
    if (strcmp(s, "reverse") == 0) {
        return LED_DIR_REVERSE;
    }
    if (strcmp(s, "bounce") == 0) {
        return LED_DIR_BOUNCE;
    }
    if (ok) {
        *ok = false;
    }
    return LED_DIR_FORWARD;
}

static led_curve_t parse_curve(const char *s, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    if (!s) {
        if (ok) {
            *ok = false;
        }
        return LED_CURVE_SINE;
    }
    if (strcmp(s, "sine") == 0) {
        return LED_CURVE_SINE;
    }
    if (strcmp(s, "linear") == 0) {
        return LED_CURVE_LINEAR;
    }
    if (strcmp(s, "ease") == 0 || strcmp(s, "ease_in_out") == 0) {
        return LED_CURVE_EASE_IN_OUT;
    }
    if (ok) {
        *ok = false;
    }
    return LED_CURVE_SINE;
}

static led_sparkle_color_mode_t parse_mode(const char *s, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    if (!s) {
        if (ok) {
            *ok = false;
        }
        return LED_SPARKLE_FIXED;
    }
    if (strcmp(s, "fixed") == 0) {
        return LED_SPARKLE_FIXED;
    }
    if (strcmp(s, "random") == 0) {
        return LED_SPARKLE_RANDOM;
    }
    if (strcmp(s, "rainbow") == 0) {
        return LED_SPARKLE_RAINBOW;
    }
    if (ok) {
        *ok = false;
    }
    return LED_SPARKLE_FIXED;
}

static bool parse_order(const char *s, led_ws281x_color_order_t *out)
{
    if (!s || !out) {
        return false;
    }
    if (strcmp(s, "grb") == 0) {
        *out = LED_WS281X_ORDER_GRB;
        return true;
    }
    if (strcmp(s, "rgb") == 0) {
        *out = LED_WS281X_ORDER_RGB;
        return true;
    }
    return false;
}

static int send_msg(const led_msg_t *m)
{
    const esp_err_t err = led_task_send(m, 0);
    if (err == ESP_OK) {
        printf("ok\n");
        return 0;
    }
    printf("err=%s\n", esp_err_to_name(err));
    return 1;
}

static int cmd_led(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "log") == 0) {
        if (argc < 3) {
            printf("usage: led log on|off|status\n");
            return 1;
        }
        if (strcmp(argv[2], "status") == 0) {
            led_status_t st = {};
            led_task_get_status(&st);
            printf("enabled=%d\n", (int)st.log_enabled);
            return 0;
        }
        if (strcmp(argv[2], "on") == 0) {
            return send_msg(&(led_msg_t){.type = LED_MSG_SET_LOG_ENABLED, .u.log_enabled = true});
        }
        if (strcmp(argv[2], "off") == 0) {
            return send_msg(&(led_msg_t){.type = LED_MSG_SET_LOG_ENABLED, .u.log_enabled = false});
        }
        printf("invalid log mode\n");
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        led_status_t st = {};
        led_task_get_status(&st);
        printf("running=%d paused=%d\n", (int)st.running, (int)st.paused);
        printf("frame_ms=%u brightness_pct=%u\n", (unsigned)st.frame_ms, (unsigned)st.pat_cfg.global_brightness_pct);
        printf("log_enabled=%d\n", (int)st.log_enabled);
        printf("ws: gpio=%d count=%u order=%s res_hz=%u\n",
               st.ws_cfg.gpio_num,
               (unsigned)st.ws_cfg.led_count,
               (st.ws_cfg.order == LED_WS281X_ORDER_RGB) ? "rgb" : "grb",
               (unsigned)st.ws_cfg.resolution_hz);
        printf("ws_timing_ns: t0h=%u t0l=%u t1h=%u t1l=%u reset_us=%u\n",
               (unsigned)st.ws_cfg.t0h_ns,
               (unsigned)st.ws_cfg.t0l_ns,
               (unsigned)st.ws_cfg.t1h_ns,
               (unsigned)st.ws_cfg.t1l_ns,
               (unsigned)st.ws_cfg.reset_us);
        print_pattern_status(&st.pat_cfg);
        return 0;
    }

    if (strcmp(argv[1], "pause") == 0) {
        return send_msg(&(led_msg_t){.type = LED_MSG_PAUSE});
    }
    if (strcmp(argv[1], "resume") == 0) {
        return send_msg(&(led_msg_t){.type = LED_MSG_RESUME});
    }
    if (strcmp(argv[1], "clear") == 0) {
        return send_msg(&(led_msg_t){.type = LED_MSG_CLEAR});
    }

    if (strcmp(argv[1], "brightness") == 0) {
        if (argc < 3) {
            printf("usage: led brightness <1..100>\n");
            return 1;
        }
        uint32_t v = 0;
        if (!parse_u32(argv[2], &v) || v < 1 || v > 100) {
            printf("invalid brightness\n");
            return 1;
        }
        return send_msg(&(led_msg_t){.type = LED_MSG_SET_GLOBAL_BRIGHTNESS_PCT, .u.brightness_pct = (uint8_t)v});
    }

    if (strcmp(argv[1], "frame") == 0) {
        if (argc < 3) {
            printf("usage: led frame <ms>\n");
            return 1;
        }
        uint32_t v = 0;
        if (!parse_u32(argv[2], &v) || v < 1 || v > 1000) {
            printf("invalid frame\n");
            return 1;
        }
        return send_msg(&(led_msg_t){.type = LED_MSG_SET_FRAME_MS, .u.frame_ms = v});
    }

    if (strcmp(argv[1], "pattern") == 0) {
        if (argc < 4 || strcmp(argv[2], "set") != 0) {
            printf("usage: led pattern set <off|rainbow|chase|breathing|sparkle>\n");
            return 1;
        }
        bool ok = false;
        const led_pattern_type_t t = parse_pattern(argv[3], &ok);
        if (!ok) {
            printf("invalid pattern\n");
            return 1;
        }
        return send_msg(&(led_msg_t){.type = LED_MSG_SET_PATTERN_TYPE, .u.pattern_type = t});
    }

    if (strcmp(argv[1], "rainbow") == 0 && argc >= 3 && strcmp(argv[2], "set") == 0) {
        led_status_t st = {};
        led_task_get_status(&st);

        led_rainbow_cfg_t cfg = {
            .speed = 5,
            .saturation = 100,
            .spread_x10 = 10,
        };
        if (st.pat_cfg.type == LED_PATTERN_RAINBOW) {
            cfg = st.pat_cfg.u.rainbow;
        }

        for (int i = 3; i < argc; i++) {
            const char *k = argv[i];
            const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
            if (strcmp(k, "--speed") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 255) {
                    printf("invalid --speed\n");
                    return 1;
                }
                cfg.speed = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--sat") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp > 100) {
                    printf("invalid --sat\n");
                    return 1;
                }
                cfg.saturation = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--spread") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 50) {
                    printf("invalid --spread\n");
                    return 1;
                }
                cfg.spread_x10 = (uint8_t)tmp;
                i++;
                continue;
            }
            printf("unknown arg: %s\n", k);
            return 1;
        }

        return send_msg(&(led_msg_t){.type = LED_MSG_SET_RAINBOW, .u.rainbow = cfg});
    }

    if (strcmp(argv[1], "chase") == 0 && argc >= 3 && strcmp(argv[2], "set") == 0) {
        led_status_t st = {};
        led_task_get_status(&st);

        led_chase_cfg_t cfg = {
            .speed = 5,
            .tail_len = 5,
            .fg = {.r = 0, .g = 255, .b = 255},
            .bg = {.r = 0, .g = 0, .b = 10},
            .dir = LED_DIR_FORWARD,
            .fade_tail = true,
        };
        if (st.pat_cfg.type == LED_PATTERN_CHASE) {
            cfg = st.pat_cfg.u.chase;
        }

        for (int i = 3; i < argc; i++) {
            const char *k = argv[i];
            const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
            if (strcmp(k, "--speed") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 255) {
                    printf("invalid --speed\n");
                    return 1;
                }
                cfg.speed = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--tail") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 255) {
                    printf("invalid --tail\n");
                    return 1;
                }
                cfg.tail_len = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--fg") == 0 && v) {
                if (!parse_hex_rgb(v, &cfg.fg)) {
                    printf("invalid --fg\n");
                    return 1;
                }
                i++;
                continue;
            }
            if (strcmp(k, "--bg") == 0 && v) {
                if (!parse_hex_rgb(v, &cfg.bg)) {
                    printf("invalid --bg\n");
                    return 1;
                }
                i++;
                continue;
            }
            if (strcmp(k, "--dir") == 0 && v) {
                bool ok = false;
                cfg.dir = parse_dir(v, &ok);
                if (!ok) {
                    printf("invalid --dir\n");
                    return 1;
                }
                i++;
                continue;
            }
            if (strcmp(k, "--fade") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp > 1) {
                    printf("invalid --fade\n");
                    return 1;
                }
                cfg.fade_tail = (tmp != 0);
                i++;
                continue;
            }
            printf("unknown arg: %s\n", k);
            return 1;
        }

        return send_msg(&(led_msg_t){.type = LED_MSG_SET_CHASE, .u.chase = cfg});
    }

    if (strcmp(argv[1], "breathing") == 0 && argc >= 3 && strcmp(argv[2], "set") == 0) {
        led_status_t st = {};
        led_task_get_status(&st);

        led_breathing_cfg_t cfg = {
            .speed = 3,
            .color = {.r = 255, .g = 0, .b = 255},
            .min_bri = 10,
            .max_bri = 255,
            .curve = LED_CURVE_SINE,
        };
        if (st.pat_cfg.type == LED_PATTERN_BREATHING) {
            cfg = st.pat_cfg.u.breathing;
        }

        for (int i = 3; i < argc; i++) {
            const char *k = argv[i];
            const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
            if (strcmp(k, "--speed") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 255) {
                    printf("invalid --speed\n");
                    return 1;
                }
                cfg.speed = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--color") == 0 && v) {
                if (!parse_hex_rgb(v, &cfg.color)) {
                    printf("invalid --color\n");
                    return 1;
                }
                i++;
                continue;
            }
            if (strcmp(k, "--min") == 0 && v) {
                uint8_t tmp = 0;
                if (!parse_u8(v, &tmp)) {
                    printf("invalid --min\n");
                    return 1;
                }
                cfg.min_bri = tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--max") == 0 && v) {
                uint8_t tmp = 0;
                if (!parse_u8(v, &tmp)) {
                    printf("invalid --max\n");
                    return 1;
                }
                cfg.max_bri = tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--curve") == 0 && v) {
                bool ok = false;
                cfg.curve = parse_curve(v, &ok);
                if (!ok) {
                    printf("invalid --curve\n");
                    return 1;
                }
                i++;
                continue;
            }
            printf("unknown arg: %s\n", k);
            return 1;
        }

        return send_msg(&(led_msg_t){.type = LED_MSG_SET_BREATHING, .u.breathing = cfg});
    }

    if (strcmp(argv[1], "sparkle") == 0 && argc >= 3 && strcmp(argv[2], "set") == 0) {
        led_status_t st = {};
        led_task_get_status(&st);

        led_sparkle_cfg_t cfg = {
            .speed = 5,
            .color = {.r = 255, .g = 255, .b = 255},
            .density_pct = 8,
            .fade_speed = 30,
            .color_mode = LED_SPARKLE_FIXED,
            .background = {.r = 5, .g = 5, .b = 5},
        };
        if (st.pat_cfg.type == LED_PATTERN_SPARKLE) {
            cfg = st.pat_cfg.u.sparkle;
        }

        for (int i = 3; i < argc; i++) {
            const char *k = argv[i];
            const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
            if (strcmp(k, "--speed") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 255) {
                    printf("invalid --speed\n");
                    return 1;
                }
                cfg.speed = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--color") == 0 && v) {
                if (!parse_hex_rgb(v, &cfg.color)) {
                    printf("invalid --color\n");
                    return 1;
                }
                i++;
                continue;
            }
            if (strcmp(k, "--density") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp > 100) {
                    printf("invalid --density\n");
                    return 1;
                }
                cfg.density_pct = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--fade") == 0 && v) {
                uint32_t tmp = 0;
                if (!parse_u32(v, &tmp) || tmp < 1 || tmp > 255) {
                    printf("invalid --fade\n");
                    return 1;
                }
                cfg.fade_speed = (uint8_t)tmp;
                i++;
                continue;
            }
            if (strcmp(k, "--mode") == 0 && v) {
                bool ok = false;
                cfg.color_mode = parse_mode(v, &ok);
                if (!ok) {
                    printf("invalid --mode\n");
                    return 1;
                }
                i++;
                continue;
            }
            if (strcmp(k, "--bg") == 0 && v) {
                if (!parse_hex_rgb(v, &cfg.background)) {
                    printf("invalid --bg\n");
                    return 1;
                }
                i++;
                continue;
            }
            printf("unknown arg: %s\n", k);
            return 1;
        }

        return send_msg(&(led_msg_t){.type = LED_MSG_SET_SPARKLE, .u.sparkle = cfg});
    }

    if (strcmp(argv[1], "ws") == 0) {
        if (argc < 3) {
            print_usage();
            return 1;
        }
        if (strcmp(argv[2], "status") == 0) {
            char *v[3] = {"led", "status", NULL};
            return cmd_led(2, v);
        }
        if (strcmp(argv[2], "apply") == 0) {
            return send_msg(&(led_msg_t){.type = LED_MSG_WS_APPLY_CFG});
        }
        if (strcmp(argv[2], "set") != 0 || argc < 5) {
            print_usage();
            return 1;
        }

        led_status_t st = {};
        led_task_get_status(&st);
        led_ws281x_cfg_update_t upd = {
            .set_mask = 0,
            .cfg = st.ws_cfg,
        };

        if (strcmp(argv[3], "gpio") == 0) {
            uint32_t v = 0;
            if (!parse_u32(argv[4], &v) || v > 48) {
                printf("invalid gpio\n");
                return 1;
            }
            upd.set_mask |= LED_WS_CFG_GPIO;
            upd.cfg.gpio_num = (int)v;
            return send_msg(&(led_msg_t){.type = LED_MSG_WS_SET_CFG, .u.ws_update = upd});
        }
        if (strcmp(argv[3], "count") == 0) {
            uint32_t v = 0;
            if (!parse_u32(argv[4], &v) || v < 1 || v > 1024) {
                printf("invalid count\n");
                return 1;
            }
            upd.set_mask |= LED_WS_CFG_LED_COUNT;
            upd.cfg.led_count = (uint16_t)v;
            return send_msg(&(led_msg_t){.type = LED_MSG_WS_SET_CFG, .u.ws_update = upd});
        }
        if (strcmp(argv[3], "order") == 0) {
            led_ws281x_color_order_t o = LED_WS281X_ORDER_GRB;
            if (!parse_order(argv[4], &o)) {
                printf("invalid order\n");
                return 1;
            }
            upd.set_mask |= LED_WS_CFG_ORDER;
            upd.cfg.order = o;
            return send_msg(&(led_msg_t){.type = LED_MSG_WS_SET_CFG, .u.ws_update = upd});
        }
        if (strcmp(argv[3], "timing") == 0) {
            for (int i = 4; i < argc; i++) {
                const char *k = argv[i];
                const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
                uint32_t tmp = 0;
                if (strcmp(k, "--t0h") == 0 && v && parse_u32(v, &tmp)) {
                    upd.set_mask |= LED_WS_CFG_TIMING;
                    upd.cfg.t0h_ns = tmp;
                    i++;
                    continue;
                }
                if (strcmp(k, "--t0l") == 0 && v && parse_u32(v, &tmp)) {
                    upd.set_mask |= LED_WS_CFG_TIMING;
                    upd.cfg.t0l_ns = tmp;
                    i++;
                    continue;
                }
                if (strcmp(k, "--t1h") == 0 && v && parse_u32(v, &tmp)) {
                    upd.set_mask |= LED_WS_CFG_TIMING;
                    upd.cfg.t1h_ns = tmp;
                    i++;
                    continue;
                }
                if (strcmp(k, "--t1l") == 0 && v && parse_u32(v, &tmp)) {
                    upd.set_mask |= LED_WS_CFG_TIMING;
                    upd.cfg.t1l_ns = tmp;
                    i++;
                    continue;
                }
                if (strcmp(k, "--reset") == 0 && v && parse_u32(v, &tmp)) {
                    upd.set_mask |= LED_WS_CFG_RESET_US;
                    upd.cfg.reset_us = tmp;
                    i++;
                    continue;
                }
                if (strcmp(k, "--res") == 0 && v && parse_u32(v, &tmp)) {
                    upd.set_mask |= LED_WS_CFG_RESOLUTION_HZ;
                    upd.cfg.resolution_hz = tmp;
                    i++;
                    continue;
                }
                printf("unknown timing arg: %s\n", k);
                return 1;
            }
            if (upd.set_mask == 0) {
                printf("no fields set\n");
                return 1;
            }
            return send_msg(&(led_msg_t){.type = LED_MSG_WS_SET_CFG, .u.ws_update = upd});
        }

        printf("unknown ws set subcommand: %s\n", argv[3]);
        return 1;
    }

    print_usage();
    return 1;
}

void led_console_start(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "led> ";

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

    esp_console_cmd_t cmd = {};
    cmd.command = "led";
    cmd.help = "Control WS281x patterns (try: led help, led status)";
    cmd.func = &cmd_led;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over %s (try: help, led status)", backend);
}
