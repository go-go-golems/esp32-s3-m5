#include "ui_kb.h"

#include <string.h>

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

#include "cardputer_kb/layout.h"
#include "cardputer_kb/scanner.h"

namespace {

static const char *TAG = "0066_kb";

struct DebugState {
    bool ready = false;
    uint8_t backend = 0;
    bool caps = false;

    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool opt = false;
    bool fn = false;

    uint8_t mods = 0;
    bool down[cardputer_kb::kRows * cardputer_kb::kCols + 1] = {false}; // index 1..56

    bool last_event_valid = false;
    ui_key_event_t last_event = {};

    uint32_t seq = 0;
};

static portMUX_TYPE s_dbg_mu = portMUX_INITIALIZER_UNLOCKED;
static DebugState s_dbg;

struct KeyValue {
    const char *first;
    const char *second;
};

// Vendor legend matches the "picture" coordinate system (4x14).
// Derived from M5Cardputer(-ADV) vendor demos.
constexpr KeyValue kKeyMapCardputer[4][14] = {
    {{"`", "~"}, {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"}, {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"}, {"del", "del"}},
    {{"tab", "tab"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"}, {"\\", "|"}},
    {{"fn", "fn"}, {"shift", "shift"}, {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"}, {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"}, {"'", "\""}, {"enter", "enter"}},
    {{"ctrl", "ctrl"}, {"opt", "opt"}, {"alt", "alt"}, {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"}, {"space", "space"}},
};

// Cardputer-ADV keyboard legend: differs in the "Fn/Shift/Caps" region.
// Derived from `0036-cardputer-adv-led-matrix-console/main/matrix_console.c`.
constexpr KeyValue kKeyMapCardputerAdv[4][14] = {
    {{"`", "~"}, {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"}, {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"}, {"del", "del"}},
    {{"tab", "tab"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"}, {"\\", "|"}},
    {{"shift", "shift"}, {"capslock", "capslock"}, {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"}, {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"}, {"'", "\""}, {"enter", "enter"}},
    {{"ctrl", "ctrl"}, {"opt", "opt"}, {"alt", "alt"}, {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"}, {"space", "space"}},
};

static void q_send_best_effort(QueueHandle_t q, const ui_key_event_t &ev)
{
    if (!q) return;
    (void)xQueueSend(q, &ev, 0);

    portENTER_CRITICAL(&s_dbg_mu);
    s_dbg.last_event = ev;
    s_dbg.last_event_valid = true;
    portEXIT_CRITICAL(&s_dbg_mu);
}

static bool key_is(const KeyValue *kv, const char *s)
{
    if (!kv || !s) return false;
    return strcmp(kv->first, s) == 0;
}

static bool key_is_letter(const char *s)
{
    if (!s || s[0] == '\0' || s[1] != '\0') return false;
    const char c = s[0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static uint8_t mods_from_state(bool shift, bool ctrl, bool alt, bool fn)
{
    uint8_t mods = 0;
    if (shift) mods |= UI_MOD_SHIFT;
    if (ctrl) mods |= UI_MOD_CTRL;
    if (alt) mods |= UI_MOD_ALT;
    if (fn) mods |= UI_MOD_FN;
    return mods;
}

static const KeyValue *key_value_for_keynum(const KeyValue map[4][14], uint8_t keynum)
{
    int x = -1;
    int y = -1;
    cardputer_kb::xy_from_keynum(keynum, &x, &y);
    if (x < 0 || x >= 14 || y < 0 || y >= 4) {
        return nullptr;
    }
    return &map[y][x];
}

static bool keynum_is_modifier_cardputer(uint8_t keynum)
{
    // Fn, Shift, Ctrl, Opt, Alt in Cardputer legend.
    return keynum == 29 || keynum == 30 || keynum == 43 || keynum == 44 || keynum == 45;
}

static bool keynum_is_modifier_adv(uint8_t keynum)
{
    // Shift, CapsLock, Ctrl, Opt, Alt in Cardputer-ADV legend.
    return keynum == 29 || keynum == 30 || keynum == 43 || keynum == 44 || keynum == 45;
}

static bool s_ready = false;
static cardputer_kb::UnifiedScanner s_scanner;

static void kb_task_main(void *arg)
{
    QueueHandle_t q = (QueueHandle_t)arg;

    const esp_err_t init = s_scanner.init();
    if (init != ESP_OK) {
        ESP_LOGE(TAG, "UnifiedScanner init failed: %s", esp_err_to_name(init));
        vTaskDelete(nullptr);
        return;
    }

    const cardputer_kb::ScannerBackend backend = s_scanner.backend();
    const bool is_adv = backend == cardputer_kb::ScannerBackend::Tca8418;
    const KeyValue (*map)[14] = is_adv ? kKeyMapCardputerAdv : kKeyMapCardputer;

    ESP_LOGI(TAG, "keyboard backend: %s", is_adv ? "TCA8418 (Cardputer-ADV)" : "GPIO matrix (Cardputer)");

    bool caps = false;
    bool prev_down[cardputer_kb::kRows * cardputer_kb::kCols + 1] = {false}; // index 1..56

    s_ready = true;
    portENTER_CRITICAL(&s_dbg_mu);
    s_dbg.ready = true;
    s_dbg.backend = (uint8_t)backend;
    s_dbg.last_event_valid = false;
    s_dbg.seq = 0;
    portEXIT_CRITICAL(&s_dbg_mu);

    while (true) {
        const cardputer_kb::ScanSnapshot snap = s_scanner.scan();

        bool down[cardputer_kb::kRows * cardputer_kb::kCols + 1] = {false}; // index 1..56
        for (uint8_t keynum : snap.pressed_keynums) {
            if (keynum >= 1 && keynum <= (cardputer_kb::kRows * cardputer_kb::kCols)) {
                down[keynum] = true;
            }
        }

        const bool shift = is_adv ? down[29] : down[30];
        const bool ctrl = down[43];
        const bool alt = is_adv ? down[45] : (down[45] || down[44]); // treat Opt as Alt on Cardputer
        const bool opt = down[44];
        const bool fn = is_adv ? opt : down[29]; // ADV: treat Opt as Fn-like modifier

        if (is_adv && down[30] && !prev_down[30]) {
            caps = !caps;
        }

        const uint8_t mods = mods_from_state(shift, ctrl, alt, fn);

        portENTER_CRITICAL(&s_dbg_mu);
        s_dbg.caps = caps;
        s_dbg.shift = shift;
        s_dbg.ctrl = ctrl;
        s_dbg.alt = alt;
        s_dbg.opt = opt;
        s_dbg.fn = fn;
        s_dbg.mods = mods;
        memcpy(s_dbg.down, down, sizeof(s_dbg.down));
        s_dbg.seq++;
        portEXIT_CRITICAL(&s_dbg_mu);

        // Emit "edge" events only (press transitions).
        for (uint8_t keynum = 1; keynum <= (cardputer_kb::kRows * cardputer_kb::kCols); keynum++) {
            if (!down[keynum] || prev_down[keynum]) {
                continue;
            }

            // Never emit modifier keys as text.
            if (is_adv) {
                if (keynum_is_modifier_adv(keynum)) {
                    continue;
                }
            } else {
                if (keynum_is_modifier_cardputer(keynum)) {
                    continue;
                }
            }

            const KeyValue *kv = key_value_for_keynum(map, keynum);
            if (!kv) {
                continue;
            }

            // Map semantic navigation chords to arrow/back events.
            //
            // Cardputer (captured bindings): Fn(29) + {;,(.),, ,/} => {Up,Down,Left,Right}
            // Back: Fn(29) + ` (keynum 1)
            //
            // Cardputer-ADV: there is no dedicated Fn key; we treat Opt as Fn-like (fn==true above).
            if (fn) {
                if (keynum == 40) { // ;
                    ui_key_event_t ev = {};
                    ev.kind = UI_KEY_UP;
                    ev.mods = mods;
                    ev.keynum = keynum;
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (keynum == 54) { // .
                    ui_key_event_t ev = {};
                    ev.kind = UI_KEY_DOWN;
                    ev.mods = mods;
                    ev.keynum = keynum;
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (keynum == 53) { // ,
                    ui_key_event_t ev = {};
                    ev.kind = UI_KEY_LEFT;
                    ev.mods = mods;
                    ev.keynum = keynum;
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (keynum == 55) { // /
                    ui_key_event_t ev = {};
                    ev.kind = UI_KEY_RIGHT;
                    ev.mods = mods;
                    ev.keynum = keynum;
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (keynum == 1) { // `
                    ui_key_event_t ev = {};
                    ev.kind = UI_KEY_BACK;
                    ev.mods = mods;
                    ev.keynum = keynum;
                    q_send_best_effort(q, ev);
                    continue;
                }
            }

            // Special key kinds.
            if (key_is(kv, "tab")) {
                ui_key_event_t ev = {};
                ev.kind = UI_KEY_TAB;
                ev.mods = mods;
                ev.keynum = keynum;
                q_send_best_effort(q, ev);
                continue;
            }
            if (key_is(kv, "enter")) {
                ui_key_event_t ev = {};
                ev.kind = UI_KEY_ENTER;
                ev.mods = mods;
                ev.keynum = keynum;
                q_send_best_effort(q, ev);
                continue;
            }
            if (key_is(kv, "del")) {
                ui_key_event_t ev = {};
                ev.kind = UI_KEY_DEL;
                ev.mods = mods;
                ev.keynum = keynum;
                q_send_best_effort(q, ev);
                continue;
            }
            if (key_is(kv, "space")) {
                ui_key_event_t ev = {};
                ev.kind = UI_KEY_SPACE;
                ev.mods = mods;
                ev.keynum = keynum;
                q_send_best_effort(q, ev);
                continue;
            }

            const bool use_shifted = key_is_letter(kv->first) ? (shift ^ caps) : shift;
            const char *s = use_shifted ? kv->second : kv->first;
            if (!s || !*s) {
                continue;
            }

            ui_key_event_t ev = {};
            ev.kind = UI_KEY_TEXT;
            ev.mods = mods;
            ev.keynum = keynum;
            strncpy(ev.text, s, sizeof(ev.text) - 1);
            ev.text[sizeof(ev.text) - 1] = '\0';
            q_send_best_effort(q, ev);
        }

        memcpy(prev_down, down, sizeof(prev_down));
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0066_KB_SCAN_PERIOD_MS));
    }
}

} // namespace

extern "C" void ui_kb_start(QueueHandle_t q)
{
    // Stack is conservative: C++ (UnifiedScanner + std::vector) + event decode.
    (void)xTaskCreate(&kb_task_main, "0066_kb", 4096, (void *)q, 6, nullptr);
}

extern "C" bool ui_kb_is_ready(void)
{
    return s_ready;
}

extern "C" bool ui_kb_debug_get_state(ui_kb_debug_state_t *out)
{
    if (!out) return false;

    DebugState snap;
    portENTER_CRITICAL(&s_dbg_mu);
    snap = s_dbg;
    portEXIT_CRITICAL(&s_dbg_mu);

    memset(out, 0, sizeof(*out));
    out->ready = snap.ready;
    out->backend = snap.backend;
    out->caps = snap.caps;
    out->shift = snap.shift;
    out->ctrl = snap.ctrl;
    out->alt = snap.alt;
    out->opt = snap.opt;
    out->fn = snap.fn;
    out->mods = snap.mods;
    out->last_event_valid = snap.last_event_valid;
    out->last_event = snap.last_event;
    out->seq = snap.seq;

    uint8_t n = 0;
    for (uint8_t keynum = 1; keynum <= (cardputer_kb::kRows * cardputer_kb::kCols); keynum++) {
        if (!snap.down[keynum]) continue;
        if (n < (uint8_t)(sizeof(out->pressed_keynums) / sizeof(out->pressed_keynums[0]))) {
            out->pressed_keynums[n++] = keynum;
        }
    }
    out->pressed_count = n;
    return true;
}
