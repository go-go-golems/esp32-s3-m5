#include "ui_kb.h"

#include <string.h>

#include <vector>

#include "sdkconfig.h"

#include "freertos/task.h"

#include "cardputer_kb/bindings.h"
#include "cardputer_kb/bindings_m5cardputer_captured.h"
#include "cardputer_kb/layout.h"
#include "cardputer_kb/scanner.h"

namespace {

struct KeyValue {
    const char *first;
    const char *second;
};

// Vendor legend matches the "picture" coordinate system (4x14).
// Derived from `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`.
constexpr KeyValue kKeyMap[4][14] = {
    {{"`", "~"}, {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"}, {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"}, {"del", "del"}},
    {{"tab", "tab"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"}, {"\\", "|"}},
    {{"fn", "fn"}, {"shift", "shift"}, {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"}, {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"}, {"'", "\""}, {"enter", "enter"}},
    {{"ctrl", "ctrl"}, {"opt", "opt"}, {"alt", "alt"}, {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"}, {"space", "space"}},
};

static bool contains_keynum(const std::vector<uint8_t> &v, uint8_t keynum) {
    for (auto x : v) {
        if (x == keynum) return true;
    }
    return false;
}

static const KeyValue *key_value_for_keynum(uint8_t keynum) {
    int x = -1;
    int y = -1;
    cardputer_kb::xy_from_keynum(keynum, &x, &y);
    if (x < 0 || x >= 14 || y < 0 || y >= 4) {
        return nullptr;
    }
    return &kKeyMap[y][x];
}

static bool is_modifier_keynum(uint8_t keynum) {
    const KeyValue *kv = key_value_for_keynum(keynum);
    if (!kv) return false;
    return strcmp(kv->first, "shift") == 0 || strcmp(kv->first, "fn") == 0 || strcmp(kv->first, "ctrl") == 0 ||
           strcmp(kv->first, "alt") == 0 || strcmp(kv->first, "opt") == 0;
}

static bool binding_contains_keynum(const cardputer_kb::Binding &b, uint8_t keynum) {
    for (uint8_t i = 0; i < b.n; i++) {
        if (b.keynums[i] == keynum) return true;
    }
    return false;
}

static ui_key_kind_t kind_from_action(cardputer_kb::Action a) {
    switch (a) {
    case cardputer_kb::Action::NavUp: return UI_KEY_UP;
    case cardputer_kb::Action::NavDown: return UI_KEY_DOWN;
    case cardputer_kb::Action::NavLeft: return UI_KEY_LEFT;
    case cardputer_kb::Action::NavRight: return UI_KEY_RIGHT;
    case cardputer_kb::Action::Back: return UI_KEY_BACK;
    case cardputer_kb::Action::Del: return UI_KEY_DEL;
    case cardputer_kb::Action::Enter: return UI_KEY_ENTER;
    case cardputer_kb::Action::Tab: return UI_KEY_TAB;
    case cardputer_kb::Action::Space: return UI_KEY_SPACE;
    }
    return UI_KEY_UNKNOWN;
}

static uint8_t mods_from_pressed(const std::vector<uint8_t> &down_keynums) {
    static constexpr uint8_t kKeynumFn = 29;
    static constexpr uint8_t kKeynumShift = 30;
    static constexpr uint8_t kKeynumCtrl = 43;
    static constexpr uint8_t kKeynumOpt = 44;
    static constexpr uint8_t kKeynumAlt = 45;

    uint8_t mods = 0;
    if (contains_keynum(down_keynums, kKeynumShift)) mods |= UI_MOD_SHIFT;
    if (contains_keynum(down_keynums, kKeynumCtrl)) mods |= UI_MOD_CTRL;
    if (contains_keynum(down_keynums, kKeynumAlt) || contains_keynum(down_keynums, kKeynumOpt)) mods |= UI_MOD_ALT;
    if (contains_keynum(down_keynums, kKeynumFn)) mods |= UI_MOD_FN;
    return mods;
}

static void q_send_best_effort(QueueHandle_t q, const ui_key_event_t &ev) {
    if (!q) return;
    (void)xQueueSend(q, &ev, 0);
}

static void kb_task_main(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;

    cardputer_kb::MatrixScanner scanner;
    scanner.init();

    std::vector<uint8_t> prev_pressed;
    bool prev_action_valid = false;
    cardputer_kb::Action prev_action = cardputer_kb::Action::Enter;

    while (true) {
        cardputer_kb::ScanSnapshot snap = scanner.scan();
        const std::vector<uint8_t> &down = snap.pressed_keynums;
        const uint8_t mods = mods_from_pressed(down);

        const cardputer_kb::Binding *active = cardputer_kb::decode_best(
            down, cardputer_kb::kCapturedBindingsM5Cardputer,
            sizeof(cardputer_kb::kCapturedBindingsM5Cardputer) / sizeof(cardputer_kb::kCapturedBindingsM5Cardputer[0]));

        if (active) {
            if (!prev_action_valid || prev_action != active->action) {
                ui_key_event_t ev = {};
                ev.kind = kind_from_action(active->action);
                ev.mods = mods;
                ev.keynum = 0;
                q_send_best_effort(q, ev);
            }
            prev_action_valid = true;
            prev_action = active->action;
        } else {
            prev_action_valid = false;
        }

        // Raw key edges (excluding modifiers and keys that are part of the active binding chord).
        for (auto keynum : down) {
            if (contains_keynum(prev_pressed, keynum)) continue;
            if (is_modifier_keynum(keynum)) continue;
            if (active && binding_contains_keynum(*active, keynum)) continue;

            const KeyValue *kv = key_value_for_keynum(keynum);
            if (!kv) continue;

            const bool shift = (mods & UI_MOD_SHIFT) != 0;
            const char *s = shift ? kv->second : kv->first;
            if (!s || !*s) continue;

            ui_key_event_t ev = {};
            ev.kind = UI_KEY_TEXT;
            ev.mods = mods;
            ev.keynum = keynum;
            strncpy(ev.text, s, sizeof(ev.text) - 1);
            ev.text[sizeof(ev.text) - 1] = '\0';

            // Normalize del->bksp to match existing JS/web conventions in this repo.
            if (strcmp(ev.text, "del") == 0) {
                strncpy(ev.text, "bksp", sizeof(ev.text) - 1);
                ev.text[sizeof(ev.text) - 1] = '\0';
            }

            q_send_best_effort(q, ev);
        }

        prev_pressed = down;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0066_KB_SCAN_PERIOD_MS));
    }
}

} // namespace

extern "C" void ui_kb_start(QueueHandle_t q) {
    // Stack is conservative: scanner is small, but we keep room for std::vector internals and C++.
    (void)xTaskCreate(&kb_task_main, "0066_kb", 4096, (void *)q, 6, nullptr);
}

