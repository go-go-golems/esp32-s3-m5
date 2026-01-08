#include "fan_cmd.h"

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void out_line(fan_cmd_out_fn out, void *ctx, const char *line) {
    if (!out || !line) return;
    out(ctx, line);
}

static void out_printf(fan_cmd_out_fn out, void *ctx, const char *fmt, ...) {
    if (!out || !fmt) return;
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    out(ctx, buf);
}

static bool parse_u32(const char *s, uint32_t *out) {
    if (!s || !out) return false;
    char *end = nullptr;
    unsigned long v = strtoul(s, &end, 0);
    if (!end || *end != '\0') return false;
    if (v > UINT32_MAX) return false;
    *out = (uint32_t)v;
    return true;
}

static void print_help(fan_cmd_out_fn out, void *ctx) {
    out_line(out, ctx, "fan commands:");
    out_line(out, ctx, "  fan status");
    out_line(out, ctx, "  fan on|off|toggle");
    out_line(out, ctx, "  fan stop");
    out_line(out, ctx, "  fan blink [on_ms] [off_ms]");
    out_line(out, ctx, "  fan strobe [ms]");
    out_line(out, ctx, "  fan tick [on_ms] [period_ms]");
    out_line(out, ctx, "  fan beat");
    out_line(out, ctx, "  fan burst [count] [on_ms] [off_ms] [pause_ms]");
    out_line(out, ctx, "  fan preset [name]");
}

