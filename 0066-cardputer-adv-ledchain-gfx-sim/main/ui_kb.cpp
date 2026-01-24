#include "ui_kb.h"

#include <string.h>

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/task.h"

#include "cardputer_kb/layout.h"
#include "cardputer_kb/scanner.h"

namespace {

static const char *TAG = "0066_kb";

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

