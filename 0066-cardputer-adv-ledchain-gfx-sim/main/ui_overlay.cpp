#include "ui_overlay.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "mqjs/js_service.h"

namespace {

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
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
        return "off";
    }
}

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int step_for_mods(uint8_t mods, int micro, int small, int medium, int large)
{
    if (mods & UI_MOD_CTRL) return large;
    if (mods & UI_MOD_ALT) return medium;
    if (mods & UI_MOD_FN) return micro;
    return small;
}

static bool text_eq_ci(const char *a, const char *b)
{
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static bool is_hex_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool parse_hex6(const char *s, led_rgb8_t *out)
{
    if (!s || !out) return false;
    if (strlen(s) != 6) return false;
    unsigned v = 0;
    for (int i = 0; i < 6; i++) {
        char c = s[i];
        unsigned d = 0;
        if (c >= '0' && c <= '9') d = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (unsigned)(10 + (c - 'a'));
        else if (c >= 'A' && c <= 'F') d = (unsigned)(10 + (c - 'A'));
        else return false;
        v = (v << 4) | d;
    }
    out->r = (uint8_t)((v >> 16) & 0xFF);
    out->g = (uint8_t)((v >> 8) & 0xFF);
    out->b = (uint8_t)(v & 0xFF);
    return true;
}

static void hex_from_rgb(const led_rgb8_t &c, char out7[7])
{
    snprintf(out7, 7, "%02X%02X%02X", (unsigned)c.r, (unsigned)c.g, (unsigned)c.b);
}

static void panel(M5Canvas *c, int x, int y, int w, int h)
{
    const uint16_t bg = rgb565(12, 20, 36);
    c->fillRoundRect(x, y, w, h, 8, bg);
    c->drawRoundRect(x, y, w, h, 8, TFT_DARKGREY);
}

static void hint_bar(M5Canvas *c, const char *s)
{
    const int w = (int)c->width();
    const int h = (int)c->height();
    const int bh = 16;
    c->fillRect(0, h - bh, w, bh, TFT_BLACK);
    c->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    c->setCursor(2, h - bh + 2);
    c->print(s ? s : "");
    c->setTextColor(TFT_WHITE, TFT_BLACK);
}

static void apply_pattern(sim_engine_t *engine, led_pattern_type_t t)
{
    led_pattern_cfg_t cfg = {};
    sim_engine_get_cfg(engine, &cfg);
    cfg.type = t;
    sim_engine_set_cfg(engine, &cfg);
}

struct PresetSlot {
    bool used = false;
    led_pattern_cfg_t cfg = {};
    uint32_t frame_ms = 16;
};

static PresetSlot s_presets[6];

static void adjust_brightness(sim_engine_t *engine, int delta)
{
    led_pattern_cfg_t cfg = {};
    sim_engine_get_cfg(engine, &cfg);
    int v = (int)cfg.global_brightness_pct + delta;
    v = clamp_i(v, 1, 100);
    cfg.global_brightness_pct = (uint8_t)v;
    sim_engine_set_cfg(engine, &cfg);
}

static void adjust_frame_ms(sim_engine_t *engine, int delta)
{
    uint32_t ms = sim_engine_get_frame_ms(engine);
    int v = (int)ms + delta;
    v = clamp_i(v, 1, 1000);
    sim_engine_set_frame_ms(engine, (uint32_t)v);
}

enum ParamId {
    P_RAINBOW_SPEED = 0,
    P_RAINBOW_SAT,
    P_RAINBOW_SPREAD,

    P_CHASE_SPEED,
    P_CHASE_TAIL,
    P_CHASE_GAP,
    P_CHASE_TRAINS,
    P_CHASE_DIR,
    P_CHASE_FADE,
    P_CHASE_FG,
    P_CHASE_BG,

    P_BREATH_SPEED,
    P_BREATH_COLOR,
    P_BREATH_MIN,
    P_BREATH_MAX,
    P_BREATH_CURVE,

    P_SPARKLE_SPEED,
    P_SPARKLE_COLOR,
    P_SPARKLE_DENSITY,
    P_SPARKLE_FADE,
    P_SPARKLE_MODE,
    P_SPARKLE_BG,
};

static int params_count(led_pattern_type_t t)
{
    switch (t) {
    case LED_PATTERN_RAINBOW:
        return 3;
    case LED_PATTERN_CHASE:
        return 8;
    case LED_PATTERN_BREATHING:
        return 5;
    case LED_PATTERN_SPARKLE:
        return 6;
    default:
        return 0;
    }
}

static int param_at(led_pattern_type_t t, int idx)
{
    switch (t) {
    case LED_PATTERN_RAINBOW: {
        static const int k[3] = {P_RAINBOW_SPEED, P_RAINBOW_SAT, P_RAINBOW_SPREAD};
        return (idx >= 0 && idx < 3) ? k[idx] : -1;
    }
    case LED_PATTERN_CHASE: {
        static const int k[8] = {P_CHASE_SPEED,
                                 P_CHASE_TAIL,
                                 P_CHASE_GAP,
                                 P_CHASE_TRAINS,
                                 P_CHASE_DIR,
                                 P_CHASE_FADE,
                                 P_CHASE_FG,
                                 P_CHASE_BG};
        return (idx >= 0 && idx < 8) ? k[idx] : -1;
    }
    case LED_PATTERN_BREATHING: {
        static const int k[5] = {P_BREATH_SPEED, P_BREATH_COLOR, P_BREATH_MIN, P_BREATH_MAX, P_BREATH_CURVE};
        return (idx >= 0 && idx < 5) ? k[idx] : -1;
    }
    case LED_PATTERN_SPARKLE: {
        static const int k[6] = {P_SPARKLE_SPEED, P_SPARKLE_COLOR, P_SPARKLE_DENSITY, P_SPARKLE_FADE, P_SPARKLE_MODE,
                                 P_SPARKLE_BG};
        return (idx >= 0 && idx < 6) ? k[idx] : -1;
    }
    default:
        return -1;
    }
}

static const char *param_name(int pid)
{
    switch (pid) {
    case P_RAINBOW_SPEED: return "speed";
    case P_RAINBOW_SAT: return "sat";
    case P_RAINBOW_SPREAD: return "spread";

    case P_CHASE_SPEED: return "speed";
    case P_CHASE_TAIL: return "tail";
    case P_CHASE_GAP: return "gap";
    case P_CHASE_TRAINS: return "trains";
    case P_CHASE_DIR: return "dir";
    case P_CHASE_FADE: return "fade";
    case P_CHASE_FG: return "fg";
    case P_CHASE_BG: return "bg";

    case P_BREATH_SPEED: return "speed";
    case P_BREATH_COLOR: return "color";
    case P_BREATH_MIN: return "min";
    case P_BREATH_MAX: return "max";
    case P_BREATH_CURVE: return "curve";

    case P_SPARKLE_SPEED: return "speed";
    case P_SPARKLE_COLOR: return "color";
    case P_SPARKLE_DENSITY: return "density";
    case P_SPARKLE_FADE: return "fade";
    case P_SPARKLE_MODE: return "mode";
    case P_SPARKLE_BG: return "bg";
    }
    return "?";
}

static bool param_is_color(int pid)
{
    return pid == P_CHASE_FG || pid == P_CHASE_BG || pid == P_BREATH_COLOR || pid == P_SPARKLE_COLOR ||
           pid == P_SPARKLE_BG;
}

static void get_param_value_string(const led_pattern_cfg_t &cfg, int pid, char out[24])
{
    if (!out) return;
    out[0] = 0;
    switch (pid) {
    case P_RAINBOW_SPEED:
        snprintf(out, 24, "%u", (unsigned)cfg.u.rainbow.speed);
        return;
    case P_RAINBOW_SAT:
        snprintf(out, 24, "%u", (unsigned)cfg.u.rainbow.saturation);
        return;
    case P_RAINBOW_SPREAD:
        snprintf(out, 24, "%u", (unsigned)cfg.u.rainbow.spread_x10);
        return;

    case P_CHASE_SPEED:
        snprintf(out, 24, "%u", (unsigned)cfg.u.chase.speed);
        return;
    case P_CHASE_TAIL:
        snprintf(out, 24, "%u", (unsigned)cfg.u.chase.tail_len);
        return;
    case P_CHASE_GAP:
        snprintf(out, 24, "%u", (unsigned)cfg.u.chase.gap_len);
        return;
    case P_CHASE_TRAINS:
        snprintf(out, 24, "%u", (unsigned)cfg.u.chase.trains);
        return;
    case P_CHASE_DIR:
        snprintf(out, 24, "%u", (unsigned)cfg.u.chase.dir);
        return;
    case P_CHASE_FADE:
        snprintf(out, 24, "%s", cfg.u.chase.fade_tail ? "on" : "off");
        return;
    case P_CHASE_FG: {
        char hex[7];
        hex_from_rgb(cfg.u.chase.fg, hex);
        snprintf(out, 24, "#%s", hex);
        return;
    }
    case P_CHASE_BG: {
        char hex[7];
        hex_from_rgb(cfg.u.chase.bg, hex);
        snprintf(out, 24, "#%s", hex);
        return;
    }

    case P_BREATH_SPEED:
        snprintf(out, 24, "%u", (unsigned)cfg.u.breathing.speed);
        return;
    case P_BREATH_COLOR: {
        char hex[7];
        hex_from_rgb(cfg.u.breathing.color, hex);
        snprintf(out, 24, "#%s", hex);
        return;
    }
    case P_BREATH_MIN:
        snprintf(out, 24, "%u", (unsigned)cfg.u.breathing.min_bri);
        return;
    case P_BREATH_MAX:
        snprintf(out, 24, "%u", (unsigned)cfg.u.breathing.max_bri);
        return;
    case P_BREATH_CURVE:
        snprintf(out, 24, "%u", (unsigned)cfg.u.breathing.curve);
        return;

    case P_SPARKLE_SPEED:
        snprintf(out, 24, "%u", (unsigned)cfg.u.sparkle.speed);
        return;
    case P_SPARKLE_COLOR: {
        char hex[7];
        hex_from_rgb(cfg.u.sparkle.color, hex);
        snprintf(out, 24, "#%s", hex);
        return;
    }
    case P_SPARKLE_DENSITY:
        snprintf(out, 24, "%u", (unsigned)cfg.u.sparkle.density_pct);
        return;
    case P_SPARKLE_FADE:
        snprintf(out, 24, "%u", (unsigned)cfg.u.sparkle.fade_speed);
        return;
    case P_SPARKLE_MODE:
        snprintf(out, 24, "%u", (unsigned)cfg.u.sparkle.color_mode);
        return;
    case P_SPARKLE_BG: {
        char hex[7];
        hex_from_rgb(cfg.u.sparkle.background, hex);
        snprintf(out, 24, "#%s", hex);
        return;
    }
    }
}

static led_rgb8_t *get_color_ptr(led_pattern_cfg_t &cfg, int pid)
{
    switch (pid) {
    case P_CHASE_FG:
        return &cfg.u.chase.fg;
    case P_CHASE_BG:
        return &cfg.u.chase.bg;
    case P_BREATH_COLOR:
        return &cfg.u.breathing.color;
    case P_SPARKLE_COLOR:
        return &cfg.u.sparkle.color;
    case P_SPARKLE_BG:
        return &cfg.u.sparkle.background;
    default:
        return nullptr;
    }
}

static void adjust_param(sim_engine_t *engine, int pid, int dir, uint8_t mods)
{
    led_pattern_cfg_t cfg = {};
    sim_engine_get_cfg(engine, &cfg);

    if (pid == P_CHASE_FADE) {
        cfg.u.chase.fade_tail = !cfg.u.chase.fade_tail;
        sim_engine_set_cfg(engine, &cfg);
        return;
    }

    const int sign = (dir < 0) ? -1 : 1;

    if (pid == P_RAINBOW_SPEED) {
        const int step = step_for_mods(mods, 1, 5, 15, 40);
        cfg.u.rainbow.speed = (uint8_t)clamp_i((int)cfg.u.rainbow.speed + sign * step, 0, 255);
    } else if (pid == P_RAINBOW_SAT) {
        const int step = step_for_mods(mods, 1, 5, 15, 40);
        cfg.u.rainbow.saturation = (uint8_t)clamp_i((int)cfg.u.rainbow.saturation + sign * step, 0, 255);
    } else if (pid == P_RAINBOW_SPREAD) {
        const int step = step_for_mods(mods, 1, 1, 2, 5);
        cfg.u.rainbow.spread_x10 = (uint8_t)clamp_i((int)cfg.u.rainbow.spread_x10 + sign * step, 0, 255);

    } else if (pid == P_CHASE_SPEED) {
        const int step = step_for_mods(mods, 1, 5, 15, 40);
        cfg.u.chase.speed = (uint8_t)clamp_i((int)cfg.u.chase.speed + sign * step, 0, 255);
    } else if (pid == P_CHASE_TAIL) {
        const int step = step_for_mods(mods, 1, 1, 2, 5);
        cfg.u.chase.tail_len = (uint8_t)clamp_i((int)cfg.u.chase.tail_len + sign * step, 0, 255);
    } else if (pid == P_CHASE_GAP) {
        const int step = step_for_mods(mods, 1, 1, 2, 5);
        cfg.u.chase.gap_len = (uint8_t)clamp_i((int)cfg.u.chase.gap_len + sign * step, 0, 255);
    } else if (pid == P_CHASE_TRAINS) {
        const int step = step_for_mods(mods, 1, 1, 2, 5);
        cfg.u.chase.trains = (uint8_t)clamp_i((int)cfg.u.chase.trains + sign * step, 0, 20);
    } else if (pid == P_CHASE_DIR) {
        int v = (int)cfg.u.chase.dir + sign;
        v = clamp_i(v, 0, 1);
        cfg.u.chase.dir = (led_direction_t)v;

    } else if (pid == P_BREATH_SPEED) {
        const int step = step_for_mods(mods, 1, 5, 15, 40);
        cfg.u.breathing.speed = (uint8_t)clamp_i((int)cfg.u.breathing.speed + sign * step, 0, 255);
    } else if (pid == P_BREATH_MIN) {
        const int step = step_for_mods(mods, 1, 1, 5, 10);
        cfg.u.breathing.min_bri = (uint8_t)clamp_i((int)cfg.u.breathing.min_bri + sign * step, 0, 255);
    } else if (pid == P_BREATH_MAX) {
        const int step = step_for_mods(mods, 1, 1, 5, 10);
        cfg.u.breathing.max_bri = (uint8_t)clamp_i((int)cfg.u.breathing.max_bri + sign * step, 0, 255);
    } else if (pid == P_BREATH_CURVE) {
        int v = (int)cfg.u.breathing.curve + sign;
        v = clamp_i(v, 0, 2);
        cfg.u.breathing.curve = (led_curve_t)v;

    } else if (pid == P_SPARKLE_SPEED) {
        const int step = step_for_mods(mods, 1, 5, 15, 40);
        cfg.u.sparkle.speed = (uint8_t)clamp_i((int)cfg.u.sparkle.speed + sign * step, 0, 255);
    } else if (pid == P_SPARKLE_DENSITY) {
        const int step = step_for_mods(mods, 1, 1, 5, 10);
        cfg.u.sparkle.density_pct = (uint8_t)clamp_i((int)cfg.u.sparkle.density_pct + sign * step, 0, 100);
    } else if (pid == P_SPARKLE_FADE) {
        const int step = step_for_mods(mods, 1, 1, 5, 10);
        cfg.u.sparkle.fade_speed = (uint8_t)clamp_i((int)cfg.u.sparkle.fade_speed + sign * step, 0, 255);
    } else if (pid == P_SPARKLE_MODE) {
        int v = (int)cfg.u.sparkle.color_mode + sign;
        v = clamp_i(v, 0, 3);
        cfg.u.sparkle.color_mode = (led_sparkle_color_mode_t)v;
    }

    sim_engine_set_cfg(engine, &cfg);
}

} // namespace

