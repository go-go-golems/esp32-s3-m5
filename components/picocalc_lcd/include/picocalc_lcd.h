#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PICOCALC_LCD_WIDTH  320
#define PICOCALC_LCD_HEIGHT 320

#define PICOCALC_LCD_RGB565_BLACK   0x0000
#define PICOCALC_LCD_RGB565_WHITE   0xffff
#define PICOCALC_LCD_RGB565_RED     0xf800
#define PICOCALC_LCD_RGB565_GREEN   0x07e0
#define PICOCALC_LCD_RGB565_BLUE    0x001f
#define PICOCALC_LCD_RGB565_YELLOW  0xffe0
#define PICOCALC_LCD_RGB565_CYAN    0x07ff
#define PICOCALC_LCD_RGB565_MAGENTA 0xf81f

esp_err_t picocalc_lcd_init(void);
esp_err_t picocalc_lcd_fill(uint16_t rgb565);
esp_err_t picocalc_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565);
esp_err_t picocalc_lcd_blit_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                  const uint16_t *pixels, size_t pixel_count);
esp_err_t picocalc_lcd_blit_row(uint16_t y, uint16_t h,
                                 const uint16_t *pixels, size_t pixel_count);
int picocalc_lcd_actual_khz(void);
int picocalc_lcd_requested_hz(void);
size_t picocalc_lcd_max_transfer_bytes(void);

#ifdef __cplusplus
}
#endif
