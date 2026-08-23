// SPDX-License-Identifier: MIT
// ESP-62 Phase 3 — on-screen UI for the CoreS3 QRCode scanner.
#include "qr_ui.h"

#include <M5Unified.h>

#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "qr_module.h"

static const char *kTag = "qr_ui";
static constexpr int kHistory = 6;

struct UIState {
    QRModule *module;
    bool scanning;
    QRCodeM14::TriggerMode mode;
    char firmware[40];
    char last_code[256];
    char history[kHistory][80];
    int hist_count;
    bool dirty;
};

static const char *mode_name(QRCodeM14::TriggerMode m) {
    switch (m) {
        case QRCodeM14::KEY: return "KEY";
        case QRCodeM14::CONTINUOUS: return "CONT";
        case QRCodeM14::AUTO: return "AUTO";
        case QRCodeM14::PULSE: return "PULSE";
        case QRCodeM14::MOTION: return "MOTION";
        default: return "?";
    }
}

static QRCodeM14::TriggerMode next_mode(QRCodeM14::TriggerMode mode) {
    switch (mode) {
        case QRCodeM14::KEY: return QRCodeM14::AUTO;
        case QRCodeM14::AUTO: return QRCodeM14::CONTINUOUS;
        case QRCodeM14::CONTINUOUS: return QRCodeM14::PULSE;
        case QRCodeM14::PULSE: return QRCodeM14::MOTION;
        default: return QRCodeM14::KEY;
    }
}

static bool mode_scans_automatically(QRCodeM14::TriggerMode mode) {
    return mode == QRCodeM14::AUTO || mode == QRCodeM14::CONTINUOUS ||
           mode == QRCodeM14::MOTION;
}

static void draw(M5Canvas &c, const UIState &st) {
    c.clear(TFT_BLACK);
    // top bar
    c.fillRect(0, 0, c.width(), 28, TFT_NAVY);
    c.setTextColor(TFT_WHITE, TFT_NAVY);
    c.setFont(&fonts::Font2);
    c.setCursor(6, 8);
    c.printf("QR MODE: %s", mode_name(st.mode));
    c.setCursor(c.width() - 76, 8);
    c.print(st.scanning ? "[active]" : "[idle]");
    // firmware line
    c.setFont(&fonts::Font0);
    c.setTextColor(TFT_CYAN, TFT_BLACK);
    c.setCursor(6, 32);
    if (st.firmware[0]) {
        c.printf("fw=%s", st.firmware);
    } else {
        c.print("UART OFFLINE: scanner may be in USB mode");
        c.setCursor(6, 43);
        c.print("Scan Serial Communication config: 21424000");
    }

    // center: last code, large yellow, word-wrapped
    c.setTextColor(TFT_YELLOW, TFT_BLACK);
    c.setFont(&fonts::Font2);
    c.setCursor(6, 60);
    if (st.last_code[0]) {
        // crude word-wrap: print and let M5GFX wrap by spaces
        c.setTextWrap(true);
        c.print(st.last_code);
        c.setTextWrap(false);
    } else {
        c.setTextColor(TFT_DARKGREY, TFT_BLACK);
        c.print("Aim at a code...");
    }

    // bottom: recent history
    c.setTextColor(TFT_WHITE, TFT_BLACK);
    c.setFont(&fonts::Font0);
    c.setCursor(6, 170);
    c.print("Recent:");
    for (int i = 0; i < st.hist_count; ++i) {
        int row = i;
        c.setCursor(6, 184 + row * 10);
        c.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        c.printf("%.*s", (int)sizeof(st.history[0]) - 1, st.history[i]);
    }

    // footer
    c.fillRect(0, c.height() - 14, c.width() / 2, 14, TFT_DARKGREY);
    c.fillRect(c.width() / 2, c.height() - 14, c.width() / 2, 14, TFT_LIGHTGREY);
    c.setTextColor(TFT_BLACK, TFT_DARKGREY);
    c.setCursor(4, c.height() - 11);
    c.print("LEFT: trigger");
    c.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    c.setCursor(c.width() / 2 + 4, c.height() - 11);
    c.print("RIGHT: next mode");
}

static void push_history(UIState &st, const char *code) {
    // shift down, drop oldest if full
    if (st.hist_count >= kHistory) {
        memmove(st.history[0], st.history[1], sizeof(st.history[0]) * (kHistory - 1));
        st.hist_count = kHistory - 1;
    }
    strncpy(st.history[st.hist_count], code, sizeof(st.history[0]) - 1);
    st.history[st.hist_count][sizeof(st.history[0]) - 1] = 0;
    st.hist_count++;
}

static void ui_task(void *arg) {
    UIState *st = static_cast<UIState *>(arg);
    M5Canvas canvas(&M5.Display);
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.setTextWrap(false);

    while (true) {
        M5.update();
        auto touch = M5.Touch.getDetail();
        if (touch.wasClicked()) {
            if (touch.x < M5.Display.width() / 2) {
                bool queued = st->module->requestHardwareTriggerPulse();
                if (queued) st->scanning = true;
                st->dirty = true;
                ESP_LOGI(kTag, "hardware trigger pulse: %s",
                         queued ? "queued" : "queue-full");
            } else {
                QRCodeM14::TriggerMode requested = next_mode(st->mode);
                bool queued = st->module->requestTriggerMode(requested);
                if (queued) {
                    st->mode = requested;
                    st->scanning = mode_scans_automatically(requested);
                }
                st->dirty = true;
                ESP_LOGI(kTag, "mode=%s: %s", mode_name(requested),
                         queued ? "queued" : "queue-full");
            }
        }
        // Drain scan results only when module initialization created the
        // queue. The UI remains usable and reports "no reply" without it.
        QueueHandle_t results = st->module->resultQueue();
        ScanResult r;
        while (results && xQueueReceive(results, &r, 0) == pdTRUE) {
            strncpy(st->last_code, r.text, sizeof(st->last_code) - 1);
            st->last_code[sizeof(st->last_code) - 1] = 0;
            push_history(*st, st->last_code);
            st->scanning = mode_scans_automatically(st->mode);
            st->dirty = true;
            ESP_LOGI(kTag, "code: %s", st->last_code);
        }
        if (st->dirty) {
            draw(canvas, *st);
            canvas.pushSprite(0, 0);
            st->dirty = false;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void QRUI::start(QRModule &module, const char *firmware,
                 bool configured_ready) {
    static UIState st;
    st.module = &module;
    st.scanning = false;
    st.mode = QRCodeM14::KEY;
    st.firmware[0] = 0;
    st.last_code[0] = 0;
    st.hist_count = 0;
    st.dirty = true;
    if (firmware && firmware[0]) {
        strncpy(st.firmware, firmware, sizeof(st.firmware) - 1);
        st.firmware[sizeof(st.firmware) - 1] = 0;
    }
    ESP_LOGI(kTag, "start: firmware=%s configured_ready=%d, creating UI task",
             st.firmware[0] ? st.firmware : "(no reply)", configured_ready);
    xTaskCreate(ui_task, "qr_ui", 6144, &st, 4, nullptr);
}
