/*
 * ESP32-S3 tutorial 0030:
 * Cardputer console + esp_event bus demo:
 * - keyboard key edges are posted as esp_event events
 * - an esp_console REPL can post events into the same event bus
 * - a UI task dispatches the event loop and renders received events on screen
 *
 * Design choice: the event loop is created with task_name=NULL, and the UI task calls
 * esp_event_loop_run() with a small tick budget. This keeps display ownership in one task.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <deque>
#include <string>
#include <vector>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_console.h"
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

static const char *TAG = "cardputer_console_eventbus_0030";

ESP_EVENT_DEFINE_BASE(CARDPUTER_BUS_EVENT);

enum : int32_t {
    BUS_EVT_KB_KEY = 1,
    BUS_EVT_KB_ACTION = 2,
    BUS_EVT_RAND = 3,
    BUS_EVT_HEARTBEAT = 4,
    BUS_EVT_CONSOLE_POST = 5,
    BUS_EVT_CONSOLE_CLEAR = 6,
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

typedef struct {
    int64_t ts_us;
    char msg[96];
} demo_console_post_event_t;

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
    uint32_t console_posts = 0;
    uint32_t post_drops = 0;

    demo_heartbeat_event_t last_hb{};

    UiState() : screen(&display) {}
};

static void lines_trim(UiState &ui) {
    while (ui.lines.size() > (size_t)CONFIG_TUTORIAL_0030_UI_MAX_LINES) {
        ui.lines.pop_front();
        ui.dropped_lines++;
    }
    if (ui.lines.empty()) {
        ui.lines.push_back(std::string());
    }
}

static void lines_append(UiState &ui, std::string s) {
    if (ui.paused) return;
    ui.lines.push_back(std::move(s));
    lines_trim(ui);
    ui.dirty = true;
}

static void lines_clear(UiState &ui) {
    ui.lines.clear();
    ui.dropped_lines = 0;
    ui.scrollback = 0;
    ui.dirty = true;
}

static std::string mods_to_string(uint8_t mods) {
    std::string s;
    if (mods & DEMO_MOD_FN) s += "Fn+";
    if (mods & DEMO_MOD_CTRL) s += "Ctrl+";
    if (mods & DEMO_MOD_ALT) s += "Alt+";
    if (mods & DEMO_MOD_SHIFT) s += "Shift+";
    if (!s.empty()) s.pop_back(); // remove trailing '+'
    return s;
}

static void render_ui(UiState &ui) {
    if (!ui.dirty) return;
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
    ui.screen.drawString("0030: esp_event bus + esp_console (Cardputer)", 2, 0 * line_h);

    char hdr[180];
    snprintf(hdr,
             sizeof(hdr),
             "kb=%" PRIu32 " act=%" PRIu32 " con=%" PRIu32 " rand=%" PRIu32 " hb=%" PRIu32 " drops=%" PRIu32
             " heap=%" PRIu32 " dma=%" PRIu32,
             ui.kb_key_events,
             ui.kb_action_events,
             ui.console_posts,
             ui.rand_events,
             ui.heartbeat_events,
             ui.post_drops,
             heap_free,
             dma_free);
    ui.screen.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    ui.screen.drawString(hdr, 2, 1 * line_h);

    ui.screen.setTextColor(TFT_DARKGREY, TFT_BLACK);
    ui.screen.drawString("Fn+W/S scroll  Fn+Space pause  Del clear  (console: evt ...)", 2, 2 * line_h);

    const int y0 = header_rows * line_h;
    const int rows = std::max(1, (int)((ui.screen.height() - y0) / line_h));

    int start = (int)ui.lines.size() - rows - ui.scrollback;
    if (start < 0) start = 0;
    int end = std::min((int)ui.lines.size(), start + rows);

    ui.screen.setTextColor(TFT_WHITE, TFT_BLACK);
    for (int i = start; i < end; i++) {
        const int row = i - start;
        ui.screen.drawString(ui.lines[(size_t)i].c_str(), 2, y0 + row * line_h);
    }

    ui.screen.pushSprite(0, 0);
    ui.display.waitDMA();
}

static void demo_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto *ui = (UiState *)arg;
    if (event_base != CARDPUTER_BUS_EVENT || !ui) return;

    if (event_id == BUS_EVT_KB_KEY) {
        ui->kb_key_events++;
        const demo_kb_key_event_t *ev = (const demo_kb_key_event_t *)event_data;
        if (!ev) return;

        int x = -1;
        int y = -1;
        cardputer_kb::xy_from_keynum(ev->keynum, &x, &y);
        const char *legend = cardputer_kb::legend_for_xy(x, y);

        char line[140];
        const std::string mods = mods_to_string(ev->modifiers);
        snprintf(line,
                 sizeof(line),
                 "[kb] keynum=%u (%s) mods=%s",
                 (unsigned)ev->keynum,
                 legend ? legend : "?",
                 mods.empty() ? "-" : mods.c_str());
        lines_append(*ui, line);

        // UI controls handled on receive side (same spirit as 0028):
        // - Del with no modifiers clears the log
        // - Fn+Space toggles pause
        if (ev->keynum == 14 && ev->modifiers == 0) { // del
            lines_clear(*ui);
            return;
        }
        if (ev->keynum == 56 && (ev->modifiers & DEMO_MOD_FN)) { // Fn+Space
            ui->paused = !ui->paused;
            ui->dirty = true;
            return;
        }
        return;
    }

    if (event_id == BUS_EVT_KB_ACTION) {
        ui->kb_action_events++;
        const demo_kb_action_event_t *ev = (const demo_kb_action_event_t *)event_data;
        if (!ev) return;

        const char *name = cardputer_kb::action_name((cardputer_kb::Action)ev->action);
        char line[160];
        snprintf(line, sizeof(line), "[kb] action=%s", name ? name : "?");
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

    if (event_id == BUS_EVT_CONSOLE_POST) {
        ui->console_posts++;
        const demo_console_post_event_t *ev = (const demo_console_post_event_t *)event_data;
        if (!ev) return;
        std::string s = "[con] ";
        s += ev->msg;
        lines_append(*ui, std::move(s));
        return;
    }

    if (event_id == BUS_EVT_CONSOLE_CLEAR) {
        lines_clear(*ui);
        return;
    }

    if (event_id == BUS_EVT_RAND) {
        ui->rand_events++;
        const demo_rand_event_t *ev = (const demo_rand_event_t *)event_data;
        if (!ev) return;

        char line[120];
        snprintf(line, sizeof(line), "[rnd] p%u value=%" PRIu32, ev->producer_id, ev->value);
        lines_append(*ui, line);
        return;
    }

    if (event_id == BUS_EVT_HEARTBEAT) {
        ui->heartbeat_events++;
        const demo_heartbeat_event_t *ev = (const demo_heartbeat_event_t *)event_data;
        if (!ev) return;
        ui->last_hb = *ev;

        char line[160];
        snprintf(line,
                 sizeof(line),
                 "[hb] heap=%" PRIu32 " dma=%" PRIu32 " drops=%" PRIu32,
                 ev->heap_free,
                 ev->dma_free,
                 s_post_drops.load());
        lines_append(*ui, line);
        return;
    }
}

static void post_drop(void) {
    s_post_drops.fetch_add(1);
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
            down,
            cardputer_kb::kCapturedBindingsM5Cardputer,
            sizeof(cardputer_kb::kCapturedBindingsM5Cardputer) / sizeof(cardputer_kb::kCapturedBindingsM5Cardputer[0]));

        if (active) {
            if (!prev_action_valid || prev_action != active->action) {
                demo_kb_action_event_t ev = {
                    .ts_us = esp_timer_get_time(),
                    .action = (uint8_t)active->action,
                    .modifiers = mods,
                };
                if (esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_KB_ACTION, &ev, sizeof(ev), 0) != ESP_OK) {
                    post_drop();
                }
            }
            prev_action_valid = true;
            prev_action = active->action;
        } else {
            prev_action_valid = false;
        }

        // Emit edge events for raw keys (excluding modifiers and keys that are part of the active binding chord).
        for (auto keynum : down) {
            if (contains_u8(prev_pressed, keynum)) continue;
            if (is_modifier_keynum(keynum)) continue;
            if (active && binding_contains_keynum(*active, keynum)) continue;

            demo_kb_key_event_t ev = {
                .ts_us = esp_timer_get_time(),
                .keynum = keynum,
                .modifiers = mods,
            };
            if (esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_KB_KEY, &ev, sizeof(ev), 0) != ESP_OK) {
                post_drop();
            }
        }

        prev_pressed = down;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0030_KB_SCAN_PERIOD_MS));
    }
}

static void rand_producer_task(void *arg) {
    const uint32_t producer_id = (uint32_t)(uintptr_t)arg;
    while (true) {
        const uint32_t v = esp_random();
        demo_rand_event_t ev = {.ts_us = esp_timer_get_time(), .producer_id = (uint8_t)producer_id, .value = v};
        if (esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_RAND, &ev, sizeof(ev), 0) != ESP_OK) {
            post_drop();
        }

        const uint32_t lo = (uint32_t)CONFIG_TUTORIAL_0030_RAND_MIN_PERIOD_MS;
        const uint32_t hi = (uint32_t)CONFIG_TUTORIAL_0030_RAND_MAX_PERIOD_MS;
        const uint32_t span = (hi > lo) ? (hi - lo) : 0;
        const uint32_t jitter = (span > 0) ? (esp_random() % (span + 1)) : 0;
        vTaskDelay(pdMS_TO_TICKS(lo + jitter));
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
        if (esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_HEARTBEAT, &ev, sizeof(ev), 0) != ESP_OK) {
            post_drop();
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0030_HEARTBEAT_PERIOD_MS));
    }
}

// ---- Console ----

static int cmd_evt(int argc, char **argv) {
    if (!s_loop) {
        printf("event loop not ready\n");
        return 1;
    }

    if (argc < 2) {
        printf("usage: evt post <text> | evt spam <n> | evt clear | evt status\n");
        return 1;
    }

    if (strcmp(argv[1], "clear") == 0) {
        (void)esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_CONSOLE_CLEAR, NULL, 0, 0);
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        printf("drops=%" PRIu32 "\n", s_post_drops.load());
        return 0;
    }

    if (strcmp(argv[1], "post") == 0) {
        if (argc < 3) {
            printf("usage: evt post <text>\n");
            return 1;
        }
        demo_console_post_event_t ev = {.ts_us = esp_timer_get_time(), .msg = {0}};
        strlcpy(ev.msg, argv[2], sizeof(ev.msg));
        esp_err_t err = esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_CONSOLE_POST, &ev, sizeof(ev), 0);
        if (err != ESP_OK) {
            printf("post failed: %s\n", esp_err_to_name(err));
            post_drop();
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "spam") == 0) {
        int n = 10;
        if (argc >= 3) {
            n = atoi(argv[2]);
        }
        if (n <= 0 || n > 1000) {
            printf("invalid n (1..1000)\n");
            return 1;
        }
        for (int i = 0; i < n; i++) {
            demo_console_post_event_t ev = {.ts_us = esp_timer_get_time(), .msg = {0}};
            snprintf(ev.msg, sizeof(ev.msg), "spam %d/%d", i + 1, n);
            if (esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, BUS_EVT_CONSOLE_POST, &ev, sizeof(ev), 0) != ESP_OK) {
                post_drop();
                printf("post failed at %d\n", i);
                return 1;
            }
        }
        printf("ok (%d)\n", n);
        return 0;
    }

    printf("usage: evt post <text> | evt spam <n> | evt clear | evt status\n");
    return 1;
}

static void console_start(void) {
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "bus> ";

    esp_err_t err = ESP_ERR_NOT_SUPPORTED;

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    const char *backend = "USB Serial/JTAG";
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    err = esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl);
    const char *backend = "USB CDC";
#elif CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    err = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    const char *backend = "UART";
#else
    const char *backend = "none";
#endif

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_console not started (backend=%s, err=%s)", backend, esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();

    esp_console_cmd_t cmd = {};
    cmd.command = "evt";
    cmd.help = "Post/control demo events: evt post|spam|clear|status";
    cmd.func = &cmd_evt;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over %s", backend);
}

extern "C" void app_main(void) {
    esp_event_loop_args_t args = {
        .queue_size = CONFIG_TUTORIAL_0030_EVENT_QUEUE_SIZE,
        .task_name = NULL,         // UI task dispatches via esp_event_loop_run()
        .task_priority = 0,
        .task_stack_size = 0,
        .task_core_id = 0,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&args, &s_loop));

    UiState ui{};
    ESP_LOGI(TAG, "display init via M5GFX autodetect...");
    ui.display.begin();
    ui.display.setRotation(1);

    ui.screen.setColorDepth(16);
    ui.screen.createSprite(ui.display.width(), ui.display.height());

    lines_append(ui, "Keyboard events are posted by keyboard_task -> esp_event_post_to().");
    lines_append(ui, "Console events are posted by esp_console cmd handler -> esp_event_post_to().");
    lines_append(ui, "Try: evt post hello");

    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, CARDPUTER_BUS_EVENT, ESP_EVENT_ANY_ID, &demo_event_handler, &ui));

    xTaskCreate(&keyboard_task, "kb", 4096, NULL, 5, NULL);
    xTaskCreate(&heartbeat_task, "hb", 3072, NULL, 2, NULL);
    for (int i = 0; i < CONFIG_TUTORIAL_0030_RAND_PRODUCERS; i++) {
        xTaskCreate(&rand_producer_task, "rnd", 3072, (void *)(uintptr_t)i, 1, NULL);
    }

    console_start();

    const int64_t frame_us = 1000000LL / std::max(1, (int)CONFIG_TUTORIAL_0030_UI_TARGET_FPS);
    int64_t last = 0;
    while (true) {
        (void)esp_event_loop_run(s_loop, (TickType_t)CONFIG_TUTORIAL_0030_DISPATCH_TICKS);
        render_ui(ui);

        const int64_t now = esp_timer_get_time();
        const int64_t elapsed = now - last;
        if (elapsed < frame_us) {
            vTaskDelay(pdMS_TO_TICKS((uint32_t)((frame_us - elapsed) / 1000)));
        }
        last = now;
    }
}
