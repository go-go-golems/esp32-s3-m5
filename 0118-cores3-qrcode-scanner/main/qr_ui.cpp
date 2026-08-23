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

static void draw(M5Canvas &c, const UIState &st) {
    c.clear(TFT_BLACK);
    // top bar
    c.fillRect(0, 0, c.width(), 28, TFT_NAVY);
    c.setTextColor(TFT_WHITE, TFT_NAVY);
    c.setFont(&fonts::Font2);
    c.setCursor(6, 8);
    c.printf("QR %s  %s", st.scanning ? "SCAN" : "IDLE", mode_name(st.mode));
    c.setCursor(c.width() - 90, 8);
    c.print(st.scanning ? "[on]   " : "[off]  ");
    // firmware line
    c.setFont(&fonts::Font0);
    c.setTextColor(TFT_CYAN, TFT_BLACK);
    c.setCursor(6, 32);
    c.printf("fw=%s", st.firmware[0] ? st.firmware : "(no reply)");

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
    c.fillRect(0, c.height() - 14, c.width(), 14, TFT_DARKGREY);
    c.setTextColor(TFT_BLACK, TFT_DARKGREY);
    c.setCursor(4, c.height() - 11);
    c.print("BtnA: scan  BtnB: mode");
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
        // buttons
        if (M5.BtnA.wasClicked()) {
            st->scanning = !st->scanning;
            if (st->scanning) st->module->startScan();
            else st->module->stopScan();
            st->dirty = true;
            ESP_LOGI(kTag, "scan %s", st->scanning ? "start" : "stop");
        }
        if (M5.BtnB.wasClicked()) {
            // cycle: KEY -> CONTINUOUS -> AUTO -> PULSE -> MOTION -> KEY
            int m = st->mode;
            do { m = (m + 1) % 6; } while (m == 3);  // 3 unused
            st->mode = (QRCodeM14::TriggerMode)m;
            st->module->setTriggerMode(st->mode);
            st->dirty = true;
            ESP_LOGI(kTag, "mode=%s", mode_name(st->mode));
        }
        // drain scan results
        ScanResult r;
        while (xQueueReceive(st->module->resultQueue(), &r, 0) == pdTRUE) {
            strncpy(st->last_code, r.text, sizeof(st->last_code) - 1);
            st->last_code[sizeof(st->last_code) - 1] = 0;
            push_history(*st, st->last_code);
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

void QRUI::start(QRModule &module) {
    ESP_LOGI(kTag, "start: entering (will read firmware)");
    static UIState st;
    st.module = &module;
    st.scanning = false;
    st.mode = QRCodeM14::CONTINUOUS;
    st.firmware[0] = 0;
    st.last_code[0] = 0;
    st.hist_count = 0;
    st.dirty = true;
    // read firmware once for the top bar
    if (!module.getInfo(0xC1, st.firmware, sizeof(st.firmware))) {
        strncpy(st.firmware, "(no reply)", sizeof(st.firmware) - 1);
    }
    ESP_LOGI(kTag, "start: firmware=%s, creating UI task", st.firmware);
    xTaskCreate(ui_task, "qr_ui", 6144, &st, 4, nullptr);
}
