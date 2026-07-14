// PPA Dial — scene switcher for Four Audio PPA modules.
// Ticket M5DIAL-PPA-CONTROL, design doc §5 (0103-m5dial-ppa-dial).
#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "mdns.h"

#include "buzzer.h"
#include "config_store.h"
#include "input_events.h"
#include "lvgl_port_m5dial.h"
#include "m5dial_board.h"
#include "ppa_client.h"
#include "scene_model.h"
#include "ui_screens.h"
#include "web_setup.h"
#include "wifi_mgr.h"

namespace {
const char *TAG = "ppa_dial";

constexpr uint32_t kUiTaskStackSize = 12288;
constexpr uint32_t kIoTaskStackSize = 4096;
constexpr UBaseType_t kUiTaskPriority = 5;
constexpr UBaseType_t kIoTaskPriority = 6;
constexpr uint32_t kUiTickMs = 10;
constexpr uint32_t kIoPollMs = 8;
constexpr size_t kInputQueueLength = 32;
constexpr size_t kPpaQueueLength = 16;
constexpr int kEncoderCountsPerDetent = 2; // attachHalfQuad: 2 counts per detent
constexpr int64_t kResultShowMs = 900;

struct AppContext {
    ppa_dial::M5DialBoard board;
    QueueHandle_t input_queue = nullptr;
    QueueHandle_t ppa_queue = nullptr;
    std::vector<PpaScene> scenes;
};

void io_task(void *arg) {
    auto *ctx = static_cast<AppContext *>(arg);
    ctx->board.set_button_irq_task(xTaskGetCurrentTaskHandle());
    while (true) {
        ctx->board.poll(ctx->input_queue);
        const TickType_t wait_ticks = pdMS_TO_TICKS(kIoPollMs);
        ulTaskNotifyTake(pdTRUE, wait_ticks == 0 ? 1 : wait_ticks);
    }
}

enum class AppState { kBooting, kApMode, kNoScenes, kCarousel };

struct Controller {
    AppContext *ctx = nullptr;
    AppState state = AppState::kBooting;
    int selected = 0;
    int active = -1;
    int encoder_acc = 0;
    bool recall_in_flight = false;
    int64_t result_shown_at_ms = -1;
    bool mdns_started = false;

    int64_t now_ms() const { return esp_timer_get_time() / 1000; }

    void refresh_online() {
        if (state != AppState::kCarousel) return;
        const int total = static_cast<int>(ctx->scenes[selected].actions.size());
        ui_carousel_set_online(ppa_client_online_count(selected), total);
    }

    void show_scene(int slide_dir) {
        const auto &s = ctx->scenes[selected];
        ui_carousel_update(selected, static_cast<int>(ctx->scenes.size()), s.name.c_str(),
                           selected == active, active, slide_dir);
        refresh_online();
    }

    void enter_state(AppState next) {
        state = next;
        switch (next) {
        case AppState::kBooting:
            ui_show_status("PPA Dial", "verbinde mit WLAN ...", "", false, true);
            break;
        case AppState::kApMode:
            ui_show_status("Setup", "WLAN: PPA-Dial / ppadial123", "http://192.168.4.1",
                           true, false);
            break;
        case AppState::kNoScenes:
            ui_show_status("Keine Szenen", "presets.json einfuegen:", "http://ppadial.local",
                           false, false);
            break;
        case AppState::kCarousel:
            ui_show_carousel();
            show_scene(0);
            break;
        }
    }

    void poll_connectivity() {
        const WifiState w = wifi_mgr_state();
        if (state == AppState::kBooting) {
            if (w == WifiState::kApMode) enter_state(AppState::kApMode);
            else if (w == WifiState::kConnected) on_connected();
        } else if (state != AppState::kApMode && w == WifiState::kApMode) {
            enter_state(AppState::kApMode);
        }
    }

    void on_connected() {
        if (!mdns_started) {
            if (mdns_init() == ESP_OK) {
                mdns_hostname_set("ppadial");
                mdns_started = true;
                ESP_LOGI(TAG, "mDNS: ppadial.local");
            }
        }
        enter_state(ctx->scenes.empty() ? AppState::kNoScenes : AppState::kCarousel);
    }

