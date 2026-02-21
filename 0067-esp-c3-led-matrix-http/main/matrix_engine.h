#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    MATRIX_MODE_IDLE = 0,
    MATRIX_MODE_TEXT,
    MATRIX_MODE_SCROLL,
    MATRIX_MODE_DROP,
} matrix_mode_t;

typedef struct {
    bool ready;
    matrix_mode_t mode;
    int chain_len;
    int width;
    int spi_hz;
    int intensity;
    bool test_mode;
    bool reverse_modules;
    bool flip_vertical;
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
} matrix_engine_config_t;

esp_err_t matrix_engine_init(const matrix_engine_config_t *cfg);
esp_err_t matrix_engine_set_text(const char *text);
esp_err_t matrix_engine_start_scroll(const char *text, uint32_t fps, uint32_t pause_ms, uint32_t repeat_count, bool wave);
esp_err_t matrix_engine_start_drop(const char *text, uint32_t fps, uint32_t pause_ms, uint32_t repeat_count);
esp_err_t matrix_engine_stop(void);
esp_err_t matrix_engine_set_intensity(int value);
esp_err_t matrix_engine_set_test(bool on);
esp_err_t matrix_engine_set_spi_hz(int hz);
esp_err_t matrix_engine_set_chain_len(int n);
esp_err_t matrix_engine_set_orientation(bool reverse_modules, bool flip_vertical);
esp_err_t matrix_engine_get_status(matrix_status_t *out);
