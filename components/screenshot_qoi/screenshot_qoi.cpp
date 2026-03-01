#include "screenshot_qoi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits>

#include "driver/usb_serial_jtag.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr size_t kTxChunkBytes = 128;
constexpr uint8_t kQoiOpIndex = 0x00;  // 00xxxxxx
constexpr uint8_t kQoiOpDiff = 0x40;   // 01xxxxxx
constexpr uint8_t kQoiOpLuma = 0x80;   // 10xxxxxx
constexpr uint8_t kQoiOpRun = 0xC0;    // 11xxxxxx
constexpr uint8_t kQoiOpRgb = 0xFE;
constexpr uint8_t kQoiOpRgba = 0xFF;

struct QoiSink {
  void* ctx = nullptr;
  bool (*write)(void* ctx, const void* data, size_t len) = nullptr;
  size_t bytes = 0;
};

struct QoiPixel {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;
};

inline bool pixel_eq(const QoiPixel& a, const QoiPixel& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline uint8_t qoi_hash(const QoiPixel& p) {
  return (uint8_t)((p.r * 3u + p.g * 5u + p.b * 7u + p.a * 11u) % 64u);
}

bool sink_write(QoiSink* sink, const void* data, size_t len) {
  if (!sink || !sink->write) return false;
  if (!sink->write(sink->ctx, data, len)) return false;
  sink->bytes += len;
  return true;
}

bool sink_write_u8(QoiSink* sink, uint8_t v) {
  return sink_write(sink, &v, 1);
}

bool sink_write_u32_be(QoiSink* sink, uint32_t v) {
  uint8_t b[4] = {
      (uint8_t)((v >> 24) & 0xFF),
      (uint8_t)((v >> 16) & 0xFF),
      (uint8_t)((v >> 8) & 0xFF),
      (uint8_t)(v & 0xFF),
  };
  return sink_write(sink, b, sizeof(b));
}

bool ensure_usb_serial_jtag_driver_ready() {
  if (usb_serial_jtag_is_driver_installed()) {
    return true;
  }

  usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
  cfg.tx_buffer_size = 4096;
  cfg.rx_buffer_size = 256;
  return usb_serial_jtag_driver_install(&cfg) == ESP_OK;
}

bool serial_write_all(const void* data, size_t len, TickType_t ticks_per_try, int max_zero_writes) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  while (len > 0) {
    const size_t chunk = (len > kTxChunkBytes) ? kTxChunkBytes : len;
    const int n = usb_serial_jtag_write_bytes(p, chunk, ticks_per_try);
    if (n < 0) return false;
    if (n == 0) {
      if (max_zero_writes-- <= 0) return false;
      vTaskDelay(1);
      continue;
    }
    p += (size_t)n;
    len -= (size_t)n;
  }
  return true;
}

bool qoi_encode_display(m5gfx::M5GFX& display, QoiSink* sink, size_t* out_len) {
  if (out_len) *out_len = 0;
  if (!sink) return false;

  const int32_t w = display.width();
  const int32_t h = display.height();
  if (w <= 0 || h <= 0) return false;
  const size_t row_bytes = (size_t)w * 3U;

  uint8_t* row = static_cast<uint8_t*>(malloc(row_bytes));
  if (!row) return false;

  bool ok = true;
  QoiPixel index[64] = {};
  QoiPixel prev = {};
  prev.r = 0;
  prev.g = 0;
  prev.b = 0;
  prev.a = 255;
  int run = 0;

  static const uint8_t kMagic[4] = {'q', 'o', 'i', 'f'};
  static const uint8_t kEndMarker[8] = {0, 0, 0, 0, 0, 0, 0, 1};

  display.waitDMA();

  if (ok) ok = sink_write(sink, kMagic, sizeof(kMagic));
  if (ok) ok = sink_write_u32_be(sink, (uint32_t)w);
  if (ok) ok = sink_write_u32_be(sink, (uint32_t)h);
  if (ok) ok = sink_write_u8(sink, 3);  // channels
  if (ok) ok = sink_write_u8(sink, 0);  // colorspace (sRGB + linear alpha)

  for (int32_t y = 0; y < h && ok; y++) {
    display.readRectRGB(0, y, w, 1, row);

    for (int32_t x = 0; x < w && ok; x++) {
      const size_t off = (size_t)x * 3U;
      QoiPixel px = {};
      px.r = row[off + 0];
      px.g = row[off + 1];
      px.b = row[off + 2];
      px.a = 255;

      const bool is_last_pixel = (y == (h - 1)) && (x == (w - 1));
      if (pixel_eq(px, prev)) {
        run++;
        if (run == 62 || is_last_pixel) {
          ok = sink_write_u8(sink, (uint8_t)(kQoiOpRun | (uint8_t)(run - 1)));
          run = 0;
        }
        continue;
      }

      if (run > 0) {
        ok = sink_write_u8(sink, (uint8_t)(kQoiOpRun | (uint8_t)(run - 1)));
        run = 0;
        if (!ok) break;
      }

      const uint8_t idx = qoi_hash(px);
      if (pixel_eq(index[idx], px)) {
        ok = sink_write_u8(sink, (uint8_t)(kQoiOpIndex | idx));
        prev = px;
        continue;
      }

      index[idx] = px;

      if (px.a == prev.a) {
        const int vr = (int)px.r - (int)prev.r;
        const int vg = (int)px.g - (int)prev.g;
        const int vb = (int)px.b - (int)prev.b;

        if (vr >= -2 && vr <= 1 && vg >= -2 && vg <= 1 && vb >= -2 && vb <= 1) {
          const uint8_t b1 = (uint8_t)(kQoiOpDiff | ((vr + 2) << 4) | ((vg + 2) << 2) | (vb + 2));
          ok = sink_write_u8(sink, b1);
        } else {
          const int vgr = vr - vg;
          const int vgb = vb - vg;
          if (vg >= -32 && vg <= 31 && vgr >= -8 && vgr <= 7 && vgb >= -8 && vgb <= 7) {
            const uint8_t b1 = (uint8_t)(kQoiOpLuma | (vg + 32));
            const uint8_t b2 = (uint8_t)(((vgr + 8) << 4) | (vgb + 8));
            ok = sink_write_u8(sink, b1) && sink_write_u8(sink, b2);
          } else {
            const uint8_t rgb[4] = {kQoiOpRgb, px.r, px.g, px.b};
            ok = sink_write(sink, rgb, sizeof(rgb));
          }
        }
      } else {
        const uint8_t rgba[5] = {kQoiOpRgba, px.r, px.g, px.b, px.a};
        ok = sink_write(sink, rgba, sizeof(rgba));
      }

      prev = px;
    }
  }

  if (ok) ok = sink_write(sink, kEndMarker, sizeof(kEndMarker));

  free(row);
  if (out_len) *out_len = sink->bytes;
  return ok;
}