void ui_overlay_init(ui_overlay_t *ui)
{
    if (!ui) return;
    memset(ui, 0, sizeof(*ui));
    ui->mode = UI_MODE_LIVE;
    ui->menu_index = 0;
    ui->pattern_index = 0;
    ui->params_index = 0;
    ui->preset_index = 0;
    ui->js_example_index = 0;
    memset(ui->js_output, 0, sizeof(ui->js_output));
    ui->color_active = false;
    ui->color_channel = 0;
    ui->color_hex_len = 0;
    memset(ui->color_hex, 0, sizeof(ui->color_hex));
    ui->color_target = -1;
}

static void open_menu(ui_overlay_t *ui)
{
    ui->mode = UI_MODE_MENU;
    ui->menu_index = clamp_i(ui->menu_index, 0, UI_MENU_COUNT - 1);
}

static void close_to_live(ui_overlay_t *ui)
{
    ui->mode = UI_MODE_LIVE;
    ui->color_active = false;
    ui->color_target = -1;
    ui->color_hex_len = 0;
}

static void open_mode(ui_overlay_t *ui, ui_mode_t m)
{
    ui->mode = m;
    ui->color_active = false;
    ui->color_target = -1;
    ui->color_hex_len = 0;
    if (m == UI_MODE_PRESETS) {
        ui->preset_index = clamp_i(ui->preset_index, 0, 5);
    }
    if (m == UI_MODE_JS) {
        ui->js_example_index = clamp_i(ui->js_example_index, 0, 3);
    }
}

