#include "lvgl_port_m5gfx.h"

#include <cstdlib>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "lvgl_port_m5gfx";

namespace {

static esp_timer_handle_t s_tick_timer = nullptr;

static void tick_cb(void *arg) {
    const int tick_ms = (int)(intptr_t)arg;
    lv_tick_inc((uint32_t)tick_ms);
}

static void flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    auto *gfx_ptr = static_cast<m5gfx::M5GFX *>(disp->user_data);
    m5gfx::M5GFX &gfx = *gfx_ptr;

    const int w = (area->x2 - area->x1 + 1);
    const int h = (area->y2 - area->y1 + 1);
    const uint32_t pixels = (uint32_t)w * (uint32_t)h;

    gfx.startWrite();
    gfx.setAddrWindow(area->x1, area->y1, w, h);

    static constexpr uint32_t kChunkPixels = 8192;
    const lgfx::rgb565_t *src = reinterpret_cast<const lgfx::rgb565_t *>(color_p);
    uint32_t remaining = pixels;
    uint32_t offset = 0;
    while (remaining > 0) {
        const uint32_t chunk = (remaining > kChunkPixels) ? kChunkPixels : remaining;
        gfx.writePixels(src + offset, chunk);
        offset += chunk;
        remaining -= chunk;
    }

    gfx.endWrite();
    lv_disp_flush_ready(disp);
}

static lv_color_t *alloc_draw_buf(size_t bytes) {
    void *p = heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!p) p = malloc(bytes);
    return static_cast<lv_color_t *>(p);
}

} // namespace

bool lvgl_port_m5gfx_init(m5gfx::M5GFX &display, const LvglM5gfxConfig &cfg) {
    lv_init();

    display.setSwapBytes(cfg.swap_bytes);

    const int w = (int)display.width();
    const int h = (int)display.height();
    if (w <= 0 || h <= 0) {
        ESP_LOGE(TAG, "invalid display geometry: %dx%d", w, h);
        return false;
    }

    const int lines = (cfg.buffer_lines > 0) ? cfg.buffer_lines : 40;
    const uint32_t buf_pixels = (uint32_t)w * (uint32_t)lines;
    const size_t buf_bytes = (size_t)buf_pixels * sizeof(lv_color_t);

    lv_color_t *buf1 = alloc_draw_buf(buf_bytes);
    lv_color_t *buf2 = cfg.double_buffer ? alloc_draw_buf(buf_bytes) : nullptr;
    if (!buf1 || (cfg.double_buffer && !buf2)) {
        ESP_LOGE(TAG, "draw buffer alloc failed: bytes=%u (double=%d)", (unsigned)buf_bytes,
                 cfg.double_buffer ? 1 : 0);
        return false;
    }
    memset(buf1, 0, buf_bytes);
    if (buf2) memset(buf2, 0, buf_bytes);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = w;
    disp_drv.ver_res = h;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = &display;
    lv_disp_drv_register(&disp_drv);

    const int tick_ms = (cfg.tick_ms > 0) ? cfg.tick_ms : 2;
    const esp_timer_create_args_t args = {
        .callback = &tick_cb,
        .arg = (void *)(intptr_t)tick_ms,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick",
        .skip_unhandled_events = true,
    };
    if (s_tick_timer) {
        esp_timer_stop(s_tick_timer);
        esp_timer_delete(s_tick_timer);
        s_tick_timer = nullptr;
    }
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_tick_timer, tick_ms * 1000));

    return true;
}

