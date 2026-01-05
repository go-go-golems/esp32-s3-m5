/*
 * ESP32-S3 tutorial 0028:
 * Cardputer esp_event demo:
 * - keyboard key edges are posted as esp_event events
 * - parallel tasks post random and heartbeat events
 * - a UI task dispatches the event loop and renders received events on screen
 *
 * Design choice: the event loop is created with task_name=NULL, and the UI task calls
 * esp_event_loop_run() with a small tick budget. This keeps display ownership in one task.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <algorithm>
#include <atomic>
#include <deque>
#include <string>
#include <vector>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "M5GFX.h"

#include "cardputer_kb/bindings.h"
#include "cardputer_kb/bindings_m5cardputer_captured.h"
#include "cardputer_kb/layout.h"
#include "cardputer_kb/scanner.h"

static const char *TAG = "cardputer_esp_event_demo_0028";

ESP_EVENT_DEFINE_BASE(CARDPUTER_DEMO_EVENT);

enum : int32_t {
    CARDPUTER_DEMO_EVT_KB_KEY = 1,
    CARDPUTER_DEMO_EVT_KB_ACTION = 2,
    CARDPUTER_DEMO_EVT_RAND = 3,
    CARDPUTER_DEMO_EVT_HEARTBEAT = 4,
};

enum : uint8_t {
    DEMO_MOD_SHIFT = 1 << 0,
    DEMO_MOD_CTRL = 1 << 1,
    DEMO_MOD_ALT = 1 << 2,
    DEMO_MOD_FN = 1 << 3,
};

typedef struct {
    int64_t ts_us;
    uint8_t keynum;     // vendor-style 1..56 (cardputer_kb)
    uint8_t modifiers;  // DEMO_MOD_*
} demo_kb_key_event_t;

typedef struct {
    int64_t ts_us;
    uint8_t action;     // cardputer_kb::Action
    uint8_t modifiers;  // DEMO_MOD_*
} demo_kb_action_event_t;

typedef struct {
    int64_t ts_us;
    uint8_t producer_id;
    uint32_t value;
} demo_rand_event_t;

typedef struct {
    int64_t ts_us;
    uint32_t heap_free;
    uint32_t dma_free;
} demo_heartbeat_event_t;

static esp_event_loop_handle_t s_loop = NULL;
static std::atomic<uint32_t> s_post_drops{0};

static bool contains_u8(const std::vector<uint8_t> &v, uint8_t x) {
    for (auto it : v) {
        if (it == x) return true;
    }
    return false;
}

static bool binding_contains_keynum(const cardputer_kb::Binding &b, uint8_t keynum) {
    for (uint8_t i = 0; i < b.n; i++) {
        if (b.keynums[i] == keynum) return true;
    }
    return false;
}

static uint8_t modifiers_from_pressed(const std::vector<uint8_t> &pressed_keynums) {
    static constexpr uint8_t kKeynumFn = 29;
    static constexpr uint8_t kKeynumShift = 30;
    static constexpr uint8_t kKeynumCtrl = 43;
    static constexpr uint8_t kKeynumOpt = 44;
    static constexpr uint8_t kKeynumAlt = 45;

    uint8_t mods = 0;
    if (contains_u8(pressed_keynums, kKeynumShift)) mods |= DEMO_MOD_SHIFT;
    if (contains_u8(pressed_keynums, kKeynumCtrl)) mods |= DEMO_MOD_CTRL;
    if (contains_u8(pressed_keynums, kKeynumFn)) mods |= DEMO_MOD_FN;
    if (contains_u8(pressed_keynums, kKeynumAlt) || contains_u8(pressed_keynums, kKeynumOpt)) mods |= DEMO_MOD_ALT;
    return mods;
}

static bool is_modifier_keynum(uint8_t keynum) {
    static constexpr uint8_t kKeynumFn = 29;
    static constexpr uint8_t kKeynumShift = 30;
    static constexpr uint8_t kKeynumCtrl = 43;
    static constexpr uint8_t kKeynumOpt = 44;
    static constexpr uint8_t kKeynumAlt = 45;
    return keynum == kKeynumFn || keynum == kKeynumShift || keynum == kKeynumCtrl || keynum == kKeynumOpt ||
           keynum == kKeynumAlt;
}

struct UiState {
    m5gfx::M5GFX display{};
    M5Canvas screen;

    std::deque<std::string> lines{};
    size_t dropped_lines = 0;
    int scrollback = 0; // 0=follow tail; >0 scroll up by N lines

    bool paused = false;
    bool dirty = true;

    uint32_t kb_key_events = 0;
    uint32_t kb_action_events = 0;
    uint32_t rand_events = 0;
    uint32_t heartbeat_events = 0;
    uint32_t post_drops = 0;

    demo_heartbeat_event_t last_hb{};

    UiState() : screen(&display) {}
};

static void lines_trim(UiState &ui) {
    while (ui.lines.size() > (size_t)CONFIG_TUTORIAL_0028_UI_MAX_LINES) {
        ui.lines.pop_front();
        ui.dropped_lines++;
    }
    if (ui.lines.empty()) {
        ui.lines.push_back(std::string());
    }
}

static void lines_append(UiState &ui, std::string s) {
    if (ui.paused) {
        return;
    }
    ui.lines.push_back(std::move(s));
    lines_trim(ui);
    ui.dirty = true;
}

static std::string mods_to_string(uint8_t mods) {
    std::string s;
    if (mods & DEMO_MOD_FN) s += "Fn+";
    if (mods & DEMO_MOD_CTRL) s += "Ctrl+";
    if (mods & DEMO_MOD_ALT) s += "Alt+";
    if (mods & DEMO_MOD_SHIFT) s += "Shift+";
    if (!s.empty()) {
        s.pop_back(); // remove trailing '+'
    }
    return s;
}

static void render_ui(UiState &ui) {
    if (!ui.dirty) {
        return;
    }

    ui.dirty = false;

    ui.screen.fillScreen(TFT_BLACK);
    ui.screen.setTextSize(1, 1);
    ui.screen.setTextDatum(lgfx::textdatum_t::top_left);
    ui.screen.setTextWrap(false);

    const int line_h = ui.screen.fontHeight() + 1;
    const int header_rows = 3;

    const uint32_t heap_free = esp_get_free_heap_size();
    const uint32_t dma_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA);
    ui.post_drops = s_post_drops.load();

    ui.screen.setTextColor(TFT_WHITE, TFT_BLACK);
    ui.screen.drawString("0028: esp_event demo (Cardputer)", 2, 0 * line_h);

    char hdr[160];
    snprintf(hdr,
             sizeof(hdr),
             "kb=%" PRIu32 " act=%" PRIu32 " rand=%" PRIu32 " hb=%" PRIu32 " drops=%" PRIu32 " heap=%" PRIu32
             " dma=%" PRIu32,
             ui.kb_key_events,
             ui.kb_action_events,
             ui.rand_events,
             ui.heartbeat_events,
             ui.post_drops,
             heap_free,
             dma_free);
    ui.screen.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    ui.screen.drawString(hdr, 2, 1 * line_h);

    ui.screen.setTextColor(TFT_DARKGREY, TFT_BLACK);
    ui.screen.drawString("Fn+W/S scroll  Fn+Space pause  Del clear  Fn+1 back", 2, 2 * line_h);

    const int y0 = header_rows * line_h;
    const int rows = std::max(1, (int)((ui.screen.height() - y0) / line_h));

    const int total = (int)ui.lines.size();
    const int tail_start = std::max(0, total - rows);
    const int start = std::max(0, tail_start - ui.scrollback);

    ui.screen.setTextColor(TFT_GREEN, TFT_BLACK);
    for (int i = 0; i < rows; i++) {
        int idx = start + i;
        if (idx >= total) break;
        ui.screen.drawString(ui.lines[(size_t)idx].c_str(), 2, y0 + i * line_h);
    }

    if (ui.paused) {
        ui.screen.fillRect(0, ui.screen.height() - line_h - 2, ui.screen.width(), line_h + 2, TFT_DARKGREY);
        ui.screen.setTextColor(TFT_WHITE, TFT_DARKGREY);
        ui.screen.drawString("PAUSED (Fn+Space to resume)", 2, ui.screen.height() - line_h - 1);
    }

    ui.screen.pushSprite(0, 0);
    ui.display.waitDMA();
}

static void demo_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)event_base;
    UiState *ui = (UiState *)arg;

    if (event_id == CARDPUTER_DEMO_EVT_KB_KEY) {
        const demo_kb_key_event_t *ev = (const demo_kb_key_event_t *)event_data;
        ui->kb_key_events++;

        int x = -1;
        int y = -1;
        cardputer_kb::xy_from_keynum(ev->keynum, &x, &y);
        const char *legend = cardputer_kb::legend_for_xy(x, y);

        const std::string mods = mods_to_string(ev->modifiers);

        char line[160];
        if (!mods.empty()) {
            snprintf(line, sizeof(line), "[%8" PRIi64 "us] KB keynum=%u (%s) mods=%s", ev->ts_us, (unsigned)ev->keynum,
                     legend, mods.c_str());
        } else {
            snprintf(line, sizeof(line), "[%8" PRIi64 "us] KB keynum=%u (%s)", ev->ts_us, (unsigned)ev->keynum, legend);
        }
        lines_append(*ui, line);

        // UI controls (handled on receive side):
        // - Del with no modifiers clears the log
        // - Fn+Space toggles pause
        if (ev->keynum == 14 && ev->modifiers == 0) { // del
            ui->lines.clear();
            ui->dropped_lines = 0;
            ui->scrollback = 0;
            ui->dirty = true;
            return;
        }
        if (ev->keynum == 56 && (ev->modifiers & DEMO_MOD_FN)) { // Fn+Space
            ui->paused = !ui->paused;
            ui->dirty = true;
            return;
        }
        return;
    }

    if (event_id == CARDPUTER_DEMO_EVT_KB_ACTION) {
        const demo_kb_action_event_t *ev = (const demo_kb_action_event_t *)event_data;
        ui->kb_action_events++;

        const char *name = cardputer_kb::action_name((cardputer_kb::Action)ev->action);
        char line[128];
        snprintf(line, sizeof(line), "[%8" PRIi64 "us] KB action=%s", ev->ts_us, name);
        lines_append(*ui, line);

        // Use nav actions for scroll control.
        switch ((cardputer_kb::Action)ev->action) {
        case cardputer_kb::Action::NavUp:
            ui->scrollback++;
            ui->dirty = true;
            break;
        case cardputer_kb::Action::NavDown:
            ui->scrollback = std::max(0, ui->scrollback - 1);
            ui->dirty = true;
            break;
        case cardputer_kb::Action::NavLeft:
            ui->scrollback += 5;
            ui->dirty = true;
            break;
        case cardputer_kb::Action::NavRight:
            ui->scrollback = std::max(0, ui->scrollback - 5);
            ui->dirty = true;
            break;
        case cardputer_kb::Action::Back:
            ui->scrollback = 0;
            ui->dirty = true;
            break;
        default:
            break;
        }
        return;
    }

    if (event_id == CARDPUTER_DEMO_EVT_RAND) {
        const demo_rand_event_t *ev = (const demo_rand_event_t *)event_data;
        ui->rand_events++;

        char line[128];
        snprintf(line, sizeof(line), "[%8" PRIi64 "us] RAND#%u value=0x%08" PRIx32, ev->ts_us, (unsigned)ev->producer_id,
                 ev->value);
        lines_append(*ui, line);
        return;
    }

    if (event_id == CARDPUTER_DEMO_EVT_HEARTBEAT) {
        const demo_heartbeat_event_t *ev = (const demo_heartbeat_event_t *)event_data;
        ui->heartbeat_events++;
        ui->last_hb = *ev;

        char line[160];
        snprintf(line,
                 sizeof(line),
                 "[%8" PRIi64 "us] HEARTBEAT heap=%" PRIu32 " dma=%" PRIu32,
                 ev->ts_us,
                 ev->heap_free,
                 ev->dma_free);
        lines_append(*ui, line);
        return;
    }
}

static void keyboard_task(void *arg) {
    (void)arg;
    cardputer_kb::MatrixScanner kb;
    kb.init();

    std::vector<uint8_t> prev_pressed;
    bool prev_action_valid = false;
    cardputer_kb::Action prev_action = cardputer_kb::Action::Enter;

    while (true) {
        cardputer_kb::ScanSnapshot snap = kb.scan();
        const std::vector<uint8_t> &down = snap.pressed_keynums;
        const uint8_t mods = modifiers_from_pressed(down);

        const cardputer_kb::Binding *active = cardputer_kb::decode_best(
            down, cardputer_kb::kCapturedBindingsM5Cardputer,
            sizeof(cardputer_kb::kCapturedBindingsM5Cardputer) / sizeof(cardputer_kb::kCapturedBindingsM5Cardputer[0]));

        if (active) {
            if (!prev_action_valid || prev_action != active->action) {
                demo_kb_action_event_t ev = {
                    .ts_us = esp_timer_get_time(),
                    .action = (uint8_t)active->action,
                    .modifiers = mods,
                };
                esp_err_t err =
                    esp_event_post_to(s_loop, CARDPUTER_DEMO_EVENT, CARDPUTER_DEMO_EVT_KB_ACTION, &ev, sizeof(ev), 0);
                if (err != ESP_OK) {
                    s_post_drops.fetch_add(1);
                }
            }
            prev_action_valid = true;
            prev_action = active->action;
        } else {
            prev_action_valid = false;
        }

        // Emit "edge" events for raw keys (excluding modifiers and keys that are part of the active binding chord).
        for (auto keynum : down) {
            if (contains_u8(prev_pressed, keynum)) continue;
            if (is_modifier_keynum(keynum)) continue;
            if (active && binding_contains_keynum(*active, keynum)) continue;

            demo_kb_key_event_t ev = {
                .ts_us = esp_timer_get_time(),
                .keynum = keynum,
                .modifiers = mods,
            };

            esp_err_t err = esp_event_post_to(s_loop, CARDPUTER_DEMO_EVENT, CARDPUTER_DEMO_EVT_KB_KEY, &ev, sizeof(ev), 0);
            if (err != ESP_OK) {
                s_post_drops.fetch_add(1);
            }
        }

        prev_pressed = down;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0028_KB_SCAN_PERIOD_MS));
    }
}

static void rand_task(void *arg) {
    const uint8_t producer_id = (uint8_t)(uintptr_t)arg;

    while (true) {
        demo_rand_event_t ev = {
            .ts_us = esp_timer_get_time(),
            .producer_id = producer_id,
            .value = esp_random(),
        };

        esp_err_t err = esp_event_post_to(s_loop, CARDPUTER_DEMO_EVENT, CARDPUTER_DEMO_EVT_RAND, &ev, sizeof(ev), 0);
        if (err != ESP_OK) {
            s_post_drops.fetch_add(1);
        }

        const uint32_t min_ms = (uint32_t)CONFIG_TUTORIAL_0028_RAND_MIN_PERIOD_MS;
        const uint32_t max_ms = (uint32_t)CONFIG_TUTORIAL_0028_RAND_MAX_PERIOD_MS;
        const uint32_t span = (max_ms > min_ms) ? (max_ms - min_ms) : 0;
        const uint32_t delay_ms = min_ms + (span ? (esp_random() % (span + 1)) : 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static void heartbeat_task(void *arg) {
    (void)arg;
    while (true) {
        demo_heartbeat_event_t ev = {
            .ts_us = esp_timer_get_time(),
            .heap_free = esp_get_free_heap_size(),
            .dma_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA),
        };

        esp_err_t err =
            esp_event_post_to(s_loop, CARDPUTER_DEMO_EVENT, CARDPUTER_DEMO_EVT_HEARTBEAT, &ev, sizeof(ev), 0);
        if (err != ESP_OK) {
            s_post_drops.fetch_add(1);
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0028_HEARTBEAT_PERIOD_MS));
    }
}

static void ui_task(void *arg) {
    (void)arg;

    esp_event_loop_args_t args = {
        .queue_size = CONFIG_TUTORIAL_0028_EVENT_QUEUE_SIZE,
        .task_name = NULL,         // UI task dispatches via esp_event_loop_run()
        .task_priority = 0,        // ignored
        .task_stack_size = 0,      // ignored
        .task_core_id = 0,         // ignored
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&args, &s_loop));
    ESP_LOGI(TAG, "created app event loop=%p queue_size=%d", s_loop, (int)args.queue_size);

    UiState ui{};

    ESP_LOGI(TAG, "display init via M5GFX autodetect...");
    ui.display.init();
    ui.display.setColorDepth(16);
    const int w = (int)ui.display.width();
    const int h = (int)ui.display.height();
    ESP_LOGI(TAG, "display ready: width=%d height=%d", w, h);

    ui.screen.setPsram(false);
    ui.screen.setColorDepth(16);
    if (!ui.screen.createSprite(w, h)) {
        ESP_LOGE(TAG, "createSprite failed (%dx%d)", w, h);
        return;
    }

    ui.lines.push_back("Ready. Press keys; background tasks post random + heartbeat events.");
    ui.lines.push_back("Keyboard events are posted by keyboard_task -> esp_event_post_to().");
    ui.lines.push_back("");
    ui.dirty = true;

    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, CARDPUTER_DEMO_EVENT, ESP_EVENT_ANY_ID, &demo_event_handler, &ui));

    BaseType_t ok = xTaskCreate(keyboard_task, "kb", 4096, NULL, 8, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create keyboard task");
        return;
    }

    ok = xTaskCreate(heartbeat_task, "hb", 4096, NULL, 5, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create heartbeat task");
        return;
    }

    for (int i = 0; i < CONFIG_TUTORIAL_0028_RAND_PRODUCERS; i++) {
        char name[12];
        snprintf(name, sizeof(name), "rand%d", i);
        ok = xTaskCreate(rand_task, name, 4096, (void *)(uintptr_t)i, 5, NULL);
        if (ok != pdPASS) {
            ESP_LOGW(TAG, "failed to create rand task %d", i);
        }
    }

    const int64_t frame_period_us = (CONFIG_TUTORIAL_0028_UI_TARGET_FPS > 0)
                                        ? (1000000LL / (int64_t)CONFIG_TUTORIAL_0028_UI_TARGET_FPS)
                                        : 33333;
    int64_t last_frame_us = 0;

    while (true) {
        (void)esp_event_loop_run(s_loop, (TickType_t)CONFIG_TUTORIAL_0028_DISPATCH_TICKS);

        const int64_t now_us = esp_timer_get_time();
        if (last_frame_us == 0 || (now_us - last_frame_us) >= frame_period_us) {
            last_frame_us = now_us;
            render_ui(ui);
        }

        vTaskDelay(1);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32, esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    BaseType_t ok = xTaskCreate(ui_task, "ui", 8192, NULL, 10, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create ui task");
    }
}