static void open_color_editor(ui_overlay_t *ui, const led_rgb8_t &cur, int target_pid)
{
    ui->color_active = true;
    ui->color_channel = 0;
    ui->color_target = target_pid;
    ui->color_orig = cur;
    ui->color_cur = cur;
    ui->color_hex_len = 0;
    memset(ui->color_hex, 0, sizeof(ui->color_hex));
}

static void apply_color_to_engine(sim_engine_t *engine, int target_pid, const led_rgb8_t &c)
{
    led_pattern_cfg_t cfg = {};
    sim_engine_get_cfg(engine, &cfg);
    led_rgb8_t *p = get_color_ptr(cfg, target_pid);
    if (p) {
        *p = c;
        sim_engine_set_cfg(engine, &cfg);
    }
}

static void cancel_color(ui_overlay_t *ui, sim_engine_t *engine)
{
    if (!ui->color_active) return;
    apply_color_to_engine(engine, ui->color_target, ui->color_orig);
    ui->color_active = false;
    ui->color_target = -1;
    ui->color_hex_len = 0;
}

static void commit_color(ui_overlay_t *ui, sim_engine_t *engine)
{
    if (!ui->color_active) return;
    apply_color_to_engine(engine, ui->color_target, ui->color_cur);
    ui->color_active = false;
    ui->color_target = -1;
    ui->color_hex_len = 0;
}

