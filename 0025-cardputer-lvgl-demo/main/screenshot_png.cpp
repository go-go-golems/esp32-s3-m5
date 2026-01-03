#include "screenshot_png.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "driver/usb_serial_jtag.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/stat.h"

#include "lgfx/utility/lgfx_miniz.h"

#include "sdcard_fatfs.h"

static constexpr size_t kTxChunkBytes = 128;
static constexpr size_t kIdatChunkBufBytes = 2048;

namespace {
struct PngSink {
    void *ctx = nullptr;
    bool (*write)(void *ctx, const void *data, size_t len) = nullptr;
    size_t bytes = 0;
};

static bool sink_write(PngSink *sink, const void *data, size_t len) {
    if (!sink || !sink->write) return false;
    if (!sink->write(sink->ctx, data, len)) return false;
    sink->bytes += len;
    return true;
}

static bool write_u32_be(PngSink *sink, uint32_t v) {
    uint8_t b[4] = {
        (uint8_t)((v >> 24) & 0xFF),
        (uint8_t)((v >> 16) & 0xFF),
        (uint8_t)((v >> 8) & 0xFF),
        (uint8_t)(v & 0xFF),
    };
    return sink_write(sink, b, sizeof(b));
}

static bool write_chunk(PngSink *sink, const char type[4], const void *data, size_t len) {
    if (!write_u32_be(sink, (uint32_t)len)) return false;
    if (!sink_write(sink, type, 4)) return false;
    if (len > 0 && data) {
        if (!sink_write(sink, data, len)) return false;
    }
    lgfx_mz_uint32 crc = (lgfx_mz_uint32)lgfx_mz_crc32(MZ_CRC32_INIT, (const lgfx_mz_uint8 *)type, 4);
    if (len > 0 && data) {
        crc = (lgfx_mz_uint32)lgfx_mz_crc32(crc, (const lgfx_mz_uint8 *)data, len);
    }
    return write_u32_be(sink, (uint32_t)crc);
}

struct DeflateCtx {
    PngSink *sink = nullptr;
    uint8_t buf[kIdatChunkBufBytes];
    size_t len = 0;
    bool ok = true;
};

static bool flush_idat(DeflateCtx *ctx) {
    if (!ctx || !ctx->ok || !ctx->sink) return false;
    if (ctx->len == 0) return true;
    const bool ok = write_chunk(ctx->sink, "IDAT", ctx->buf, ctx->len);
    ctx->len = 0;
    ctx->ok = ok;
    return ok;
}

static lgfx_mz_bool deflate_putter(const void *pBuf, int len, void *pUser) {
    auto *ctx = static_cast<DeflateCtx *>(pUser);
    if (!ctx || !ctx->ok || !ctx->sink) return MZ_FALSE;
    if (!pBuf || len <= 0) return MZ_TRUE;

    const uint8_t *p = static_cast<const uint8_t *>(pBuf);
    size_t remaining = (size_t)len;
    while (remaining > 0) {
        const size_t space = sizeof(ctx->buf) - ctx->len;
        if (space == 0) {
            if (!flush_idat(ctx)) return MZ_FALSE;
            continue;
        }
        const size_t n = (remaining < space) ? remaining : space;
        memcpy(ctx->buf + ctx->len, p, n);
        ctx->len += n;
        p += n;
        remaining -= n;
    }
    return MZ_TRUE;
}

static bool stream_png(m5gfx::M5GFX &display, PngSink *sink, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!sink) return false;

    const int32_t w = display.width();
    const int32_t h = display.height();
    if (w <= 0 || h <= 0) return false;

    const int bpl = (int)((size_t)w * 3U);
    uint8_t *row = (uint8_t *)malloc((size_t)bpl);
    if (!row) return false;

    tdefl_compressor *comp = (tdefl_compressor *)malloc(sizeof(tdefl_compressor));
    if (!comp) {
        free(row);
        return false;
    }

    static const lgfx_mz_uint s_tdefl_png_num_probes[11] = {0, 1, 6, 32, 16, 32, 128, 256, 512, 768, 1500};

    display.waitDMA();

    static const uint8_t kPngSig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    bool ok = sink_write(sink, kPngSig, sizeof(kPngSig));

