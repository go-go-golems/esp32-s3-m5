#include <stdio.h>

#include <string>

#include "esp_log.h"
#include "esp_timer.h"

#include "M5GFX.h"
#include "lgfx/v1/LGFX_Sprite.hpp"

#include "cardputer_kb/layout.h"
#include "cardputer_kb/scanner.h"

static const char *TAG = "kb_scancode_cal";

static std::string join_keynums(const std::vector<uint8_t> &nums) {
    std::string s;
    for (size_t i = 0; i < nums.size(); i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", (unsigned)nums[i]);
        if (!s.empty()) s += ",";
        s += buf;
    }
    return s;
}

static bool pos_is_pressed(const std::vector<cardputer_kb::KeyPos> &pressed, int x, int y) {
    for (const auto &p : pressed) {
        if (p.x == x && p.y == y) {
            return true;
        }
    }
    return false;
}

extern "C" void app_main(void) {
    m5gfx::M5GFX display;
    display.init();

    const int w = display.width();
    const int h = display.height();

    lgfx::LGFX_Sprite screen(&display);
    screen.createSprite(w, h);
    screen.setFont(&fonts::Font0);
    screen.setTextSize(1);

    cardputer_kb::MatrixScanner kb;
    kb.init();

    std::vector<uint8_t> last_keynums;
    int64_t last_render_us = 0;

    while (true) {
        cardputer_kb::ScanSnapshot snap = kb.scan();
        if (snap.pressed_keynums != last_keynums) {
            last_keynums = snap.pressed_keynums;
            if (!last_keynums.empty()) {
                ESP_LOGI(TAG, "pressed keynums=[%s] pinset=%s", join_keynums(last_keynums).c_str(),
                         snap.use_alt_in01 ? "alt_in01(1,2)" : "primary(13,15)");
            }
        }

        const int64_t now_us = esp_timer_get_time();
        if (now_us - last_render_us < 33000) { // ~30 fps
            vTaskDelay(1);
            continue;
        }
        last_render_us = now_us;

        const int top_h = 16;
        const int bottom_h = 16;
        const int grid_h = h - top_h - bottom_h;
        const int cell_w = w / 14;
        const int cell_h = grid_h / 4;

        screen.fillScreen(TFT_BLACK);

        screen.setTextColor(TFT_WHITE, TFT_BLACK);
        screen.setCursor(2, 2);
        screen.printf("KB scancodes  pinset=%s", snap.use_alt_in01 ? "alt(1,2)" : "primary(13,15)");

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 14; x++) {
                const int x0 = x * cell_w;
                const int y0 = top_h + y * cell_h;
                const bool pressed = pos_is_pressed(snap.pressed, x, y);

                uint32_t bg = pressed ? TFT_DARKGREEN : TFT_BLACK;
                uint32_t fg = pressed ? TFT_WHITE : TFT_DARKGREY;
                screen.fillRect(x0 + 1, y0 + 1, cell_w - 2, cell_h - 2, bg);
                screen.drawRect(x0, y0, cell_w, cell_h, TFT_DARKGREY);

                const int keynum = (y * 14) + (x + 1);
                screen.setTextColor(fg, bg);
                screen.setCursor(x0 + 2, y0 + 2);
                screen.printf("%d", keynum);
                screen.setCursor(x0 + 2, y0 + 10);
                screen.printf("%s", cardputer_kb::legend_for_xy(x, y));
            }
        }

        screen.fillRect(0, h - bottom_h, w, bottom_h, TFT_BLACK);
        screen.setTextColor(TFT_WHITE, TFT_BLACK);
        screen.setCursor(2, h - bottom_h + 2);
        if (!snap.pressed_keynums.empty()) {
            screen.printf("pressed: [%s]", join_keynums(snap.pressed_keynums).c_str());
        } else {
            screen.printf("pressed: []");
        }

        screen.pushSprite(0, 0);
        display.waitDMA();

        vTaskDelay(1);
    }
}
