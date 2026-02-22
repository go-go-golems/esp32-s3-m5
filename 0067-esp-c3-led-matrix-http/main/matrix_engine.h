#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    MATRIX_MODE_IDLE = 0,
    MATRIX_MODE_TEXT,
    MATRIX_MODE_SCROLL,
    MATRIX_MODE_DROP,
    MATRIX_MODE_SCRIPT,
} matrix_mode_t;

typedef enum {
    MATRIX_SCROLL_LOOP_GAP = 0,
    MATRIX_SCROLL_LOOP_WRAP,
    MATRIX_SCROLL_LOOP_RIGHT_EXIT,
} matrix_scroll_loop_t;

typedef struct {
    bool ready;
    matrix_mode_t mode;
    matrix_scroll_loop_t scroll_loop;
    int chain_len;
    int width;
    int spi_hz;
    int intensity;
    bool test_mode;
    bool reverse_modules;
    bool flip_vertical;
    bool rotate_180;
    uint32_t fps;
    uint32_t pause_ms;
    uint32_t repeat_count;
    char text[65];
} matrix_status_t;

typedef struct {
    int pin_mosi;
    int pin_sck;
    int pin_cs;
    int chain_len;
    int spi_hz;
    int default_fps;
    bool default_fliph;
    bool default_flipv;
    bool default_rotate_180;
} matrix_engine_config_t;

esp_err_t matrix_engine_init(const matrix_engine_config_t *cfg);
esp_err_t matrix_engine_play_boot_animation(void);
esp_err_t matrix_engine_show_boot_ip(const char *ip_text, uint32_t fps, uint32_t pause_ms, matrix_scroll_loop_t loop_mode);
esp_err_t matrix_engine_set_text(const char *text);
esp_err_t matrix_engine_start_scroll(const char *text,
                                     uint32_t fps,
                                     uint32_t pause_ms,
                                     uint32_t repeat_count,
                                     bool wave,
                                     matrix_scroll_loop_t loop_mode);
esp_err_t matrix_engine_start_drop(const char *text, uint32_t fps, uint32_t pause_ms, uint32_t repeat_count);
esp_err_t matrix_engine_stop(void);
esp_err_t matrix_engine_set_intensity(int value);
esp_err_t matrix_engine_set_test(bool on);
esp_err_t matrix_engine_set_spi_hz(int hz);
esp_err_t matrix_engine_set_chain_len(int n);
esp_err_t matrix_engine_set_orientation(bool reverse_modules, bool flip_vertical);
esp_err_t matrix_engine_set_rotate_180(bool on);
esp_err_t matrix_engine_get_status(matrix_status_t *out);

int matrix_engine_width(void);
int matrix_engine_height(void);
esp_err_t matrix_engine_frame_clear(void);
esp_err_t matrix_engine_frame_fill(bool on);
esp_err_t matrix_engine_frame_set_pixel(int x, int y, bool on);
esp_err_t matrix_engine_frame_get_pixel(int x, int y, bool *out_on);
esp_err_t matrix_engine_frame_present(void);
