#include "kb_scan.h"

#include <algorithm>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "kb_scan";

static constexpr int kOutPins[3] = {8, 9, 11};
static constexpr int kInPinsPrimary[7] = {13, 15, 3, 4, 5, 6, 7};
static constexpr int kInPinsAltIn01[7] = {1, 2, 3, 4, 5, 6, 7};

static inline void kb_set_output(uint8_t out_bits3) {
    out_bits3 &= 0x07;
    gpio_set_level((gpio_num_t)kOutPins[0], (out_bits3 & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)kOutPins[1], (out_bits3 & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)kOutPins[2], (out_bits3 & 0x04) ? 1 : 0);
}

static inline uint8_t kb_get_input_mask(const int pins[7]) {
    uint8_t mask = 0;
    for (int i = 0; i < 7; i++) {
        int level = gpio_get_level((gpio_num_t)pins[i]);
        if (level == 0) {
            mask |= (uint8_t)(1U << i);
        }
    }
    return mask;
}

static inline uint8_t keynum_from_pos(const KeyPos &p) {
    if (p.x < 0 || p.x > 13 || p.y < 0 || p.y > 3) {
        return 0;
    }
    return (uint8_t)((p.y * 14) + (p.x + 1));
}

void KbScanner::init() {
    for (int i = 0; i < 3; i++) {
        int pin = kOutPins[i];
        gpio_reset_pin((gpio_num_t)pin);
        gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_PULLDOWN);
        gpio_set_level((gpio_num_t)pin, 0);
    }

    auto init_in = [](const int pins[7]) {
        for (int i = 0; i < 7; i++) {
            int pin = pins[i];
            gpio_reset_pin((gpio_num_t)pin);
            gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
        }
    };
    init_in(kInPinsPrimary);
    init_in(kInPinsAltIn01);

    kb_set_output(0);
}

ScanSnapshot KbScanner::scan() {
    ScanSnapshot snap;
    snap.use_alt_in01 = use_alt_in01_;

    for (int scan_state = 0; scan_state < 8; scan_state++) {
        kb_set_output((uint8_t)scan_state);
        esp_rom_delay_us(10);

        uint8_t in_mask = 0;
        if (use_alt_in01_) {
            in_mask = kb_get_input_mask(kInPinsAltIn01);
        } else {
            in_mask = kb_get_input_mask(kInPinsPrimary);
            if (in_mask == 0) {
                uint8_t alt = kb_get_input_mask(kInPinsAltIn01);
                if (alt != 0) {
                    use_alt_in01_ = true;
                    snap.use_alt_in01 = true;
                    in_mask = alt;
                    ESP_LOGW(TAG, "autodetect: switching IN0/IN1 to alt pins [1,2] (was [13,15])");
                }
            }
        }

        if (in_mask == 0) {
            continue;
        }

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
            snap.pressed.push_back(KeyPos{.x = x, .y = y});
        }
    }

    kb_set_output(0);

    std::sort(snap.pressed.begin(), snap.pressed.end(), [](const KeyPos &a, const KeyPos &b) {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });
    snap.pressed.erase(std::unique(snap.pressed.begin(), snap.pressed.end()), snap.pressed.end());

    snap.pressed_keynums.reserve(snap.pressed.size());
    for (const auto &p : snap.pressed) {
        uint8_t kn = keynum_from_pos(p);
        if (kn != 0) {
            snap.pressed_keynums.push_back(kn);
        }
    }

    return snap;
}

