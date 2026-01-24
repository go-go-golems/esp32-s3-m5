#include "tca8418.h"

#include <string.h>

static esp_err_t dev_write_reg8(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev, buf, sizeof(buf), 50);
}

static esp_err_t dev_read_reg8(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    return i2c_master_transmit_receive(dev, &reg, 1, out, 1, 50);
}

esp_err_t tca8418_open(tca8418_t *dev,
                       i2c_master_bus_handle_t bus,
                       uint8_t addr7,
                       uint32_t scl_speed_hz,
                       uint8_t rows,
                       uint8_t cols) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    if (!bus) return ESP_ERR_INVALID_ARG;
    if (rows > 8 || cols > 10) return ESP_ERR_INVALID_ARG;
    memset(dev, 0, sizeof(*dev));
    i2c_device_config_t devcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr7,
        .scl_speed_hz = scl_speed_hz,
        .scl_wait_us = 0,
        .flags.disable_ack_check = 0,
    };
    i2c_master_dev_handle_t handle = NULL;
    esp_err_t err = i2c_master_bus_add_device(bus, &devcfg, &handle);
    if (err != ESP_OK) return err;

    dev->dev = handle;
    dev->rows = rows;
    dev->cols = cols;
    return ESP_OK;
}

esp_err_t tca8418_write_reg8(const tca8418_t *dev, uint8_t reg, uint8_t val) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    if (!dev->dev) return ESP_ERR_INVALID_STATE;
    return dev_write_reg8(dev->dev, reg, val);
}

esp_err_t tca8418_read_reg8(const tca8418_t *dev, uint8_t reg, uint8_t *out) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    if (!dev->dev) return ESP_ERR_INVALID_STATE;
    return dev_read_reg8(dev->dev, reg, out);
}

static uint8_t mask_low_bits(uint8_t n) {
    if (n == 0) return 0;
    if (n >= 8) return 0xFF;
    return (uint8_t)((1u << n) - 1u);
}

esp_err_t tca8418_begin(tca8418_t *dev) {
    if (!dev) return ESP_ERR_INVALID_ARG;

    // Basic GPIO defaults + enable event mode for all pins.
    esp_err_t err = ESP_OK;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_DIR_1, 0x00);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_DIR_2, 0x00);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_DIR_3, 0x00);
    if (err != ESP_OK) return err;

    err = tca8418_write_reg8(dev, TCA8418_REG_GPI_EM_1, 0xFF);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPI_EM_2, 0xFF);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPI_EM_3, 0xFF);
    if (err != ESP_OK) return err;

    // Falling-edge interrupts for GPIOs + enable.
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_INT_LVL_1, 0x00);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_INT_LVL_2, 0x00);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_INT_LVL_3, 0x00);
    if (err != ESP_OK) return err;

    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_INT_EN_1, 0xFF);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_INT_EN_2, 0xFF);
    if (err != ESP_OK) return err;
    err = tca8418_write_reg8(dev, TCA8418_REG_GPIO_INT_EN_3, 0xFF);
    if (err != ESP_OK) return err;

    // Configure keypad matrix size (use lowest pins for rows/cols).
    if (dev->rows > 0 && dev->cols > 0) {
        const uint8_t row_mask = mask_low_bits(dev->rows);
        err = tca8418_write_reg8(dev, TCA8418_REG_KP_GPIO_1, row_mask);
        if (err != ESP_OK) return err;

        const uint8_t col_mask1 = mask_low_bits(dev->cols > 8 ? 8 : dev->cols);
        err = tca8418_write_reg8(dev, TCA8418_REG_KP_GPIO_2, col_mask1);
        if (err != ESP_OK) return err;

        if (dev->cols > 8) {
            const uint8_t col_mask2 = (dev->cols == 9) ? 0x01 : 0x03;
            err = tca8418_write_reg8(dev, TCA8418_REG_KP_GPIO_3, col_mask2);
            if (err != ESP_OK) return err;
        }
    }

    // Clear any pending events/interrupts and enable key-event + gpio interrupts.
    uint8_t flushed = 0;
    (void)tca8418_flush(dev, &flushed);
    return tca8418_enable_interrupts(dev, true);
}

esp_err_t tca8418_enable_interrupts(const tca8418_t *dev, bool enable) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    uint8_t cfg = 0;
    esp_err_t err = tca8418_read_reg8(dev, TCA8418_REG_CFG, &cfg);
    if (err != ESP_OK) return err;
    if (enable) {
        cfg |= (uint8_t)(TCA8418_CFG_GPI_IEN | TCA8418_CFG_KE_IEN);
    } else {
        cfg &= (uint8_t)~(TCA8418_CFG_GPI_IEN | TCA8418_CFG_KE_IEN);
    }
    return tca8418_write_reg8(dev, TCA8418_REG_CFG, cfg);
}

esp_err_t tca8418_available(const tca8418_t *dev, uint8_t *out_count) {
    if (!out_count) return ESP_ERR_INVALID_ARG;
    uint8_t v = 0;
    esp_err_t err = tca8418_read_reg8(dev, TCA8418_REG_KEY_LCK_EC, &v);
    if (err != ESP_OK) return err;
    *out_count = (uint8_t)(v & 0x0F);
    return ESP_OK;
}

esp_err_t tca8418_get_event(const tca8418_t *dev, uint8_t *out_evt) {
    if (!out_evt) return ESP_ERR_INVALID_ARG;
    return tca8418_read_reg8(dev, TCA8418_REG_KEY_EVENT_A, out_evt);
}

esp_err_t tca8418_flush(const tca8418_t *dev, uint8_t *out_flushed) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    uint8_t count = 0;
    for (;;) {
        uint8_t evt = 0;
        esp_err_t err = tca8418_get_event(dev, &evt);
        if (err != ESP_OK) break;
        if (evt == 0) break;
        count++;
    }

    uint8_t tmp = 0;
    (void)tca8418_read_reg8(dev, TCA8418_REG_GPIO_INT_STAT_1, &tmp);
    (void)tca8418_read_reg8(dev, TCA8418_REG_GPIO_INT_STAT_2, &tmp);
    (void)tca8418_read_reg8(dev, TCA8418_REG_GPIO_INT_STAT_3, &tmp);
    (void)tca8418_write_reg8(dev, TCA8418_REG_INT_STAT, 0x03);

    if (out_flushed) *out_flushed = count;
    return ESP_OK;
}

