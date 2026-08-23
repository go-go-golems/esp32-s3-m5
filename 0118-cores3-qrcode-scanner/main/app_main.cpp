// SPDX-License-Identifier: MIT
// ESP-62 CoreS3 + Module13.2 QRCode scanner — app_main
//
// Phase 1: skeleton + display boot. Initializes M5Unified (display + In_I2C +
// AXP), prints a boot banner on the ILI9341 LCD and over the USB Serial/JTAG
// console. Phase 2 adds the scanner driver (qr_module + qr_engine).

#include <M5Unified.h>

#include <cstdio>

#include "esp_chip_info.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *kTag = "cores3_qr";

static void draw_banner(M5Canvas &canvas) {
    canvas.clear(TFT_BLACK);
    canvas.fillRect(0, 0, canvas.width(), 40, TFT_NAVY);
    canvas.setTextColor(TFT_WHITE, TFT_NAVY);
    canvas.setFont(&fonts::Font2);
    canvas.setCursor(8, 14);
    canvas.print("CoreS3 QRCode Scanner");
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setFont(&fonts::Font0);
    canvas.setCursor(8, 56);
    canvas.print("ESP-62  Phase 1: boot\n");
    canvas.print("IDF 5.3.4 / M5Unified\n");
    canvas.print("Waiting for scanner driver (P2)...\n");
}

extern "C" void app_main(void) {
    // USB Serial/JTAG console is enabled via sdkconfig; wait for it so early
    // logs are visible over USB before M5Unified reconfigures anything.
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(kTag, "boot: CoreS3 QRCode scanner (ESP-62)");
    ESP_LOGI(kTag, "chip: model=%d rev=%d cores=%d", chip.model, chip.revision,
             chip.cores);

    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.init();
    M5.Display.setRotation(1);  // landscape 320x240 on CoreS3
    M5.Display.setBrightness(80);

    M5Canvas canvas(&M5.Display);
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    draw_banner(canvas);
    canvas.pushSprite(0, 0);

    ESP_LOGI(kTag, "display: %dx%d ready", (int)M5.Display.width(),
             (int)M5.Display.height());
    ESP_LOGI(kTag, "ready -- Phase 1 boot OK");

    int n = 0;
    while (true) {
        M5.update();
        // heartbeat blink on the log so we know the app is alive over USB
        if (n % 20 == 0) {
            ESP_LOGI(kTag, "alive tick=%d", n);
        }
        ++n;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
