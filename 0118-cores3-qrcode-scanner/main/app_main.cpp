// SPDX-License-Identifier: MIT
// ESP-62 CoreS3 + Module13.2 QRCode scanner — app_main
//
// Phase 2: skeleton + display + scanner driver. Initializes M5Unified, the
// Module13.2 QRCode (PI4IOE5V6408 power/TRIG + UART1 to the M14-Pro engine),
// and an esp_console over USB Serial/JTAG with `qr status` (the on-device
// probe). The scan-result pump feeds a queue that Phase 3 will render.

#include <M5Unified.h>

#include <cstdio>

#include "esp_chip_info.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "qr_console.h"
#include "qr_module.h"

static const char *kTag = "cores3_qr";
static QRModule g_qr;
static QRConsole g_console;

static void draw_banner(M5Canvas &canvas, const char *state) {
    canvas.clear(TFT_BLACK);
    canvas.fillRect(0, 0, canvas.width(), 40, TFT_NAVY);
    canvas.setTextColor(TFT_WHITE, TFT_NAVY);
    canvas.setFont(&fonts::Font2);
    canvas.setCursor(8, 14);
    canvas.print("CoreS3 QRCode Scanner");
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setFont(&fonts::Font0);
    canvas.setCursor(8, 56);
    canvas.printf("ESP-62  Phase 2: driver+console\n");
    canvas.printf("IDF 5.3.4 / M5Unified\n");
    canvas.printf("Scanner: %s\n", state);
    canvas.printf("USB console: 'qr status'\n");
}

extern "C" void app_main(void) {
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(kTag, "boot: CoreS3 QRCode scanner (ESP-62) Phase 2");
    ESP_LOGI(kTag, "chip: model=%d rev=%d cores=%d", (int)chip.model,
             (int)chip.revision, (int)chip.cores);

    M5.begin();
    M5.Display.init();
    M5.Display.setRotation(1);
    M5.Display.setBrightness(80);

    M5Canvas canvas(&M5.Display);
    canvas.createSprite(M5.Display.width(), M5.Display.height());

    const char *state = "init...";
    draw_banner(canvas, state);
    canvas.pushSprite(0, 0);

    // --- Scanner module (on-device probe target) ---
    if (g_qr.begin()) {
        char fw[64] = {0};
        bool ok = g_qr.engine().getInfos(0xC1, fw, sizeof(fw));
        state = ok ? fw : "present (no fw reply)";
        ESP_LOGI(kTag, "module ready, firmware=%s", ok ? fw : "(no reply)");
        // Sensible defaults for a handheld reader.
        g_qr.engine().setFillLightMode(QRCodeM14::FILL_ON_DECODE);
        g_qr.engine().setPosLightMode(QRCodeM14::POS_ON_DECODE);
        g_qr.engine().setTriggerMode(QRCodeM14::CONTINUOUS);
    } else {
        state = "NOT FOUND (12V? DIP?)";
        ESP_LOGE(kTag, "module init failed -- 12V power / DIP switch (UART) / stack");
    }
    draw_banner(canvas, state);
    canvas.pushSprite(0, 0);

    // --- USB Serial/JTAG console (probe + commands) ---
    g_console.start(g_qr);

    int n = 0;
    while (true) {
        M5.update();
        if (n % 100 == 0) ESP_LOGI(kTag, "alive tick=%d", n);
        ++n;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
