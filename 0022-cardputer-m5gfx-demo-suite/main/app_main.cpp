/*
 * Demo suite scaffold for ticket 0021-M5-GFX-DEMO:
 * - Cardputer display bring-up via M5GFX autodetect
 * - keyboard scan (Cardputer matrix) -> key events
 * - scene switcher + menu (A2) + HUD (A1) + perf overlay (B2)
 */

#include <inttypes.h>
#include <stdint.h>

#include <stdio.h>
#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "M5GFX.h"

#include "input_keyboard.h"
#include "ui_hud.h"
#include "ui_list_view.h"

static const char *TAG = "cardputer_m5gfx_demo_suite";

namespace {

constexpr int kHeaderH = 16;
constexpr int kFooterH = 16;

enum class SceneId {
    Menu,
    A1HudDemo,
    B2PerfDemo,
    E1TerminalDemo,
    B3ScreenshotDemo,
};

static const char *scene_name(SceneId id) {
    switch (id) {
        case SceneId::Menu: return "Home";
        case SceneId::A1HudDemo: return "A1 HUD overlay";
        case SceneId::B2PerfDemo: return "B2 Perf overlay";
        case SceneId::E1TerminalDemo: return "E1 Terminal";
        case SceneId::B3ScreenshotDemo: return "B3 Screenshot";
        default: return "Unknown";
    }
}

static SceneId next_scene(SceneId cur, bool backwards) {
    static const SceneId kOrder[] = {
        SceneId::Menu,
        SceneId::A1HudDemo,
        SceneId::B2PerfDemo,
        SceneId::E1TerminalDemo,
        SceneId::B3ScreenshotDemo,
    };

    int idx = 0;
    for (int i = 0; i < (int)(sizeof(kOrder) / sizeof(kOrder[0])); i++) {
        if (kOrder[i] == cur) {
            idx = i;
            break;
        }
    }

    int n = (int)(sizeof(kOrder) / sizeof(kOrder[0]));
    idx = backwards ? (idx - 1 + n) % n : (idx + 1) % n;
    return kOrder[idx];
}

static NavKey nav_key_from_event(const KeyEvent &ev, bool *out_is_nav) {
    *out_is_nav = true;
    if (ev.key == "w" || ev.key == "W" || ev.key == "k" || ev.key == "K") {
        return NavKey::Up;
    }
    if (ev.key == "s" || ev.key == "S" || ev.key == "j" || ev.key == "J") {
        return NavKey::Down;
    }
    if (ev.key == "," || ev.key == "<") {
        return NavKey::PageUp;
    }
    if (ev.key == "." || ev.key == ">") {
        return NavKey::PageDown;
    }
    if (ev.key == "enter") {
        return NavKey::Enter;
    }
    *out_is_nav = false;
    return NavKey::Enter;
}

static void render_placeholder(M5Canvas &body, const char *title) {
    body.fillRect(0, 0, body.width(), body.height(), TFT_BLACK);
    body.drawRect(0, 0, body.width(), body.height(), TFT_DARKGREY);
    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString(title, 6, 8);
    body.setTextColor(TFT_DARKGREY, TFT_BLACK);
    body.drawString("TODO: implement demo module", 6, 28);
    body.drawString("Del: back   Tab: next   Shift+Tab: prev", 6, 46);
    body.drawString("H: toggle HUD   F: toggle perf", 6, 64);
}

static bool hud_equals(const HudState &a, const HudState &b) {
    return a.screen_name == b.screen_name && a.heap_free == b.heap_free && a.dma_free == b.dma_free &&
           a.fps == b.fps && a.frame_ms == b.frame_ms;
}

} // namespace

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32, esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "display init via M5GFX autodetect...");
    display.init();
    display.setColorDepth(16);

    const int w = (int)display.width();
    const int h = (int)display.height();
    ESP_LOGI(TAG, "display ready: width=%d height=%d", w, h);

    M5Canvas header(&display);
    M5Canvas footer(&display);
    M5Canvas body(&display);

    header.setPsram(false);
    footer.setPsram(false);
    body.setPsram(false);

    header.setColorDepth(16);
    footer.setColorDepth(16);
    body.setColorDepth(16);

    if (!header.createSprite(w, kHeaderH)) {
        ESP_LOGE(TAG, "header createSprite failed (%dx%d)", w, kHeaderH);
        return;
    }
    if (!footer.createSprite(w, kFooterH)) {
        ESP_LOGE(TAG, "footer createSprite failed (%dx%d)", w, kFooterH);
        return;
    }
    const int body_h = h - kHeaderH - kFooterH;
    if (!body.createSprite(w, body_h)) {
        ESP_LOGE(TAG, "body createSprite failed (%dx%d)", w, body_h);
        return;
    }

    CardputerKeyboard keyboard;
    if (keyboard.init() != ESP_OK) {
        ESP_LOGE(TAG, "keyboard init failed");
        return;
    }

    struct MenuItem {
        const char *label;
        SceneId scene;
    };

    const std::vector<MenuItem> menu_items = {
        {"A2 List view + selection (menu pattern)", SceneId::Menu},
        {"A1 Status bar + HUD overlay", SceneId::A1HudDemo},
        {"B2 Frame-time / perf overlay", SceneId::B2PerfDemo},
        {"E1 Terminal / log console", SceneId::E1TerminalDemo},
        {"B3 Screenshot to serial", SceneId::B3ScreenshotDemo},
    };

    ListView list;
    list.set_bounds(0, 0, w, body_h);
    list.set_style(ListViewStyle{});
    {
        std::vector<std::string> labels;
        labels.reserve(menu_items.size());
        for (const auto &it : menu_items) {
            labels.emplace_back(it.label);
        }
        list.set_items(std::move(labels));
    }

    SceneId scene = SceneId::Menu;

    bool body_dirty = true;
    bool header_dirty = true;
    bool footer_dirty = true;
    bool hud_enabled = true;
    bool perf_enabled = true;

    PerfTracker perf;
    HudState hud{};
    HudState last_hud{};

    int64_t last_hud_update_us = 0;
    int64_t last_footer_update_us = 0;

    while (true) {
        const int64_t loop_start_us = esp_timer_get_time();

        bool selection_opened = false;
        bool pushed_any = false;
        int64_t present_start_us = -1;

        for (const auto &ev : keyboard.poll()) {
            if (ev.key == "h" || ev.key == "H") {
                hud_enabled = !hud_enabled;
                header_dirty = true;
                continue;
            }
            if (ev.key == "f" || ev.key == "F") {
                perf_enabled = !perf_enabled;
                footer_dirty = true;
                continue;
            }

            if (ev.key == "tab") {
                scene = next_scene(scene, ev.shift);
                body_dirty = true;
                header_dirty = true;
                continue;
            }

            if (ev.key == "del") {
                if (scene != SceneId::Menu) {
                    scene = SceneId::Menu;
                    body_dirty = true;
                    header_dirty = true;
                } else {
                    ESP_LOGI(TAG, "back (del) pressed");
                }
                continue;
            }

            bool is_nav = false;
            NavKey k = nav_key_from_event(ev, &is_nav);
            if (is_nav && scene == SceneId::Menu) {
                if (list.on_key(k)) {
                    body_dirty = true;
                }
                if (k == NavKey::Enter) {
                    selection_opened = true;
                }
                continue;
            }
        }

        if (selection_opened) {
            int idx = list.selected_index();
            if (idx >= 0 && idx < (int)menu_items.size()) {
                scene = menu_items[(size_t)idx].scene;
                ESP_LOGI(TAG, "open: idx=%d scene=%s", idx, scene_name(scene));
                body_dirty = true;
                header_dirty = true;
            }
        }

        const int64_t after_update_us = esp_timer_get_time();

        uint32_t render_us = 0;
        if (body_dirty) {
            const int64_t r0 = esp_timer_get_time();
            if (scene == SceneId::Menu) {
                list.render(body);
            } else {
                render_placeholder(body, scene_name(scene));
            }
            render_us += (uint32_t)(esp_timer_get_time() - r0);
            if (present_start_us < 0) {
                present_start_us = esp_timer_get_time();
            }
            body.pushSprite(0, kHeaderH);
            body_dirty = false;
            pushed_any = true;
        }

        const int64_t now_us = esp_timer_get_time();
        if (now_us - last_hud_update_us >= 250000) {
            last_hud_update_us = now_us;

            perf.sample_memory(esp_get_free_heap_size(), (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));
            const auto &p = perf.snapshot();
            hud.screen_name = scene_name(scene);
            hud.heap_free = p.heap_free;
            hud.dma_free = p.dma_free;
            hud.frame_ms = (int)(p.avg_total_us / 1000);
            hud.fps = (p.avg_total_us > 0) ? (int)(1000000 / p.avg_total_us) : 0;

            if (!hud_equals(hud, last_hud)) {
                header_dirty = true;
                last_hud = hud;
            }
        }

        if (now_us - last_footer_update_us >= 250000) {
            last_footer_update_us = now_us;
            footer_dirty = true;
        }

        uint32_t footer_bg = TFT_BLACK;
        uint32_t footer_fg = TFT_WHITE;

        if (header_dirty) {
            const int64_t r0 = esp_timer_get_time();
            if (hud_enabled) {
                draw_header_bar(header, hud, TFT_DARKGREY, TFT_WHITE);
            } else {
                header.fillRect(0, 0, header.width(), header.height(), TFT_BLACK);
            }
            render_us += (uint32_t)(esp_timer_get_time() - r0);
            if (present_start_us < 0) {
                present_start_us = esp_timer_get_time();
            }
            header.pushSprite(0, 0);
            header_dirty = false;
            pushed_any = true;
        }

        if (footer_dirty) {
            const int64_t r0 = esp_timer_get_time();
            if (perf_enabled) {
                draw_perf_bar(footer, perf.snapshot(), footer_bg, footer_fg);
            } else {
                footer.fillRect(0, 0, footer.width(), footer.height(), TFT_BLACK);
            }
            render_us += (uint32_t)(esp_timer_get_time() - r0);
            if (present_start_us < 0) {
                present_start_us = esp_timer_get_time();
            }
            footer.pushSprite(0, h - kFooterH);
            footer_dirty = false;
            pushed_any = true;
        }

        uint32_t present_us = 0;
        if (pushed_any) {
            display.waitDMA();
            const int64_t p1 = esp_timer_get_time();
            present_us = (present_start_us >= 0) ? (uint32_t)(p1 - present_start_us) : 0U;

            FrameTimingsUs t{};
            t.update_us = (uint32_t)(after_update_us - loop_start_us);
            t.render_us = render_us;
            t.present_us = present_us;
            t.total_us = t.update_us + t.render_us + t.present_us;
            perf.on_frame_present(t);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
