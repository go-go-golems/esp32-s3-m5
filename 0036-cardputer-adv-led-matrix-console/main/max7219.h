#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"
#include "hal/spi_types.h"

#define MAX7219_DEFAULT_SPI_HOST SPI2_HOST
#define MAX7219_DEFAULT_PIN_SCK  40
#define MAX7219_DEFAULT_PIN_MOSI 14
#define MAX7219_DEFAULT_PIN_CS   5
#define MAX7219_DEFAULT_CHAIN_LEN 4
#define MAX7219_DEFAULT_SPI_HZ (100 * 1000)

#define MAX7219_REG_DIGIT0       0x01
#define MAX7219_REG_DECODE_MODE  0x09
#define MAX7219_REG_INTENSITY    0x0A
#define MAX7219_REG_SCAN_LIMIT   0x0B
#define MAX7219_REG_SHUTDOWN     0x0C
#define MAX7219_REG_DISPLAY_TEST 0x0F

typedef struct {
    spi_device_handle_t spi;
    spi_host_device_t host;
    int pin_cs;
    int chain_len;
    int clock_hz;
} max7219_t;

esp_err_t max7219_open(max7219_t *dev,
                       spi_host_device_t host,
                       int pin_sck,
                       int pin_mosi,
                       int pin_cs,
                       int chain_len);

esp_err_t max7219_init(max7219_t *dev);
esp_err_t max7219_clear(max7219_t *dev);
esp_err_t max7219_set_intensity(max7219_t *dev, uint8_t intensity);
esp_err_t max7219_set_test(max7219_t *dev, bool on);
esp_err_t max7219_set_spi_clock_hz(max7219_t *dev, int clock_hz);
esp_err_t max7219_set_row_raw(max7219_t *dev, uint8_t row, uint8_t bits);
esp_err_t max7219_set_rows_raw(max7219_t *dev, const uint8_t rows[8]);

// Write one row across a daisy-chained set of MAX7219 devices.
// `bytes` must contain `dev->chain_len` bytes, one per module.
esp_err_t max7219_set_row_chain(max7219_t *dev, uint8_t row, const uint8_t *bytes);
