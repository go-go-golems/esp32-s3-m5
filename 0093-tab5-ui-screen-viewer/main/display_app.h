#ifndef DISPLAY_APP_H
#define DISPLAY_APP_H

#include "esp_err.h"
#include "lvgl.h"
#include "esp_heap_caps.h"

/**
 * Initialize the Tab5 display with a black screen and SPIRAM pixel buffer.
 *
 * After init, the fullscreen LVGL image object can be updated by writing
 * RGB565 pixel data into the buffer returned by display_app_get_buffer()
 * and calling display_app_invalidate().
 */
esp_err_t display_app_init(void);

/** Return the SPIRAM pixel buffer (writable, SCREEN_W * SCREEN_H * 2 bytes). */
uint8_t *display_app_get_buffer(void);

/** Return the LVGL image object for direct manipulation. */
lv_obj_t *display_app_get_image_obj(void);

/** Screen width in landscape (after ROTATION_90). */
int display_app_get_width(void);

/** Screen height in landscape. */
int display_app_get_height(void);

/** Total buffer size in bytes (SCREEN_W * SCREEN_H * 2). */
size_t display_app_get_buf_size(void);

/** Mark the image as dirty so LVGL re-renders it on next refresh. */
void display_app_invalidate(void);

#endif // DISPLAY_APP_H
