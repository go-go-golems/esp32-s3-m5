#include "input_keyboard.h"

#include <algorithm>
#include <cstring>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "driver/gpio.h"

static const char *TAG = "cardputer_keyboard";

namespace {

constexpr int kKbOutPins[3] = {8, 9, 11};

constexpr int kKbInPinsPrimary[7] = {13, 15, 3, 4, 5, 6, 7};
constexpr int kKbInPinsAlt[7] = {1, 2, 3, 4, 5, 6, 7};

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

static inline void kb_set_output(uint8_t out_bits3) {
    out_bits3 &= 0x07;
    gpio_set_level((gpio_num_t)kKbOutPins[0], (out_bits3 & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)kKbOutPins[1], (out_bits3 & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)kKbOutPins[2], (out_bits3 & 0x04) ? 1 : 0);
}

static inline uint8_t kb_get_input_mask(const int in_pins[7]) {
    // Input pins are pulled up; a pressed key reads low -> we invert to build a 7-bit pressed mask.
    uint8_t mask = 0;
    for (int i = 0; i < 7; i++) {
        int level = gpio_get_level((gpio_num_t)in_pins[i]);
        if (level == 0) {
            mask |= (uint8_t)(1U << i);
        }
    }
    return mask;
}

static int kb_scan_pressed(CardputerKeyboard::KeyPos *out_keys, int out_cap, bool *io_use_alt_in01,
                           bool *io_allow_autodetect_in01) {
    int count = 0;

    for (int scan_state = 0; scan_state < 8; scan_state++) {
        kb_set_output((uint8_t)scan_state);

        uint8_t in_mask = 0;
        if (*io_use_alt_in01) {
            in_mask = kb_get_input_mask(kKbInPinsAlt);
        } else {
            in_mask = kb_get_input_mask(kKbInPinsPrimary);
        }

        if (*io_allow_autodetect_in01) {
            // If the primary wiring shows no activity, also check the alternate IN0/IN1 pins.
            // If we see activity there, switch permanently and log once.
            if (!*io_use_alt_in01 && in_mask == 0) {
                uint8_t alt = kb_get_input_mask(kKbInPinsAlt);
                if (alt != 0) {
                    *io_use_alt_in01 = true;
                    in_mask = alt;
                    ESP_LOGW(TAG, "keyboard autodetect: switching IN0/IN1 to alt pins [%d,%d] (was [%d,%d])",
                             kKbInPinsAlt[0], kKbInPinsAlt[1], kKbInPinsPrimary[0], kKbInPinsPrimary[1]);
                }
            }
        }

        if (in_mask == 0) {
            continue;
        }

        // For each asserted input bit, map to x/y based on vendor chart.
        for (int j = 0; j < 7; j++) {
            if ((in_mask & (1U << j)) == 0) {
                continue;
            }

            int x = (scan_state > 3) ? (2 * j) : (2 * j + 1);
            int y_base = (scan_state > 3) ? (scan_state - 4) : scan_state; // 0..3
            int y = (-y_base) + 3;                                          // flip to match "picture"

            if (x < 0 || x > 13 || y < 0 || y > 3) {
                continue;
            }

            if (count < out_cap) {
                out_keys[count].x = x;
                out_keys[count].y = y;
                count++;
            }
        }
    }

    kb_set_output(0);
    return count;
}

static bool pos_eq(const CardputerKeyboard::KeyPos &a, const CardputerKeyboard::KeyPos &b) {
    return a.x == b.x && a.y == b.y;
}

static bool pos_contains(const std::vector<CardputerKeyboard::KeyPos> &v, const CardputerKeyboard::KeyPos &pos) {
    for (const auto &p : v) {
        if (pos_eq(p, pos)) {
            return true;
        }
    }
    return false;
}

static const KeyValue &key_value_at(const CardputerKeyboard::KeyPos &pos) {
    return kKeyMap[pos.y][pos.x];
}

static bool is_modifier(const char *k) {
    return strcmp(k, "shift") == 0 || strcmp(k, "fn") == 0 || strcmp(k, "ctrl") == 0 || strcmp(k, "alt") == 0 ||
           strcmp(k, "opt") == 0;
}

} // namespace

esp_err_t CardputerKeyboard::init() {
    gpio_config_t out_cfg = {};
    out_cfg.pin_bit_mask = (1ULL << kKbOutPins[0]) | (1ULL << kKbOutPins[1]) | (1ULL << kKbOutPins[2]);
    out_cfg.mode = GPIO_MODE_OUTPUT;
    out_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    out_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    out_cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&out_cfg), TAG, "gpio_config(out)");

    gpio_config_t in_cfg = {};
    in_cfg.pin_bit_mask = 0;
    for (int i = 0; i < 7; i++) {
        in_cfg.pin_bit_mask |= (1ULL << kKbInPinsPrimary[i]);
    }
    in_cfg.mode = GPIO_MODE_INPUT;
    in_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    in_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    in_cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&in_cfg), TAG, "gpio_config(in_primary)");

    gpio_config_t in_alt_cfg = in_cfg;
    in_alt_cfg.pin_bit_mask = 0;
    in_alt_cfg.pin_bit_mask |= (1ULL << kKbInPinsAlt[0]) | (1ULL << kKbInPinsAlt[1]);
    ESP_RETURN_ON_ERROR(gpio_config(&in_alt_cfg), TAG, "gpio_config(in_alt01)");

    kb_set_output(0);
    inited_ = true;
    return ESP_OK;
}

std::vector<KeyEvent> CardputerKeyboard::poll() {
    std::vector<KeyEvent> events;
    if (!inited_) {
        return events;
    }

    CardputerKeyboard::KeyPos down_buf[32] = {};
    int down_n = kb_scan_pressed(down_buf, (int)(sizeof(down_buf) / sizeof(down_buf[0])), &use_alt_in01_,
                                 &allow_autodetect_in01_);

    std::vector<KeyPos> down;
    down.reserve((size_t)down_n);
    for (int i = 0; i < down_n; i++) {
        down.push_back(down_buf[i]);
    }

    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool fn = false;
    for (const auto &pos : down) {
        const auto &kv = key_value_at(pos);
        if (strcmp(kv.first, "shift") == 0) {
            shift = true;
        } else if (strcmp(kv.first, "ctrl") == 0) {
            ctrl = true;
        } else if (strcmp(kv.first, "alt") == 0) {
            alt = true;
        } else if (strcmp(kv.first, "fn") == 0) {
            fn = true;
        }
    }

    // Emit "edge" events: keys that are down now but were not down previously.
    for (const auto &pos : down) {
        if (pos_contains(prev_down_, pos)) {
            continue;
        }

        const auto &kv = key_value_at(pos);
        if (is_modifier(kv.first)) {
            continue;
        }

        KeyEvent ev;
        ev.shift = shift;
        ev.ctrl = ctrl;
        ev.alt = alt;
        ev.fn = fn;
        ev.key = shift ? kv.second : kv.first;
        events.push_back(std::move(ev));
    }

    prev_down_ = std::move(down);
    return events;
}
