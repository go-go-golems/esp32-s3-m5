#include "lvgl_port_m5dial.h"

#include <cstdlib>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace ppa_dial {

namespace {

static const char *TAG = "lvgl_port_m5dial";
static esp_timer_handle_t s_tick_timer = nullptr;

void tick_cb(void *arg) {
  const int tick_ms = static_cast<int>(reinterpret_cast<intptr_t>(arg));
  lv_tick_inc(static_cast<uint32_t>(tick_ms));
}

void flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  auto *gfx = static_cast<LGFX_M5Dial *>(disp->user_data);
  if (!gfx) {
    lv_disp_flush_ready(disp);
    return;
  }

  const int width = area->x2 - area->x1 + 1;
  const int height = area->y2 - area->y1 + 1;
  const uint32_t pixels = static_cast<uint32_t>(width) * static_cast<uint32_t>(height);

  gfx->startWrite();
  gfx->setAddrWindow(area->x1, area->y1, width, height);

  static constexpr uint32_t kChunkPixels = 8192;
  const lgfx::rgb565_t *src = reinterpret_cast<const lgfx::rgb565_t *>(color_p);
  uint32_t remaining = pixels;
  uint32_t offset = 0;
  while (remaining > 0) {
    const uint32_t chunk = remaining > kChunkPixels ? kChunkPixels : remaining;
    gfx->writePixels(src + offset, chunk);
    offset += chunk;
    remaining -= chunk;
  }

  gfx->endWrite();
  lv_disp_flush_ready(disp);
}

lv_color_t *alloc_draw_buffer(size_t bytes) {
  void *buffer = heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
  if (!buffer) {
    buffer = malloc(bytes);
  }
  return static_cast<lv_color_t *>(buffer);
}

}  // namespace

bool lvgl_port_m5dial_init(LGFX_M5Dial &display, const LvglPortM5DialConfig &cfg) {
  lv_init();

  display.setSwapBytes(cfg.swap_bytes);

  const int width = static_cast<int>(display.width());
  const int height = static_cast<int>(display.height());
  if (width <= 0 || height <= 0) {
    ESP_LOGE(TAG, "invalid display size %dx%d", width, height);
    return false;
  }

  const int lines = cfg.buffer_lines > 0 ? cfg.buffer_lines : 40;
  const uint32_t buffer_pixels = static_cast<uint32_t>(width) * static_cast<uint32_t>(lines);
  const size_t buffer_bytes = static_cast<size_t>(buffer_pixels) * sizeof(lv_color_t);

  lv_color_t *buf1 = alloc_draw_buffer(buffer_bytes);
  lv_color_t *buf2 = cfg.double_buffer ? alloc_draw_buffer(buffer_bytes) : nullptr;
  if (!buf1 || (cfg.double_buffer && !buf2)) {
    ESP_LOGE(TAG, "LVGL draw buffer allocation failed (%u bytes)", static_cast<unsigned>(buffer_bytes));
    return false;
  }

  memset(buf1, 0, buffer_bytes);
  if (buf2) {
    memset(buf2, 0, buffer_bytes);
  }

  static lv_disp_draw_buf_t draw_buf;
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = width;
  disp_drv.ver_res = height;
  disp_drv.flush_cb = flush_cb;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.user_data = &display;
  lv_disp_drv_register(&disp_drv);

  const int tick_ms = cfg.tick_ms > 0 ? cfg.tick_ms : 2;
  const esp_timer_create_args_t timer_args = {
      .callback = &tick_cb,
      .arg = reinterpret_cast<void *>(static_cast<intptr_t>(tick_ms)),
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lv_tick",
      .skip_unhandled_events = true,
  };
  if (s_tick_timer) {
    esp_timer_stop(s_tick_timer);
    esp_timer_delete(s_tick_timer);
    s_tick_timer = nullptr;
  }
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(s_tick_timer, static_cast<uint64_t>(tick_ms) * 1000ULL));

  return true;
}

}  // namespace ppa_dial