int fan_cmd_exec(fan_control_t *fan, int argc, const char *const *argv, fan_cmd_out_fn out, void *ctx) {
    if (!fan || !argv) {
        out_line(out, ctx, "ERR: fan not initialized");
        return 1;
    }
    if (argc < 1 || !argv[0] || argv[0][0] == '\0') {
        print_help(out, ctx);
        return 1;
    }
    if (strcmp(argv[0], "fan") != 0) {
        out_line(out, ctx, "ERR: expected command 'fan'");
        return 1;
    }

    if (argc < 2) {
        print_help(out, ctx);
        return 0;
    }

    const char *sub = argv[1];
    if (!sub || sub[0] == '\0') {
        print_help(out, ctx);
        return 0;
    }

    if (strcmp(sub, "help") == 0) {
        print_help(out, ctx);
        return 0;
    }

    if (strcmp(sub, "status") == 0) {
        out_printf(out,
                   ctx,
                   "fan: ready=%d on=%d mode=%s",
                   fan_control_is_ready(fan) ? 1 : 0,
                   fan_control_is_on(fan) ? 1 : 0,
                   fan_control_mode_name(fan_control_mode(fan)));
        if (fan_control_mode(fan) == FAN_MODE_PRESET) {
            const char *preset = fan_control_active_preset_name(fan);
            out_printf(out, ctx, "preset: %s", preset ? preset : "(none)");
        }
        out_printf(out,
                   ctx,
                   "blink: on_ms=%" PRIu32 " off_ms=%" PRIu32,
                   fan->blink_on_ms,
                   fan->blink_off_ms);
        out_printf(out,
                   ctx,
                   "tick: on_ms=%" PRIu32 " period_ms=%" PRIu32,
                   fan->tick_on_ms,
                   fan->tick_period_ms);
        out_printf(out,
                   ctx,
                   "burst: count=%" PRIu32 " on_ms=%" PRIu32 " off_ms=%" PRIu32 " pause_ms=%" PRIu32,
                   fan->burst_count,
                   fan->burst_on_ms,
                   fan->burst_off_ms,
                   fan->burst_pause_ms);
        return 0;
    }

    if (!fan_control_is_ready(fan)) {
        out_line(out, ctx, "ERR: relay not initialized");
        return 1;
    }

    if (strcmp(sub, "on") == 0) {
        fan_control_set_manual(fan, true);
        out_line(out, ctx, "OK");
        return 0;
    }
    if (strcmp(sub, "off") == 0) {
        fan_control_set_manual(fan, false);
        out_line(out, ctx, "OK");
        return 0;
    }
    if (strcmp(sub, "toggle") == 0) {
        fan_control_toggle_manual(fan);
        out_line(out, ctx, "OK");
        return 0;
    }
    if (strcmp(sub, "stop") == 0) {
        fan_control_stop(fan);
        out_line(out, ctx, "OK");
        return 0;
    }

    if (strcmp(sub, "blink") == 0) {
        uint32_t on_ms = fan->blink_on_ms;
        uint32_t off_ms = fan->blink_off_ms;
        if (argc >= 3 && !parse_u32(argv[2], &on_ms)) {
            out_printf(out, ctx, "ERR: invalid on_ms: %s", argv[2]);
            return 1;
        }
        if (argc >= 4 && !parse_u32(argv[3], &off_ms)) {
            out_printf(out, ctx, "ERR: invalid off_ms: %s", argv[3]);
            return 1;
        }
        fan_control_start_blink(fan, on_ms, off_ms);
        out_line(out, ctx, "OK");
        return 0;
    }

    if (strcmp(sub, "strobe") == 0) {
        uint32_t ms = 100;
        if (argc >= 3 && !parse_u32(argv[2], &ms)) {
            out_printf(out, ctx, "ERR: invalid ms: %s", argv[2]);
            return 1;
        }
        fan_control_start_blink(fan, ms, ms);
        out_line(out, ctx, "OK");
        return 0;
    }

    if (strcmp(sub, "tick") == 0) {
        uint32_t on_ms = fan->tick_on_ms;
        uint32_t period_ms = fan->tick_period_ms;
        if (argc >= 3 && !parse_u32(argv[2], &on_ms)) {
            out_printf(out, ctx, "ERR: invalid on_ms: %s", argv[2]);
            return 1;
        }
        if (argc >= 4 && !parse_u32(argv[3], &period_ms)) {
            out_printf(out, ctx, "ERR: invalid period_ms: %s", argv[3]);
            return 1;
        }
        if (period_ms <= on_ms) {
            out_line(out, ctx, "ERR: period_ms must be > on_ms");
            return 1;
        }
        fan_control_start_tick(fan, on_ms, period_ms);
        out_line(out, ctx, "OK");
        return 0;
    }

    if (strcmp(sub, "beat") == 0) {
        fan_control_start_beat(fan);
        out_line(out, ctx, "OK");
        return 0;
    }

    if (strcmp(sub, "burst") == 0) {
        uint32_t count = fan->burst_count;
        uint32_t on_ms = fan->burst_on_ms;
        uint32_t off_ms = fan->burst_off_ms;
        uint32_t pause_ms = fan->burst_pause_ms;

        if (argc >= 3 && !parse_u32(argv[2], &count)) {
            out_printf(out, ctx, "ERR: invalid count: %s", argv[2]);
            return 1;
        }
        if (argc >= 4 && !parse_u32(argv[3], &on_ms)) {
            out_printf(out, ctx, "ERR: invalid on_ms: %s", argv[3]);
            return 1;
        }
        if (argc >= 5 && !parse_u32(argv[4], &off_ms)) {
            out_printf(out, ctx, "ERR: invalid off_ms: %s", argv[4]);
            return 1;
        }
        if (argc >= 6 && !parse_u32(argv[5], &pause_ms)) {
            out_printf(out, ctx, "ERR: invalid pause_ms: %s", argv[5]);
            return 1;
        }

        fan_control_start_burst(fan, count, on_ms, off_ms, pause_ms);
        out_line(out, ctx, "OK");
        return 0;
    }

    if (strcmp(sub, "preset") == 0) {
        if (argc < 3) {
            size_t n = 0;
            const fan_preset_t *p = fan_control_presets(&n);
            out_line(out, ctx, "presets:");
            for (size_t i = 0; i < n; i++) {
                out_printf(out, ctx, "  %s", p[i].name);
            }
            out_line(out, ctx, "usage: fan preset <name>");
            return 0;
        }

        if (!fan_control_start_preset(fan, argv[2])) {
            out_printf(out, ctx, "ERR: unknown preset: %s", argv[2]);
            out_line(out, ctx, "run: fan preset");
            return 1;
        }
        out_line(out, ctx, "OK");
        return 0;
    }

    out_printf(out, ctx, "ERR: unknown subcommand: %s", sub);
    out_line(out, ctx, "try: fan help");
    return 1;
}
