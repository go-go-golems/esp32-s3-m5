// SPDX-License-Identifier: MIT
// ESP-62 CoreS3 + Module13.2 QRCode scanner — app_main
//
// Boot: M5Unified -> scanner module (PI4IOE5V6408 power/TRIG + UART1) ->
// on-screen UI task (owns the display + buttons) -> USB Serial/JTAG console.
// The UI task is the only thing that calls M5.update()/draws.

#include <M5Unified.h>

#include "esp_chip_info.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "qr_console.h"
#include "qr_module.h"
#include "qr_ui.h"

static const char *kTag = "cores3_qr";
static QRModule g_qr;
static QRConsole g_console;
static QRUI g_ui;

extern "C" void app_main(void) {
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(kTag, "boot: CoreS3 QRCode scanner (ESP-62)");
    ESP_LOGI(kTag, "chip: model=%d rev=%d cores=%d", (int)chip.model,
             (int)chip.revision, (int)chip.cores);

    ESP_LOGI(kTag, "step: M5.begin");
    M5.begin();
    M5.Display.init();
    M5.Display.setRotation(1);  // landscape 320x240
    M5.Display.setBrightness(80);
    ESP_LOGI(kTag, "step: display ready");

    // Start the USB console FIRST so the REPL is always available even if the
    // module init blocks (e.g. I2C/UART hang, missing 12V).
    g_console.start(g_qr);
    ESP_LOGI(kTag, "step: console started");

    // Prove TTL serial health before changing scanner policy. Configuration is
    // applied only after a valid firmware reply and every ACK-producing write
    // must succeed before the UI claims AUTO readiness.
    char fw[64] = {0};
    bool auto_ready = false;
    if (g_qr.begin()) {
        bool responsive = g_qr.getInfo(0xC1, fw, sizeof(fw));
        ESP_LOGI(kTag, "module ready, firmware=%s",
                 responsive ? fw : "(no reply)");
        if (responsive) {
            QRCodeM14::CmdResult uart = g_qr.setModeUart();
            QRCodeM14::CmdResult fill =
                g_qr.setFillLightMode(QRCodeM14::FILL_ON_DECODE);
            QRCodeM14::CmdResult pos =
                g_qr.setPosLightMode(QRCodeM14::POS_ON_DECODE);
            QRCodeM14::CmdResult mode =
                g_qr.setTriggerMode(QRCodeM14::AUTO);
            auto_ready = uart == QRCodeM14::OK && fill == QRCodeM14::OK &&
                         pos == QRCodeM14::OK && mode == QRCodeM14::OK;
            ESP_LOGI(kTag,
                     "AUTO config: uart=%s fill=%s pos=%s mode=%s ready=%d",
                     QRCodeM14::resultName(uart),
                     QRCodeM14::resultName(fill),
                     QRCodeM14::resultName(pos),
                     QRCodeM14::resultName(mode), auto_ready);
        }
    } else {
        ESP_LOGE(kTag, "module init failed -- 12V power / DIP switch (UART) / stack");
    }

    g_ui.start(g_qr, fw, auto_ready);  // UI owns M5.update() + display
    ESP_LOGI(kTag, "step: UI started");

    ESP_LOGI(kTag, "ready -- UI + console started");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
