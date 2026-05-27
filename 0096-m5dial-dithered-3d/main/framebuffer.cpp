#include "framebuffer.h"
#include <string.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

static const char* TAG = "framebuffer";
static uint8_t* s_fb = nullptr;

bool fb_init(void) {
    s_fb = (uint8_t*)heap_caps_malloc(FB_TOTAL_BYTES, MALLOC_CAP_8BIT);
    if (!s_fb) {
        ESP_LOGE(TAG, "failed to allocate %d bytes for framebuffer", FB_TOTAL_BYTES);
        return false;
    }
    memset(s_fb, 0, FB_TOTAL_BYTES);
    ESP_LOGI(TAG, "framebuffer allocated: %d bytes at %p", FB_TOTAL_BYTES, s_fb);
    return true;
}

void fb_deinit(void) {
    if (s_fb) {
        free(s_fb);
        s_fb = nullptr;
    }
}

uint8_t* fb_buffer(void) {
    return s_fb;
}

void fb_fill(uint8_t* buf, uint8_t color) {
    // Pack 4 identical 2-bit values into each byte
    uint8_t packed = (color & 0x03);
    packed |= (packed << 2);
    packed |= (packed << 4);
    packed |= (packed << 6);
    memset(buf, packed, FB_TOTAL_BYTES);
}

void fb_fill_rect(uint8_t* buf, int x0, int y0, int w, int h, uint8_t color) {
    if (!buf || w <= 0 || h <= 0) return;
    int x1 = x0 + w;
    int y1 = y0 + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > FB_WIDTH) x1 = FB_WIDTH;
    if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            fb_set(buf, x, y, color);
        }
    }
}

void fb_fill_circle(uint8_t* buf, int cx, int cy, int radius, uint8_t color) {
    if (!buf || radius <= 0) return;
    const int r2 = radius * radius;
    for (int y = cy - radius; y <= cy + radius; y++) {
        if (y < 0 || y >= FB_HEIGHT) continue;
        for (int x = cx - radius; x <= cx + radius; x++) {
            if (x < 0 || x >= FB_WIDTH) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy <= r2) {
                fb_set(buf, x, y, color);
            }
        }
    }
}

void fb_expand_scanline(const uint8_t* buf, int y, uint16_t* rgb565_line, const uint16_t palette[4]) {
    const uint8_t* row = buf + y * FB_BYTES_PER_ROW;
    for (int x = 0; x < FB_WIDTH; x += 4) {
        uint8_t byte_val = *row++;
        rgb565_line[x + 0] = palette[byte_val & 0x03];
        rgb565_line[x + 1] = palette[(byte_val >> 2) & 0x03];
        rgb565_line[x + 2] = palette[(byte_val >> 4) & 0x03];
        rgb565_line[x + 3] = palette[(byte_val >> 6) & 0x03];
    }
}
