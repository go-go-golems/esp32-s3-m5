#include "input_keyboard.h"

#include <cstring>
#include <string_view>

#include "cardputer_kb/bindings_m5cardputer_captured.h"
#include "cardputer_kb/layout.h"

namespace {

struct KeyValue {
    const char *first;
    const char *second;
};

// Key legend matches the vendor HAL's "picture" coordinate system (4 rows x 14 columns).
// Derived from M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h.
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

static std::string_view key_for_action(cardputer_kb::Action a) {
    switch (a) {
    case cardputer_kb::Action::NavUp: return "up";
    case cardputer_kb::Action::NavDown: return "down";
    case cardputer_kb::Action::NavLeft: return "left";
    case cardputer_kb::Action::NavRight: return "right";
    case cardputer_kb::Action::Back: return "esc";
    case cardputer_kb::Action::Del: return "del";
    case cardputer_kb::Action::Enter: return "enter";
    case cardputer_kb::Action::Tab: return "tab";
    case cardputer_kb::Action::Space: return "space";
    }
    return "";
}

} // namespace

esp_err_t CardputerKeyboard::init() {
    esp_err_t err = scanner_.init();
    if (err != ESP_OK) {
        return err;
    }
    inited_ = true;
    return ESP_OK;
}

std::vector<KeyEvent> CardputerKeyboard::poll() {
    std::vector<KeyEvent> events;
    if (!inited_) {
        return events;
    }

    cardputer_kb::ScanSnapshot snap = scanner_.scan();
    const std::vector<uint8_t> &down_keynums = snap.pressed_keynums;

    static constexpr uint8_t kKeynumFn = 29;
    static constexpr uint8_t kKeynumShift = 30;
    static constexpr uint8_t kKeynumCtrl = 43;
    static constexpr uint8_t kKeynumOpt = 44;
    static constexpr uint8_t kKeynumAlt = 45;

    const bool shift = contains_keynum(down_keynums, kKeynumShift);
    const bool ctrl = contains_keynum(down_keynums, kKeynumCtrl);
    const bool alt = contains_keynum(down_keynums, kKeynumAlt) || contains_keynum(down_keynums, kKeynumOpt);
    const bool fn = contains_keynum(down_keynums, kKeynumFn);

    const cardputer_kb::Binding *active = cardputer_kb::decode_best(
        down_keynums, cardputer_kb::kCapturedBindingsM5Cardputer,
        sizeof(cardputer_kb::kCapturedBindingsM5Cardputer) / sizeof(cardputer_kb::kCapturedBindingsM5Cardputer[0]));

    if (active) {
        if (!prev_action_valid_ || prev_action_ != active->action) {
            KeyEvent ev;
            ev.shift = shift;
            ev.ctrl = ctrl;
            ev.alt = alt;
            ev.fn = fn;
            ev.key = std::string(key_for_action(active->action));
            events.push_back(std::move(ev));
        }
        prev_action_valid_ = true;
        prev_action_ = active->action;
    } else {
        prev_action_valid_ = false;
    }

    // Emit "edge" events for raw keys (excluding any key that is part of the active binding chord).
    for (auto keynum : down_keynums) {
        if (contains_keynum(prev_pressed_keynums_, keynum)) {
            continue;
        }
        if (is_modifier_keynum(keynum)) {
            continue;
        }
        if (active && binding_contains_keynum(*active, keynum)) {
            continue;
        }

        const KeyValue *kv = key_value_for_keynum(keynum);
        if (!kv) {
            continue;
        }

        std::string key = shift ? kv->second : kv->first;
        if (key == "del") {
            key = "bksp";
        }

        KeyEvent ev;
        ev.shift = shift;
        ev.ctrl = ctrl;
        ev.alt = alt;
        ev.fn = fn;
        ev.key = std::move(key);
        events.push_back(std::move(ev));
    }

    prev_pressed_keynums_ = down_keynums;
    return events;
}
