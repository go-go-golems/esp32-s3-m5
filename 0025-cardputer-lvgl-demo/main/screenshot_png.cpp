#include "screenshot_png.h"

#include <stdint.h>
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

static bool screenshot_png_to_usb_serial_jtag_impl(m5gfx::M5GFX &display, size_t *out_len) {
    if (!ensure_usb_serial_jtag_driver_ready()) {
        if (out_len) *out_len = 0;
        return false;
    }

    display.waitDMA();

    size_t len = 0;
    void *png = display.createPng(&len, 0, 0, display.width(), display.height());
    if (out_len) *out_len = len;

    char header[64];
    int header_n = snprintf(header, sizeof(header), "PNG_BEGIN %u\n", (unsigned)len);
    if (header_n > 0) {
        if (!serial_write_all(header, (size_t)header_n, pdMS_TO_TICKS(25), 50)) {
            if (png) {
                free(png);
            }
            return false;
        }
    }

    if (png && len > 0) {
        if (!serial_write_all(png, len, pdMS_TO_TICKS(25), 200)) {
            free(png);
            return false;
        }
    }

    static const char footer[] = "\nPNG_END\n";
    (void)serial_write_all(footer, sizeof(footer) - 1, pdMS_TO_TICKS(25), 50);

    if (png) {
        free(png);
    }

    return true;
}

struct ScreenshotTaskArgs {
    m5gfx::M5GFX *display = nullptr;
    TaskHandle_t notify_task = nullptr;
    bool *out_ok = nullptr;
    size_t *out_len = nullptr;
};

static void screenshot_task(void *arg) {
    ScreenshotTaskArgs *a = (ScreenshotTaskArgs *)arg;
    bool ok = false;
    size_t len = 0;
    if (a && a->display) {
        ok = screenshot_png_to_usb_serial_jtag_impl(*a->display, &len);
    }
    if (a && a->out_ok) {
        *a->out_ok = ok;
    }
    if (a && a->out_len) {
        *a->out_len = len;
    }
    if (a && a->notify_task) {
        (void)xTaskNotify(a->notify_task, ok ? 1U : 2U, eSetValueWithOverwrite);
    }
    free(a);
    vTaskDelete(nullptr);
}

void screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX &display) {
    (void)screenshot_png_to_usb_serial_jtag_ex(display, nullptr);
}

bool screenshot_png_to_usb_serial_jtag_ex(m5gfx::M5GFX &display, size_t *out_len) {
    // PNG encoding (miniz/tdefl) can be stack-hungry. Avoid running it in the ESP-IDF `main` task.
    // Instead, run the encode/send in a dedicated task with a larger stack, and block until it completes.
    ScreenshotTaskArgs *args = (ScreenshotTaskArgs *)calloc(1, sizeof(ScreenshotTaskArgs));
    if (!args) {
        if (out_len) *out_len = 0;
        return false;
    }
    args->display = &display;
    args->notify_task = xTaskGetCurrentTaskHandle();
    bool ok_value = false;
    size_t len_value = 0;
    args->out_ok = &ok_value;
    args->out_len = &len_value;

    TaskHandle_t task = nullptr;
    BaseType_t ok = xTaskCreatePinnedToCore(screenshot_task, "screenshot_png", 8192, args, 2, &task, tskNO_AFFINITY);
    if (ok != pdPASS) {
        free(args);
        if (out_len) *out_len = 0;
        return false;
    }

    uint32_t value = 0;
    if (xTaskNotifyWait(0, UINT32_MAX, &value, pdMS_TO_TICKS(15000)) != pdTRUE) {
        if (out_len) *out_len = 0;
        return false;
    }

    if (out_len) *out_len = len_value;
    return ok_value && value == 1U;
}