    uint8_t ihdr[13] = {};
    ihdr[0] = (uint8_t)((w >> 24) & 0xFF);
    ihdr[1] = (uint8_t)((w >> 16) & 0xFF);
    ihdr[2] = (uint8_t)((w >> 8) & 0xFF);
    ihdr[3] = (uint8_t)(w & 0xFF);
    ihdr[4] = (uint8_t)((h >> 24) & 0xFF);
    ihdr[5] = (uint8_t)((h >> 16) & 0xFF);
    ihdr[6] = (uint8_t)((h >> 8) & 0xFF);
    ihdr[7] = (uint8_t)(h & 0xFF);
    ihdr[8] = 8;  // bit depth
    ihdr[9] = 2;  // color type: truecolor (RGB)
    ihdr[10] = 0; // compression
    ihdr[11] = 0; // filter
    ihdr[12] = 0; // interlace
    if (ok) ok = write_chunk(sink, "IHDR", ihdr, sizeof(ihdr));

    DeflateCtx dctx{};
    dctx.sink = sink;

    if (ok) {
        const lgfx_mz_uint level = 6;
        const lgfx_mz_uint idx = (level > 10) ? 10 : level;
        const int flags = (int)s_tdefl_png_num_probes[idx] | TDEFL_WRITE_ZLIB_HEADER;
        tdefl_init(comp, deflate_putter, &dctx, flags);

        const uint8_t filter = 0;
        for (int32_t y = 0; y < h && ok; y++) {
            if (tdefl_compress_buffer(comp, &filter, 1, TDEFL_NO_FLUSH) < 0) {
                ok = false;
                break;
            }
            display.readRectRGB(0, y, w, 1, row);
            if (tdefl_compress_buffer(comp, row, (size_t)bpl, TDEFL_NO_FLUSH) < 0) {
                ok = false;
                break;
            }
            if (!dctx.ok) {
                ok = false;
                break;
            }
        }

        if (ok) {
            if (tdefl_compress_buffer(comp, nullptr, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE) {
                ok = false;
            }
        }
        if (ok) {
            ok = flush_idat(&dctx);
        }
    }

    if (ok) ok = write_chunk(sink, "IEND", nullptr, 0);

    free(comp);
    free(row);

    if (out_len) *out_len = sink->bytes;
    return ok;
}
} // namespace

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

    char header[64];
    // We don't know the PNG length up front without a second encoding pass (which is slow and can fragment heap).
    // Send a length of 0 and let the host parse the streamed PNG bytes until IEND.
    int header_n = snprintf(header, sizeof(header), "PNG_BEGIN 0\n");
    if (header_n > 0) {
        if (!serial_write_all(header, (size_t)header_n, pdMS_TO_TICKS(25), 50)) {
            if (out_len) *out_len = 0;
            return false;
        }
    }

    bool ok = true;
    auto serial_write = [](void *ctx, const void *data, size_t len) -> bool {
        (void)ctx;
        return serial_write_all(data, len, pdMS_TO_TICKS(25), 200);
    };
    PngSink serial_sink{};
    serial_sink.ctx = nullptr;
    serial_sink.write = serial_write;
    serial_sink.bytes = 0;
    size_t png_len = 0;
    ok = stream_png(display, &serial_sink, &png_len) && png_len > 0;

    static const char footer[] = "\nPNG_END\n";
    (void)serial_write_all(footer, sizeof(footer) - 1, pdMS_TO_TICKS(25), 50);

    if (out_len) *out_len = ok ? png_len : 0;
    return ok;
}

