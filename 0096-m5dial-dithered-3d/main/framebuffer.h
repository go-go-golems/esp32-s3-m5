#pragma once

#include <stdint.h>
#include <stddef.h>

// 2-bit packed framebuffer for 240×240 display
// 4 pixels per byte, 60 bytes per row, 14400 bytes total
// Color indices: 0=black, 1=warm, 2=cool, 3=high

#define FB_WIDTH  240
#define FB_HEIGHT 240
#define FB_BYTES_PER_ROW  (FB_WIDTH / 4)  // 60 bytes
#define FB_TOTAL_BYTES    (FB_BYTES_PER_ROW * FB_HEIGHT)  // 14400

// Color indices
#define COLOR_BLACK 0
#define COLOR_WARM  1
#define COLOR_COOL  2
#define COLOR_HIGH  3

// Initialize the framebuffer (allocates from heap)
bool fb_init(void);

// Free the framebuffer
void fb_deinit(void);

// Get raw buffer pointer
uint8_t* fb_buffer(void);

// Set pixel at (x, y) to color index (0–3)
static inline void fb_set(uint8_t* buf, int x, int y, uint8_t color) {
    const int bit_pos = (x & 3) * 2;           // 0, 2, 4, 6
    const int byte_idx = y * FB_BYTES_PER_ROW + (x >> 2);
    const uint8_t mask = ~(0x03 << bit_pos);    // clear the 2-bit slot
    buf[byte_idx] = (buf[byte_idx] & mask) | ((color & 0x03) << bit_pos);
}

// Get pixel color at (x, y) — returns 0–3
static inline uint8_t fb_get(const uint8_t* buf, int x, int y) {
    const int bit_pos = (x & 3) * 2;
    const int byte_idx = y * FB_BYTES_PER_ROW + (x >> 2);
    return (buf[byte_idx] >> bit_pos) & 0x03;
}

// Fill entire framebuffer with one color
void fb_fill(uint8_t* buf, uint8_t color);

// Filled 2D primitives for cheap post-process overlays / diagnostics.
void fb_fill_rect(uint8_t* buf, int x0, int y0, int w, int h, uint8_t color);
void fb_fill_circle(uint8_t* buf, int cx, int cy, int radius, uint8_t color);

// Expand one scanline from 2-bit framebuffer to RGB565
// rgb565_line must be at least FB_WIDTH * 2 bytes
void fb_expand_scanline(const uint8_t* buf, int y, uint16_t* rgb565_line, const uint16_t palette[4]);
