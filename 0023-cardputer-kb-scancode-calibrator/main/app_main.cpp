#include <stdio.h>

#include <string>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "M5GFX.h"
#include "lgfx/v1/LGFX_Sprite.hpp"

#include "cardputer_kb/layout.h"
#include "cardputer_kb/scanner.h"

#include "wizard.h"

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

enum class UiMode {
    Live,
    Wizard,
};

struct ButtonEdge {
    bool prev_down = false;
    int64_t last_change_us = 0;
    bool edge_pressed = false;
};

static void button_init_gpio0() {
    gpio_reset_pin(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
}

static void button_poll_gpio0(ButtonEdge &b, int64_t now_us) {
    bool down = gpio_get_level(GPIO_NUM_0) == 0;
    b.edge_pressed = false;

    static constexpr int64_t kDebounceUs = 30 * 1000;
    if (down != b.prev_down && (now_us - b.last_change_us) >= kDebounceUs) {
        b.last_change_us = now_us;
        b.prev_down = down;
        if (down) {
            b.edge_pressed = true;
        }
    }
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

    button_init_gpio0();
    ButtonEdge g0;

    UiMode mode = UiMode::Live;
    CalibrationWizard wizard;
    wizard.reset();

    std::vector<uint8_t> last_keynums;
    int64_t last_render_us = 0;
    cardputer_kb::ScanSnapshot prev_scan;

    while (true) {
        const int64_t now_us = esp_timer_get_time();
        button_poll_gpio0(g0, now_us);
        if (g0.edge_pressed) {
            mode = (mode == UiMode::Live) ? UiMode::Wizard : UiMode::Live;
            ESP_LOGI(TAG, "mode=%s (GPIO0 toggle)", mode == UiMode::Live ? "live" : "wizard");
            if (mode == UiMode::Wizard) {
                wizard.reset();
            }
        }

        cardputer_kb::ScanSnapshot snap = kb.scan();

        if (mode == UiMode::Wizard) {
            if (wizard.update(snap, prev_scan, now_us)) {
                if (wizard.phase() == WizardPhase::Done && wizard.printed_config()) {
                    std::string cfg = wizard.config_json();
                    ESP_LOGI(TAG, "CFG_BEGIN %u", (unsigned)cfg.size());
                    printf("%s", cfg.c_str());
                    printf("CFG_END\n");
                    fflush(stdout);
                }
            }
        } else {
            if (snap.pressed_keynums != last_keynums) {
                last_keynums = snap.pressed_keynums;
                if (!last_keynums.empty()) {
                    ESP_LOGI(TAG, "pressed keynums=[%s] pinset=%s", join_keynums(last_keynums).c_str(),
                             snap.use_alt_in01 ? "alt_in01(1,2)" : "primary(13,15)");
                }
            }
        }

        if (now_us - last_render_us < 33000) { // ~30 fps
            vTaskDelay(1);
            prev_scan = snap;
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
        if (mode == UiMode::Live) {
            screen.printf("KB scancodes (live) pinset=%s", snap.use_alt_in01 ? "alt(1,2)" : "primary(13,15)");
        } else {
            screen.printf("KB scancodes (wizard) G0 toggles");
        }

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
        if (mode == UiMode::Wizard) {
            const WizardBinding *cur = wizard.current();
            const std::string &status = wizard.status_text();
            if (wizard.phase() == WizardPhase::Done) {
                screen.printf("DONE: Enter prints config");
            } else if (cur) {
                if (wizard.phase() == WizardPhase::CapturedAwaitConfirm) {
                    screen.printf("%s captured:[%s] Enter=ok Del=redo Tab=skip", cur->prompt.c_str(),
                                  join_keynums(cur->required_keynums).c_str());
                } else {
                    if (!status.empty()) {
                        screen.printf("%s %s", cur->prompt.c_str(), status.c_str());
                    } else {
                        screen.printf("%s pressed:[%s]", cur->prompt.c_str(), join_keynums(snap.pressed_keynums).c_str());
                    }
                }
            } else {
                screen.printf("wizard: (no current)");
            }
        } else {
            if (!snap.pressed_keynums.empty()) {
                screen.printf("pressed: [%s]", join_keynums(snap.pressed_keynums).c_str());
            } else {
                screen.printf("pressed: []");
            }
        }

        screen.pushSprite(0, 0);
        display.waitDMA();

        prev_scan = snap;
        vTaskDelay(1);
    }
}
