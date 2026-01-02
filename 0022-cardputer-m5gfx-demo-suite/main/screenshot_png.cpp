#include "screenshot_png.h"

#include <stdio.h>
#include <stdlib.h>

#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void serial_write_all(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        int n = usb_serial_jtag_write_bytes(p, len, portMAX_DELAY);
        if (n <= 0) {
            continue;
        }
        p += (size_t)n;
        len -= (size_t)n;
    }
}

void screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX &display) {
    display.waitDMA();

    size_t len = 0;
    void *png = display.createPng(&len, 0, 0, display.width(), display.height());

    char header[64];
    int header_n = snprintf(header, sizeof(header), "PNG_BEGIN %u\n", (unsigned)len);
    if (header_n > 0) {
        serial_write_all(header, (size_t)header_n);
    }

    if (png && len > 0) {
        serial_write_all(png, len);
    }

    static const char footer[] = "\nPNG_END\n";
    serial_write_all(footer, sizeof(footer) - 1);

    if (png) {
        free(png);
    }
}

