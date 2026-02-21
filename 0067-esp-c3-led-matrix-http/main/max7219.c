#include "max7219.h"

#include <string.h>

static int clamp_spi_hz(int hz)
{
    if (hz < 1000) return 1000;
    if (hz > 10000000) return 10000000;
    return hz;
}

static esp_err_t write_broadcast(max7219_t *dev, uint8_t reg, uint8_t data)
{
    if (!dev || !dev->spi || dev->chain_len <= 0) return ESP_ERR_INVALID_STATE;
    if (dev->chain_len > MAX7219_MAX_CHAIN_LEN) return ESP_ERR_INVALID_ARG;

    uint8_t tx[2 * MAX7219_MAX_CHAIN_LEN] = {0};
    for (int i = 0; i < dev->chain_len; i++) {
        tx[2 * i] = reg;
        tx[2 * i + 1] = data;
    }

    spi_transaction_t t = {
        .length = 16 * dev->chain_len,
        .tx_buffer = tx,
    };
    return spi_device_transmit(dev->spi, &t);
}

esp_err_t max7219_open(max7219_t *dev, spi_host_device_t host, int pin_sck, int pin_mosi, int pin_cs, int chain_len, int clock_hz)
{
    if (!dev || chain_len < 1 || chain_len > MAX7219_MAX_CHAIN_LEN) return ESP_ERR_INVALID_ARG;

    if (!dev->spi) {
        spi_bus_config_t buscfg = {
            .mosi_io_num = pin_mosi,
            .miso_io_num = -1,
            .sclk_io_num = pin_sck,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 2 * MAX7219_MAX_CHAIN_LEN,
        };

        esp_err_t err = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = clamp_spi_hz(clock_hz),
            .mode = 0,
            .spics_io_num = pin_cs,
            .queue_size = 1,
        };

        spi_device_handle_t handle = NULL;
        err = spi_bus_add_device(host, &devcfg, &handle);
        if (err != ESP_OK) return err;

        dev->spi = handle;
        dev->host = host;
        dev->pin_cs = pin_cs;
        dev->clock_hz = devcfg.clock_speed_hz;
    }

    dev->chain_len = chain_len;
    return ESP_OK;
}

esp_err_t max7219_init(max7219_t *dev)
{
    esp_err_t err = max7219_set_test(dev, false);
    if (err != ESP_OK) return err;
    err = write_broadcast(dev, MAX7219_REG_DECODE_MODE, 0x00);
    if (err != ESP_OK) return err;
    err = write_broadcast(dev, MAX7219_REG_SCAN_LIMIT, 0x07);
    if (err != ESP_OK) return err;
    err = max7219_set_intensity(dev, 0x04);
    if (err != ESP_OK) return err;
    err = write_broadcast(dev, MAX7219_REG_SHUTDOWN, 0x01);
    if (err != ESP_OK) return err;
    return max7219_clear(dev);
}

esp_err_t max7219_clear(max7219_t *dev)
{
    uint8_t zeros[MAX7219_MAX_CHAIN_LEN] = {0};
    for (int row = 0; row < 8; row++) {
        esp_err_t err = max7219_set_row_chain(dev, (uint8_t)row, zeros);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t max7219_set_intensity(max7219_t *dev, uint8_t intensity)
{
    return write_broadcast(dev, MAX7219_REG_INTENSITY, intensity & 0x0F);
}

esp_err_t max7219_set_test(max7219_t *dev, bool on)
{
    return write_broadcast(dev, MAX7219_REG_DISPLAY_TEST, on ? 0x01 : 0x00);
}

esp_err_t max7219_set_spi_clock_hz(max7219_t *dev, int clock_hz)
{
    if (!dev || !dev->spi) return ESP_ERR_INVALID_STATE;

    const int hz = clamp_spi_hz(clock_hz);
    esp_err_t err = spi_bus_remove_device(dev->spi);
    if (err != ESP_OK) return err;
    dev->spi = NULL;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = hz,
        .mode = 0,
        .spics_io_num = dev->pin_cs,
        .queue_size = 1,
    };
    spi_device_handle_t handle = NULL;
    err = spi_bus_add_device(dev->host, &devcfg, &handle);
    if (err != ESP_OK) return err;

    dev->spi = handle;
    dev->clock_hz = hz;
    return ESP_OK;
}

esp_err_t max7219_set_row_chain(max7219_t *dev, uint8_t row, const uint8_t *bytes)
{
    if (!dev || !dev->spi || !bytes || row >= 8 || dev->chain_len <= 0) return ESP_ERR_INVALID_ARG;
    if (dev->chain_len > MAX7219_MAX_CHAIN_LEN) return ESP_ERR_INVALID_ARG;

    uint8_t tx[2 * MAX7219_MAX_CHAIN_LEN] = {0};
    const uint8_t reg = (uint8_t)(MAX7219_REG_DIGIT0 + row);

    for (int i = 0; i < dev->chain_len; i++) {
        const int src = (dev->chain_len - 1) - i;
        tx[2 * i] = reg;
        tx[2 * i + 1] = bytes[src];
    }

    spi_transaction_t t = {
        .length = 16 * dev->chain_len,
        .tx_buffer = tx,
    };
    return spi_device_transmit(dev->spi, &t);
}
