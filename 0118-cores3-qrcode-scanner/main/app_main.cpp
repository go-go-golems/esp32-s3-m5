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

    M5.begin();
    M5.Display.init();
    M5.Display.setRotation(1);  // landscape 320x240
    M5.Display.setBrightness(80);

    if (g_qr.begin()) {
        char fw[64] = {0};
        bool ok = g_qr.engine().getInfos(0xC1, fw, sizeof(fw));
        ESP_LOGI(kTag, "module ready, firmware=%s", ok ? fw : "(no reply)");
        g_qr.engine().setFillLightMode(QRCodeM14::FILL_ON_DECODE);
        g_qr.engine().setPosLightMode(QRCodeM14::POS_ON_DECODE);
        g_qr.engine().setTriggerMode(QRCodeM14::CONTINUOUS);
    } else {
        ESP_LOGE(kTag, "module init failed -- 12V power / DIP switch (UART) / stack");
    }

    g_ui.start(g_qr);       // UI task owns M5.update() + display
    g_console.start(g_qr);  // esp_console over USB Serial/JTAG

    ESP_LOGI(kTag, "ready -- UI + console started");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