static bool screenshot_png_save_to_sd_impl(m5gfx::M5GFX &display, char *out_path, size_t out_path_cap, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (out_path && out_path_cap > 0) out_path[0] = '\0';

    esp_err_t err = sdcard_mount();
    if (err != ESP_OK) {
        printf("ERR: saveshot mount failed: %s (%d)\n", esp_err_to_name(err), (int)err);
        return false;
    }

    // Use 8.3-friendly names (FATFS LFN is not guaranteed to be enabled on-device).
    char dir[64];
    int dir_n = snprintf(dir, sizeof(dir), "%s/shots", sdcard_mount_path());
    if (dir_n <= 0 || (size_t)dir_n >= sizeof(dir)) {
        return false;
    }
    if (mkdir(dir, 0) != 0) {
        if (errno != EEXIST) {
            printf("ERR: saveshot mkdir(%s) failed: errno=%d\n", dir, errno);
            return false;
        }
    }

    const uint32_t id = (uint32_t)((uint64_t)esp_timer_get_time() & 0xFFFFFFu);
    char path[96];
    int path_n = snprintf(path, sizeof(path), "%s/S%06" PRIX32 ".PNG", dir, id);
    if (path_n <= 0 || (size_t)path_n >= sizeof(path)) {
        return false;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        printf("ERR: saveshot fopen(%s) failed: errno=%d\n", path, errno);
        return false;
    }

    auto file_write = [](void *ctx, const void *data, size_t len) -> bool {
        FILE *f = static_cast<FILE *>(ctx);
        return fwrite(data, 1, len, f) == len;
    };
    PngSink file_sink{};
    file_sink.ctx = f;
    file_sink.write = file_write;
    file_sink.bytes = 0;

    size_t len = 0;
    const bool ok = stream_png(display, &file_sink, &len);
    fclose(f);

    if (!ok || len == 0) {
        printf("ERR: saveshot write failed (ok=%d len=%u)\n", ok ? 1 : 0, (unsigned)len);
        return false;
    }

    if (out_len) *out_len = len;
    if (out_path && out_path_cap > 0) {
        strncpy(out_path, path, out_path_cap - 1);
        out_path[out_path_cap - 1] = '\0';
    }
    return true;
}

struct ScreenshotTaskArgs {
    m5gfx::M5GFX *display = nullptr;
    TaskHandle_t notify_task = nullptr;
    bool *out_ok = nullptr;
    size_t *out_len = nullptr;
    char *out_path = nullptr;
    size_t out_path_cap = 0;
    bool save_to_sd = false;
};

static void screenshot_task(void *arg) {
    ScreenshotTaskArgs *a = (ScreenshotTaskArgs *)arg;
    bool ok = false;
    size_t len = 0;
    if (a && a->display) {
        if (a->save_to_sd) {
            ok = screenshot_png_save_to_sd_impl(*a->display, a->out_path, a->out_path_cap, &len);
        } else {
            ok = screenshot_png_to_usb_serial_jtag_impl(*a->display, &len);
        }
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
    BaseType_t ok = xTaskCreatePinnedToCore(screenshot_task, "screenshot_png", 16384, args, 2, &task, tskNO_AFFINITY);
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

bool screenshot_png_save_to_sd_ex(m5gfx::M5GFX &display, char *out_path, size_t out_path_cap, size_t *out_len) {
    ScreenshotTaskArgs *args = (ScreenshotTaskArgs *)calloc(1, sizeof(ScreenshotTaskArgs));
    if (!args) {
        if (out_len) *out_len = 0;
        if (out_path && out_path_cap > 0) out_path[0] = '\0';
        return false;
    }
    args->display = &display;
    args->notify_task = xTaskGetCurrentTaskHandle();
    bool ok_value = false;
    size_t len_value = 0;
    args->out_ok = &ok_value;
    args->out_len = &len_value;
    args->out_path = out_path;
    args->out_path_cap = out_path_cap;
    args->save_to_sd = true;

    TaskHandle_t task = nullptr;
    BaseType_t ok = xTaskCreatePinnedToCore(screenshot_task, "screenshot_sd", 16384, args, 2, &task, tskNO_AFFINITY);
    if (ok != pdPASS) {
        free(args);
        if (out_len) *out_len = 0;
        if (out_path && out_path_cap > 0) out_path[0] = '\0';
        return false;
    }

    uint32_t value = 0;
    if (xTaskNotifyWait(0, UINT32_MAX, &value, pdMS_TO_TICKS(15000)) != pdTRUE) {
        if (out_len) *out_len = 0;
        if (out_path && out_path_cap > 0) out_path[0] = '\0';
        return false;
    }

    if (out_len) *out_len = len_value;
    return ok_value && value == 1U;
}