bool screenshot_qoi_to_usb_serial_jtag_impl(m5gfx::M5GFX& display, size_t* out_len) {
  if (out_len) *out_len = 0;
  if (!ensure_usb_serial_jtag_driver_ready()) return false;

  // Pass 1: count exact QOI size.
  size_t count_len = 0;
  auto count_write = [](void* ctx, const void* data, size_t len) -> bool {
    (void)data;
    size_t* count = static_cast<size_t*>(ctx);
    *count += len;
    return true;
  };
  QoiSink count_sink = {};
  count_sink.ctx = &count_len;
  count_sink.write = count_write;
  size_t count_encoded = 0;
  if (!qoi_encode_display(display, &count_sink, &count_encoded) || count_encoded == 0) {
    return false;
  }
  if (count_encoded > (size_t)std::numeric_limits<uint32_t>::max()) {
    return false;
  }

  char header[64];
  const int header_n = snprintf(header, sizeof(header), "QOI_BEGIN %u\n", (unsigned)count_encoded);
  if (header_n <= 0) return false;
  if (!serial_write_all(header, (size_t)header_n, pdMS_TO_TICKS(25), 500)) {
    return false;
  }

  // Pass 2: write QOI payload.
  auto serial_write = [](void* ctx, const void* data, size_t len) -> bool {
    (void)ctx;
    return serial_write_all(data, len, pdMS_TO_TICKS(25), 4000);
  };
  QoiSink serial_sink = {};
  serial_sink.ctx = nullptr;
  serial_sink.write = serial_write;
  size_t written_len = 0;
  const bool write_ok = qoi_encode_display(display, &serial_sink, &written_len);

  static const char footer[] = "\nQOI_END\n";
  (void)serial_write_all(footer, sizeof(footer) - 1, pdMS_TO_TICKS(25), 500);

  if (!write_ok || written_len != count_encoded) {
    return false;
  }

  if (out_len) *out_len = written_len;
  return true;
}

struct ScreenshotTaskArgs {
  m5gfx::M5GFX* display = nullptr;
  TaskHandle_t notify_task = nullptr;
  bool* out_ok = nullptr;
  size_t* out_len = nullptr;
};

void screenshot_task(void* arg) {
  ScreenshotTaskArgs* a = static_cast<ScreenshotTaskArgs*>(arg);
  bool ok = false;
  size_t len = 0;
  if (a && a->display) {
    ok = screenshot_qoi_to_usb_serial_jtag_impl(*a->display, &len);
  }
  if (a && a->out_ok) *a->out_ok = ok;
  if (a && a->out_len) *a->out_len = len;
  if (a && a->notify_task) {
    (void)xTaskNotify(a->notify_task, ok ? 1U : 2U, eSetValueWithOverwrite);
  }
  free(a);
  vTaskDelete(nullptr);
}

}  // namespace

void screenshot_qoi_to_usb_serial_jtag(m5gfx::M5GFX& display) {
  (void)screenshot_qoi_to_usb_serial_jtag_ex(display, nullptr);
}

bool screenshot_qoi_to_usb_serial_jtag_ex(m5gfx::M5GFX& display, size_t* out_len) {
  ScreenshotTaskArgs* args = static_cast<ScreenshotTaskArgs*>(calloc(1, sizeof(ScreenshotTaskArgs)));
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
  const BaseType_t created =
      xTaskCreatePinnedToCore(screenshot_task, "screenshot_qoi", 12288, args, 2, &task, tskNO_AFFINITY);
  if (created != pdPASS) {
    free(args);
    if (out_len) *out_len = 0;
    return false;
  }

  uint32_t value = 0;
  if (xTaskNotifyWait(0, UINT32_MAX, &value, pdMS_TO_TICKS(20000)) != pdTRUE) {
    if (out_len) *out_len = 0;
    return false;
  }

  if (out_len) *out_len = len_value;
  return ok_value && value == 1U;
}