    void handle_input(const ppa_dial::InputEvent &ev) {
        if (state != AppState::kCarousel) return;
        using ppa_dial::InputEventType;
        if (ev.type == InputEventType::kEncoderDelta) {
            if (recall_in_flight) return;
            encoder_acc += ev.value;
            while (encoder_acc >= kEncoderCountsPerDetent) {
                encoder_acc -= kEncoderCountsPerDetent;
                step(1);
            }
            while (encoder_acc <= -kEncoderCountsPerDetent) {
                encoder_acc += kEncoderCountsPerDetent;
                step(-1);
            }
        } else if (ev.type == InputEventType::kButtonShortPress) {
            if (recall_in_flight || result_shown_at_ms >= 0) return;
            recall_in_flight = true;
            buzzer_tone(3000, 40);
            ui_activation_show(ctx->scenes[selected].name.c_str());
            ppa_client_request_recall(selected);
        }
    }

    void step(int dir) {
        const int n = static_cast<int>(ctx->scenes.size());
        selected = (selected + dir + n) % n;
        buzzer_tone(2400, 15);
        show_scene(dir);
    }

    void handle_ppa(const PpaEvent &ev) {
        switch (ev.type) {
        case PpaEventType::kDiscoveryUpdate:
            refresh_online();
            break;
        case PpaEventType::kRecallProgress:
            if (recall_in_flight) ui_activation_progress(ev.actions_done, ev.actions_total);
            break;
        case PpaEventType::kRecallDone:
            if (!recall_in_flight) break;
            if (ev.ok) active = ev.scene_index;
            ui_activation_result(ev.ok, ev.actions_done, ev.actions_total);
            buzzer_tone(ev.ok ? 4000 : 800, ev.ok ? 60 : 250);
            recall_in_flight = false;
            result_shown_at_ms = now_ms();
            break;
        }
    }

    void tick() {
        poll_connectivity();
        if (result_shown_at_ms >= 0 && now_ms() - result_shown_at_ms > kResultShowMs) {
            result_shown_at_ms = -1;
            ui_activation_hide();
            if (state == AppState::kCarousel) show_scene(0);
        }
    }
};

void ui_task(void *arg) {
    auto *ctx = static_cast<AppContext *>(arg);

    ppa_dial::LvglPortM5DialConfig lvgl_cfg{};
    lvgl_cfg.buffer_lines = 40;
    lvgl_cfg.tick_ms = 2;
    if (!ppa_dial::lvgl_port_m5dial_init(ctx->board.display(), lvgl_cfg)) {
        ESP_LOGE(TAG, "LVGL port init failed");
        vTaskDelete(nullptr);
        return;
    }
    ui_init();

    Controller controller;
    controller.ctx = ctx;
    controller.enter_state(AppState::kBooting);

    while (true) {
        ppa_dial::InputEvent input;
        while (xQueueReceive(ctx->input_queue, &input, 0) == pdTRUE)
            controller.handle_input(input);
        PpaEvent ppa_ev;
        while (xQueueReceive(ctx->ppa_queue, &ppa_ev, 0) == pdTRUE)
            controller.handle_ppa(ppa_ev);
        controller.tick();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(kUiTickMs));
    }
}
} // namespace

extern "C" void app_main(void) {
    static AppContext app;

    if (!config_store_init()) {
        ESP_LOGE(TAG, "NVS init failed");
        return;
    }
    std::string ssid, pass, presets;
    config_store_load(ssid, pass, presets);
    if (!scene_model_parse(presets.c_str(), app.scenes))
        ESP_LOGW(TAG, "stored presets.json failed to parse");
    ESP_LOGI(TAG, "loaded %d scenes, ssid='%s'", static_cast<int>(app.scenes.size()),
             ssid.c_str());

    app.input_queue = xQueueCreate(kInputQueueLength, sizeof(ppa_dial::InputEvent));
    app.ppa_queue = xQueueCreate(kPpaQueueLength, sizeof(PpaEvent));
    if (app.input_queue == nullptr || app.ppa_queue == nullptr) {
        ESP_LOGE(TAG, "queue creation failed");
        return;
    }

    if (!app.board.init()) {
        ESP_LOGE(TAG, "board init failed");
        return;
    }
    buzzer_init();

    xTaskCreatePinnedToCore(ui_task, "ppa_ui", kUiTaskStackSize, &app, kUiTaskPriority, nullptr, 1);
    xTaskCreatePinnedToCore(io_task, "ppa_io", kIoTaskStackSize, &app, kIoTaskPriority, nullptr, 0);

    wifi_mgr_start(ssid.c_str(), pass.c_str());
    web_setup_start();
    ppa_client_start(app.ppa_queue);
    ppa_client_set_scenes(app.scenes);
}