static void handle_shortcuts(ui_overlay_t *ui, sim_engine_t *engine, const ui_key_event_t *ev)
{
    if (!ui || !engine || !ev) return;

    if (ui->mode != UI_MODE_LIVE) return;

    if (ev->kind == UI_KEY_TEXT && ev->text[0]) {
        const bool fn = (ev->mods & UI_MOD_FN) != 0;
        if (fn && text_eq_ci(ev->text, "h")) {
            open_mode(ui, UI_MODE_HELP);
            return;
        }

        if (text_eq_ci(ev->text, "p")) {
            open_mode(ui, UI_MODE_PATTERN);
            return;
        }
        if (text_eq_ci(ev->text, "e")) {
            open_mode(ui, UI_MODE_PARAMS);
            return;
        }
        if (text_eq_ci(ev->text, "b")) {
            open_mode(ui, UI_MODE_BRIGHTNESS);
            return;
        }
        if (text_eq_ci(ev->text, "f")) {
            open_mode(ui, UI_MODE_FRAME);
            return;
        }
        if (text_eq_ci(ev->text, "j")) {
            open_mode(ui, UI_MODE_JS);
            return;
        }
    }
}

static void presets_save(sim_engine_t *engine, int idx)
{
    if (!engine || idx < 0 || idx >= 6) return;
    s_presets[idx].used = true;
    sim_engine_get_cfg(engine, &s_presets[idx].cfg);
    s_presets[idx].frame_ms = sim_engine_get_frame_ms(engine);
}

static void presets_load(sim_engine_t *engine, int idx)
{
    if (!engine || idx < 0 || idx >= 6) return;
    if (!s_presets[idx].used) return;
    sim_engine_set_cfg(engine, &s_presets[idx].cfg);
    sim_engine_set_frame_ms(engine, s_presets[idx].frame_ms);
}

static void presets_clear(int idx)
{
    if (idx < 0 || idx >= 6) return;
    s_presets[idx] = PresetSlot{};
}

struct JsExample {
    const char *name;
    const char *code;
};

static const JsExample kJsExamples[] = {
    {"Blink G3 (250ms)",
     "var h = every(250, function(){ gpio.toggle('G3'); emit('gpio',{pin:'G3',v:Date.now()}); });\n"
     "emit('log',{msg:'started: cancel(h) to stop', handle:h});\n"},
    {"Cycle patterns",
     "var i=0; var p=['rainbow','chase','breathing','sparkle','off'];\n"
     "var h = every(2000, function(){ sim.setPattern(p[i%p.length]); emit('pattern',{type:p[i%p.length]}); i++; });\n"
     "emit('log',{msg:'cycling: cancel(h) to stop', handle:h});\n"},
    {"Sparkle random color",
     "function rnd(){ var h=Math.floor(Math.random()*255); var s=h.toString(16); return (s.length<2?'0':'')+s; }\n"
     "var c='#'+rnd()+rnd()+rnd(); sim.setSparkle(64,c,20,10,0,'#000000'); emit('sparkle',{color:c});\n"},
    {"Chase beat",
     "sim.setChase(140,40,8,2,'#FF8800','#001020',0,1,25); emit('log',{msg:'chase set'});\n"},
};

static void js_set_output(ui_overlay_t *ui, const std::string &s)
{
    if (!ui) return;
    memset(ui->js_output, 0, sizeof(ui->js_output));
    const size_t n = s.size();
    const size_t cap = sizeof(ui->js_output) - 1;
    const size_t m = (n < cap) ? n : cap;
    memcpy(ui->js_output, s.data(), m);
    ui->js_output[m] = 0;
}

static void js_run_example(ui_overlay_t *ui, int idx)
{
    if (!ui) return;
    if (idx < 0 || (size_t)idx >= (sizeof(kJsExamples) / sizeof(kJsExamples[0]))) return;
    std::string out;
    std::string err;
    const char *code = kJsExamples[idx].code;
    const size_t n = code ? strlen(code) : 0;
    const esp_err_t st = js_service_eval(code, n, 0, "<ui>", &out, &err);
    std::string combined;
    combined.reserve(out.size() + err.size() + 64);
    combined += "example: ";
    combined += kJsExamples[idx].name;
    combined += "\n";
    combined += "status: ";
    combined += esp_err_to_name(st);
    combined += "\n";
    if (!err.empty()) {
        combined += "\nerror:\n";
        combined += err;
    }
    if (!out.empty()) {
        combined += "\noutput:\n";
        combined += out;
    }
    js_set_output(ui, combined);
}

