#include "screenshot_png.h"

#include <stdio.h>
#include <stdlib.h>

#include "driver/usb_serial_jtag.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr size_t kTxChunkBytes = 128;

static bool ensure_usb_serial_jtag_driver_ready() {
    if (usb_serial_jtag_is_driver_installed()) {
        return true;
    }

    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    cfg.tx_buffer_size = 4096;
    cfg.rx_buffer_size = 256;
    esp_err_t err = usb_serial_jtag_driver_install(&cfg);
    return err == ESP_OK;
}

static bool serial_write_all(const void *data, size_t len, TickType_t ticks_per_try, int max_zero_writes) {
    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        size_t chunk = (len > kTxChunkBytes) ? kTxChunkBytes : len;
        int n = usb_serial_jtag_write_bytes(p, chunk, ticks_per_try);
        if (n < 0) {
            return false;
        }
        if (n == 0) {
            if (max_zero_writes-- <= 0) {
                return false;
            }
            vTaskDelay(1);
            continue;
        }
        p += (size_t)n;
        len -= (size_t)n;
    }
    return true;
}

void screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX &display) {
    if (!ensure_usb_serial_jtag_driver_ready()) {
        return;
    }

    display.waitDMA();

    size_t len = 0;
    void *png = display.createPng(&len, 0, 0, display.width(), display.height());

    char header[64];
    int header_n = snprintf(header, sizeof(header), "PNG_BEGIN %u\n", (unsigned)len);
    if (header_n > 0) {
        if (!serial_write_all(header, (size_t)header_n, pdMS_TO_TICKS(25), 50)) {
            if (png) {
                free(png);
            }
            return;
        }
    }

    if (png && len > 0) {
        if (!serial_write_all(png, len, pdMS_TO_TICKS(25), 200)) {
            free(png);
            return;
        }
    }

    static const char footer[] = "\nPNG_END\n";
    (void)serial_write_all(footer, sizeof(footer) - 1, pdMS_TO_TICKS(25), 50);

    if (png) {
        free(png);
    }
}