void ui_overlay_handle(ui_overlay_t *ui, sim_engine_t *engine, const ui_key_event_t *ev)
{
    if (!ui || !engine || !ev) return;

    // Tab is a global "menu / back" toggle, except when the color editor is active (Tab cycles channels there).
    if (!ui->color_active && ev->kind == UI_KEY_TAB) {
        if (ui->mode == UI_MODE_LIVE) {
            open_menu(ui);
        } else {
            close_to_live(ui);
        }
        return;
    }

    // Global back closes overlays (or cancels color editor first).
    if (ev->kind == UI_KEY_BACK) {
        if (ui->color_active) {
            cancel_color(ui, engine);
            return;
        }
        close_to_live(ui);
        return;
    }

    // If a color editor is active, it owns the input.
    if (ui->color_active) {
        const int step = step_for_mods(ev->mods, 1, 5, 15, 40);

        if (ev->kind == UI_KEY_TAB) {
            ui->color_channel = (ui->color_channel + 1) % 3;
            return;
        }
        if (ev->kind == UI_KEY_ENTER) {
            commit_color(ui, engine);
            return;
        }
        if (ev->kind == UI_KEY_LEFT || ev->kind == UI_KEY_DOWN) {
            if (ui->color_channel == 0) ui->color_cur.r = (uint8_t)clamp_i((int)ui->color_cur.r - step, 0, 255);
            if (ui->color_channel == 1) ui->color_cur.g = (uint8_t)clamp_i((int)ui->color_cur.g - step, 0, 255);
            if (ui->color_channel == 2) ui->color_cur.b = (uint8_t)clamp_i((int)ui->color_cur.b - step, 0, 255);
            apply_color_to_engine(engine, ui->color_target, ui->color_cur);
            return;
        }
        if (ev->kind == UI_KEY_RIGHT || ev->kind == UI_KEY_UP) {
            if (ui->color_channel == 0) ui->color_cur.r = (uint8_t)clamp_i((int)ui->color_cur.r + step, 0, 255);
            if (ui->color_channel == 1) ui->color_cur.g = (uint8_t)clamp_i((int)ui->color_cur.g + step, 0, 255);
            if (ui->color_channel == 2) ui->color_cur.b = (uint8_t)clamp_i((int)ui->color_cur.b + step, 0, 255);
            apply_color_to_engine(engine, ui->color_target, ui->color_cur);
            return;
        }
        if (ev->kind == UI_KEY_TEXT && ev->text[0] && ui->color_hex_len < 6) {
            const char c = ev->text[0];
            if (is_hex_char(c)) {
                ui->color_hex[ui->color_hex_len++] = c;
                ui->color_hex[ui->color_hex_len] = 0;
                if (ui->color_hex_len == 6) {
                    led_rgb8_t parsed = {};
                    if (parse_hex6(ui->color_hex, &parsed)) {
                        ui->color_cur = parsed;
                        apply_color_to_engine(engine, ui->color_target, ui->color_cur);
                    }
                }
            }
            return;
        }
        return;
    }

    // In overlays, accept Vim-style navigation via raw text keys (keeps UI usable without a dedicated arrow cluster).
    // Only map when no chord modifiers are held, to avoid breaking Ctrl+S / Ctrl+R shortcuts.
    ui_key_event_t mapped = *ev;
    if (ui->mode != UI_MODE_LIVE && ev->kind == UI_KEY_TEXT && ev->text[0] && !(ev->mods & (UI_MOD_CTRL | UI_MOD_ALT | UI_MOD_FN))) {
        if (text_eq_ci(ev->text, "h")) mapped.kind = UI_KEY_LEFT;
        if (text_eq_ci(ev->text, "j")) mapped.kind = UI_KEY_DOWN;
        if (text_eq_ci(ev->text, "k")) mapped.kind = UI_KEY_UP;
        if (text_eq_ci(ev->text, "l")) mapped.kind = UI_KEY_RIGHT;
    }
    ev = &mapped;

    // Global shortcuts work from any non-color state.
    handle_shortcuts(ui, engine, ev);

    switch (ui->mode) {
    case UI_MODE_LIVE:
        return;

    case UI_MODE_MENU:
        if (ev->kind == UI_KEY_UP) ui->menu_index = clamp_i(ui->menu_index - 1, 0, UI_MENU_COUNT - 1);
        if (ev->kind == UI_KEY_DOWN) ui->menu_index = clamp_i(ui->menu_index + 1, 0, UI_MENU_COUNT - 1);
        if (ev->kind == UI_KEY_ENTER) {
            switch ((ui_menu_item_t)ui->menu_index) {
            case UI_MENU_PATTERN:
                open_mode(ui, UI_MODE_PATTERN);
                break;
            case UI_MENU_PARAMS:
                open_mode(ui, UI_MODE_PARAMS);
                break;
            case UI_MENU_BRIGHTNESS:
                open_mode(ui, UI_MODE_BRIGHTNESS);
                break;
            case UI_MENU_FRAME:
                open_mode(ui, UI_MODE_FRAME);
                break;
            case UI_MENU_PRESETS:
                open_mode(ui, UI_MODE_PRESETS);
                break;
            case UI_MENU_JS:
                open_mode(ui, UI_MODE_JS);
                break;
            case UI_MENU_HELP:
                open_mode(ui, UI_MODE_HELP);
                break;
            default:
                break;
            }
        }
        return;

    case UI_MODE_PATTERN: {
        static const led_pattern_type_t kPatterns[] = {
            LED_PATTERN_OFF, LED_PATTERN_RAINBOW, LED_PATTERN_CHASE, LED_PATTERN_BREATHING, LED_PATTERN_SPARKLE};
        const int n = (int)(sizeof(kPatterns) / sizeof(kPatterns[0]));
        if (ev->kind == UI_KEY_UP) ui->pattern_index = clamp_i(ui->pattern_index - 1, 0, n - 1);
        if (ev->kind == UI_KEY_DOWN) ui->pattern_index = clamp_i(ui->pattern_index + 1, 0, n - 1);
        if (ev->kind == UI_KEY_ENTER) {
            apply_pattern(engine, kPatterns[ui->pattern_index]);
            close_to_live(ui);
        }
        return;
    }

    case UI_MODE_BRIGHTNESS: {
        const int step = step_for_mods(ev->mods, 1, 1, 5, 10);
        if (ev->kind == UI_KEY_LEFT || ev->kind == UI_KEY_DOWN) adjust_brightness(engine, -step);
        if (ev->kind == UI_KEY_RIGHT || ev->kind == UI_KEY_UP) adjust_brightness(engine, step);
        if (ev->kind == UI_KEY_ENTER) close_to_live(ui);
        return;
    }

    case UI_MODE_FRAME: {
        const int step = step_for_mods(ev->mods, 1, 2, 5, 20);
        if (ev->kind == UI_KEY_LEFT || ev->kind == UI_KEY_DOWN) adjust_frame_ms(engine, -step);
        if (ev->kind == UI_KEY_RIGHT || ev->kind == UI_KEY_UP) adjust_frame_ms(engine, step);
        if (ev->kind == UI_KEY_ENTER) close_to_live(ui);
        return;
    }

    case UI_MODE_PARAMS: {
        led_pattern_cfg_t cfg = {};
        sim_engine_get_cfg(engine, &cfg);
        const int n = params_count(cfg.type);
        if (n == 0) {
            if (ev->kind == UI_KEY_ENTER) close_to_live(ui);
            return;
        }

        ui->params_index = clamp_i(ui->params_index, 0, n - 1);
        if (ev->kind == UI_KEY_UP) ui->params_index = clamp_i(ui->params_index - 1, 0, n - 1);
        if (ev->kind == UI_KEY_DOWN) ui->params_index = clamp_i(ui->params_index + 1, 0, n - 1);

        const int pid = param_at(cfg.type, ui->params_index);
        if (pid < 0) return;

        if ((ev->kind == UI_KEY_LEFT) || (ev->kind == UI_KEY_RIGHT)) {
            adjust_param(engine, pid, (ev->kind == UI_KEY_LEFT) ? -1 : +1, ev->mods);
            return;
        }

        if (ev->kind == UI_KEY_ENTER) {
            if (param_is_color(pid)) {
                led_pattern_cfg_t cfg2 = {};
                sim_engine_get_cfg(engine, &cfg2);
                led_rgb8_t *p = get_color_ptr(cfg2, pid);
                if (p) {
                    open_color_editor(ui, *p, pid);
                }
            } else {
                // Toggle bools on Enter as well (so users don't need left/right).
                if (pid == P_CHASE_FADE) {
                    adjust_param(engine, pid, +1, ev->mods);
                }
            }
            return;
        }
        return;
    }

    case UI_MODE_HELP:
        if (ev->kind == UI_KEY_ENTER || ev->kind == UI_KEY_TAB) close_to_live(ui);
        return;

    case UI_MODE_PRESETS:
        if (ev->kind == UI_KEY_UP) ui->preset_index = clamp_i(ui->preset_index - 1, 0, 5);
        if (ev->kind == UI_KEY_DOWN) ui->preset_index = clamp_i(ui->preset_index + 1, 0, 5);
        if (ev->kind == UI_KEY_ENTER) {
            presets_load(engine, ui->preset_index);
            close_to_live(ui);
        }
        if (ev->kind == UI_KEY_DEL) {
            presets_clear(ui->preset_index);
        }
        if (ev->kind == UI_KEY_TEXT && (ev->mods & UI_MOD_CTRL) && text_eq_ci(ev->text, "s")) {
            presets_save(engine, ui->preset_index);
        }
        return;

    case UI_MODE_JS:
        if (ev->kind == UI_KEY_UP) ui->js_example_index = clamp_i(ui->js_example_index - 1, 0, 3);
        if (ev->kind == UI_KEY_DOWN) ui->js_example_index = clamp_i(ui->js_example_index + 1, 0, 3);
        if (ev->kind == UI_KEY_ENTER) {
            js_run_example(ui, ui->js_example_index);
        }
        if (ev->kind == UI_KEY_TEXT && (ev->mods & UI_MOD_CTRL) && text_eq_ci(ev->text, "r")) {
            memset(ui->js_output, 0, sizeof(ui->js_output));
        }
        return;
    }
}

void ui_overlay_draw(ui_overlay_t *ui,
                     M5Canvas *canvas,
                     const led_pattern_cfg_t *cfg,
                     uint16_t led_count,
                     uint32_t frame_ms)
{
    if (!ui || !canvas || !cfg) return;

    const int w = (int)canvas->width();
    const int h = (int)canvas->height();

    // Status bar (top, on black).
    canvas->fillRect(0, 0, w, 16, TFT_BLACK);
    canvas->setTextSize(1, 1);
    canvas->setTextDatum(lgfx::textdatum_t::top_left);
    canvas->setTextColor(TFT_WHITE, TFT_BLACK);

    char left[96];
    snprintf(left,
             sizeof(left),
             "%s  bri=%u%%  %ums  %u leds  kb:%s",
             pattern_name(cfg->type),
             (unsigned)cfg->global_brightness_pct,
             (unsigned)frame_ms,
             (unsigned)led_count,
             ui_kb_is_ready() ? "ok" : "err");
    canvas->drawString(left, 2, 2);

    const char *mode = "";
    switch (ui->mode) {
    case UI_MODE_LIVE:
        mode = "LIVE";
        break;
    case UI_MODE_MENU:
        mode = "MENU";
        break;
    case UI_MODE_PATTERN:
        mode = "PATTERN";
        break;
    case UI_MODE_PARAMS:
        mode = "PARAMS";
        break;
    case UI_MODE_BRIGHTNESS:
        mode = "BRI";
        break;
    case UI_MODE_FRAME:
        mode = "FRAME";
        break;
    case UI_MODE_HELP:
        mode = "HELP";
        break;
    case UI_MODE_PRESETS:
        mode = "PRESETS";
        break;
    case UI_MODE_JS:
        mode = "JS";
        break;
    }
    canvas->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    canvas->drawRightString(mode, w - 2, 2);
    canvas->setTextColor(TFT_WHITE, TFT_BLACK);

    // Hint bar (bottom).
    switch (ui->mode) {
    case UI_MODE_LIVE:
        hint_bar(canvas, "TAB menu   P pattern   E params   B bri   F frame   Opt+H help");
        break;
    case UI_MODE_MENU:
        hint_bar(canvas, "HJKL navigate   Enter open   Tab back");
        break;
    case UI_MODE_PATTERN:
        hint_bar(canvas, "HJKL navigate   Enter apply   Tab back");
        break;
    case UI_MODE_PARAMS:
        hint_bar(canvas, "HJKL navigate   Enter (color/toggle)   Tab back");
        break;
    case UI_MODE_BRIGHTNESS:
        hint_bar(canvas, "HJKL adjust   Enter close   Opt=1  Alt=5  Ctrl=10   Tab back");
        break;
    case UI_MODE_FRAME:
        hint_bar(canvas, "HJKL adjust   Enter close   Opt=1  Alt=5  Ctrl=20   Tab back");
        break;
    case UI_MODE_HELP:
        hint_bar(canvas, "Tab close");
        break;
    case UI_MODE_PRESETS:
        hint_bar(canvas, "HJKL slot  Enter load  Ctrl+S save  Del clear  Tab back");
        break;
    case UI_MODE_JS:
        hint_bar(canvas, "HJKL example  Enter run  Ctrl+R clear output  Tab back");
        break;
    }

    // Overlays.
    if (ui->mode == UI_MODE_MENU) {
        const int px = 10;
        const int py = 22;
        const int pw = w / 2;
        const int ph = 16 + UI_MENU_COUNT * 14 + 10;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("MENU");

        static const char *kItems[] = {"Pattern", "Params", "Brightness", "Frame", "Presets", "JS Lab", "Help"};
        for (int i = 0; i < UI_MENU_COUNT; i++) {
            const int y = py + 22 + i * 14;
            const bool sel = (i == ui->menu_index);
            const uint16_t bg = sel ? rgb565(30, 56, 90) : rgb565(12, 20, 36);
            canvas->fillRect(px + 6, y, pw - 12, 14, bg);
            canvas->setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            canvas->setCursor(px + 10, y + 2);
            canvas->print(sel ? "> " : "  ");
            canvas->print(kItems[i]);
        }
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_PATTERN) {
        const int px = 10;
        const int py = 22;
        const int pw = w / 2;
        const int ph = 16 + 5 * 14 + 10;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("PATTERN");

        static const led_pattern_type_t kPatterns[] = {
            LED_PATTERN_OFF, LED_PATTERN_RAINBOW, LED_PATTERN_CHASE, LED_PATTERN_BREATHING, LED_PATTERN_SPARKLE};
        for (int i = 0; i < 5; i++) {
            const int y = py + 22 + i * 14;
            const bool sel = (i == ui->pattern_index);
            const uint16_t bg = sel ? rgb565(30, 56, 90) : rgb565(12, 20, 36);
            canvas->fillRect(px + 6, y, pw - 12, 14, bg);
            canvas->setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            canvas->setCursor(px + 10, y + 2);
            canvas->print(sel ? "> " : "  ");
            canvas->print(pattern_name(kPatterns[i]));
        }
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_BRIGHTNESS) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = 60;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("BRIGHTNESS");

        const int pct = (int)cfg->global_brightness_pct;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        canvas->drawRightString(buf, px + pw - 10, py + 6);

        const int bar_x = px + 10;
        const int bar_y = py + 30;
        const int bar_w = pw - 20;
        const int bar_h = 14;
        canvas->drawRect(bar_x, bar_y, bar_w, bar_h, TFT_DARKGREY);
        const int fill = (bar_w - 2) * pct / 100;
        canvas->fillRect(bar_x + 1, bar_y + 1, fill, bar_h - 2, rgb565(60, 140, 220));
        canvas->fillRect(bar_x + 1 + fill, bar_y + 1, (bar_w - 2) - fill, bar_h - 2, rgb565(24, 34, 56));
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_FRAME) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = 60;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("FRAME");

        char buf[32];
        snprintf(buf, sizeof(buf), "%ums", (unsigned)frame_ms);
        canvas->drawRightString(buf, px + pw - 10, py + 6);

        const int bar_x = px + 10;
        const int bar_y = py + 30;
        const int bar_w = pw - 20;
        const int bar_h = 14;
        canvas->drawRect(bar_x, bar_y, bar_w, bar_h, TFT_DARKGREY);
        // Map 1..200ms to 0..100% for visualization (clamped); beyond 200ms shows "full".
        int ms = (int)frame_ms;
        if (ms < 1) ms = 1;
        if (ms > 200) ms = 200;
        const int pct = (ms - 1) * 100 / 199;
        const int fill = (bar_w - 2) * pct / 100;
        canvas->fillRect(bar_x + 1, bar_y + 1, fill, bar_h - 2, rgb565(60, 140, 220));
        canvas->fillRect(bar_x + 1 + fill, bar_y + 1, (bar_w - 2) - fill, bar_h - 2, rgb565(24, 34, 56));
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_PARAMS) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = h - 22 - 20 - 16;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->printf("PARAMS: %s", pattern_name(cfg->type));

        const int n = params_count(cfg->type);
        for (int i = 0; i < n; i++) {
            const int pid = param_at(cfg->type, i);
            const int y = py + 22 + i * 14;
            const bool sel = (i == ui->params_index);
            const uint16_t bg = sel ? rgb565(30, 56, 90) : rgb565(12, 20, 36);
            canvas->fillRect(px + 6, y, pw - 12, 14, bg);
            canvas->setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            canvas->setCursor(px + 10, y + 2);
            canvas->print(sel ? "> " : "  ");
            canvas->print(param_name(pid));

            char val[24];
            get_param_value_string(*cfg, pid, val);
            canvas->drawRightString(val, px + pw - 10, y + 2);
        }
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->color_active) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = 80;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("COLOR");

        char hex[7];
        hex_from_rgb(ui->color_cur, hex);
        char label[24];
        snprintf(label, sizeof(label), "#%s", hex);
        canvas->drawRightString(label, px + pw - 10, py + 6);

        const uint8_t chan[3] = {ui->color_cur.r, ui->color_cur.g, ui->color_cur.b};
        static const char *names[3] = {"R", "G", "B"};

        for (int i = 0; i < 3; i++) {
            const int y = py + 24 + i * 16;
            const bool sel = (i == ui->color_channel);
            const uint16_t bg = sel ? rgb565(30, 56, 90) : rgb565(12, 20, 36);
            canvas->fillRect(px + 6, y, pw - 12, 16, bg);
            canvas->setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            canvas->setCursor(px + 10, y + 3);
            canvas->printf("%s: %3u", names[i], (unsigned)chan[i]);

            const int bar_x = px + 80;
            const int bar_w = pw - 100;
            canvas->drawRect(bar_x, y + 2, bar_w, 12, TFT_DARKGREY);
            const int fill = (bar_w - 2) * (int)chan[i] / 255;
            canvas->fillRect(bar_x + 1, y + 3, fill, 10, rgb565(60, 140, 220));
        }

        canvas->setTextColor(TFT_LIGHTGREY, rgb565(12, 20, 36));
        canvas->setCursor(px + 10, py + ph - 16);
        if (ui->color_hex_len > 0) {
            canvas->printf("hex: %s", ui->color_hex);
        } else {
            canvas->print("TAB=next  arrows=adjust  hex=type  ENTER=ok  ESC=cancel");
        }
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_HELP) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = 88;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("HELP");

        canvas->setTextColor(TFT_LIGHTGREY, rgb565(12, 20, 36));
        canvas->setCursor(px + 10, py + 24);
        canvas->print("TAB menu   P pattern   E params   B bri   F frame   J JS");
        canvas->setCursor(px + 10, py + 38);
        canvas->print("Fn+H help  Fn micro-step  Alt medium-step  Ctrl large-step");
        canvas->setCursor(px + 10, py + 52);
        canvas->print("Nav: Fn+; up  Fn+. down  Fn+, left  Fn+/ right");
        canvas->setCursor(px + 10, py + 66);
        canvas->print("Back: Fn+`   Del: Fn+del");
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_PRESETS) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = 16 + 6 * 14 + 16;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("PRESETS");

        for (int i = 0; i < 6; i++) {
            const int y = py + 22 + i * 14;
            const bool sel = (i == ui->preset_index);
            const uint16_t bg = sel ? rgb565(30, 56, 90) : rgb565(12, 20, 36);
            canvas->fillRect(px + 6, y, pw - 12, 14, bg);
            canvas->setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            canvas->setCursor(px + 10, y + 2);
            canvas->printf(sel ? "> %d " : "  %d ", i + 1);
            if (s_presets[i].used) {
                canvas->printf("%s bri=%u %ums", pattern_name(s_presets[i].cfg.type),
                               (unsigned)s_presets[i].cfg.global_brightness_pct, (unsigned)s_presets[i].frame_ms);
            } else {
                canvas->print("(empty)");
            }
        }
        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (ui->mode == UI_MODE_JS) {
        const int px = 10;
        const int py = 22;
        const int pw = w - 20;
        const int ph = h - 22 - 20 - 16;
        panel(canvas, px, py, pw, ph);

        canvas->setTextColor(TFT_WHITE, rgb565(12, 20, 36));
        canvas->setCursor(px + 8, py + 6);
        canvas->print("JS LAB");

        const int list_w = pw / 2;
        const int list_x = px + 6;
        const int list_y = py + 22;
        const int out_x = px + list_w + 6;
        const int out_y = py + 22;

        // Example list.
        for (int i = 0; i < 4; i++) {
            const int y = list_y + i * 14;
            const bool sel = (i == ui->js_example_index);
            const uint16_t bg = sel ? rgb565(30, 56, 90) : rgb565(12, 20, 36);
            canvas->fillRect(list_x, y, list_w - 8, 14, bg);
            canvas->setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            canvas->setCursor(list_x + 4, y + 2);
            canvas->print(sel ? "> " : "  ");
            canvas->print(kJsExamples[i].name);
        }

        // Output box.
        canvas->setTextColor(TFT_LIGHTGREY, rgb565(12, 20, 36));
        canvas->drawRect(out_x, out_y, pw - (out_x - px) - 10, ph - (out_y - py) - 10, TFT_DARKGREY);
        canvas->setCursor(out_x + 4, out_y + 2);
        if (ui->js_output[0]) {
            canvas->print(ui->js_output);
        } else {
            canvas->print("(select an example and press Enter)");
        }

        canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    }
}
