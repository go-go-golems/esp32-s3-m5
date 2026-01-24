#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

// Minimal TCA8418 keypad controller helpers (register-level).
//
// Intentionally tiny: just enough to (a) probe, (b) configure, and (c) drain key events.

// Registers
#define TCA8418_REG_CFG 0x01
#define TCA8418_REG_INT_STAT 0x02
#define TCA8418_REG_KEY_LCK_EC 0x03
#define TCA8418_REG_KEY_EVENT_A 0x04

#define TCA8418_REG_GPIO_INT_STAT_1 0x11
#define TCA8418_REG_GPIO_INT_STAT_2 0x12
#define TCA8418_REG_GPIO_INT_STAT_3 0x13

#define TCA8418_REG_GPIO_INT_EN_1 0x1A
#define TCA8418_REG_GPIO_INT_EN_2 0x1B
#define TCA8418_REG_GPIO_INT_EN_3 0x1C
#define TCA8418_REG_KP_GPIO_1 0x1D
#define TCA8418_REG_KP_GPIO_2 0x1E
#define TCA8418_REG_KP_GPIO_3 0x1F
#define TCA8418_REG_GPI_EM_1 0x20
#define TCA8418_REG_GPI_EM_2 0x21
#define TCA8418_REG_GPI_EM_3 0x22
#define TCA8418_REG_GPIO_DIR_1 0x23
#define TCA8418_REG_GPIO_DIR_2 0x24
#define TCA8418_REG_GPIO_DIR_3 0x25
#define TCA8418_REG_GPIO_INT_LVL_1 0x26
#define TCA8418_REG_GPIO_INT_LVL_2 0x27
#define TCA8418_REG_GPIO_INT_LVL_3 0x28

// CFG bits
#define TCA8418_CFG_GPI_IEN 0x02
#define TCA8418_CFG_KE_IEN 0x01

// INT_STAT bits
#define TCA8418_INTSTAT_K_INT 0x01

typedef struct {
    i2c_master_dev_handle_t dev;
    uint8_t rows;
    uint8_t cols;
} tca8418_t;

esp_err_t tca8418_open(tca8418_t *dev,
                       i2c_master_bus_handle_t bus,
                       uint8_t addr7,
                       uint32_t scl_speed_hz,
                       uint8_t rows,
                       uint8_t cols);
esp_err_t tca8418_begin(tca8418_t *dev);

esp_err_t tca8418_write_reg8(const tca8418_t *dev, uint8_t reg, uint8_t val);
esp_err_t tca8418_read_reg8(const tca8418_t *dev, uint8_t reg, uint8_t *out);

esp_err_t tca8418_available(const tca8418_t *dev, uint8_t *out_count);
esp_err_t tca8418_get_event(const tca8418_t *dev, uint8_t *out_evt);
esp_err_t tca8418_flush(const tca8418_t *dev, uint8_t *out_flushed);

esp_err_t tca8418_enable_interrupts(const tca8418_t *dev, bool enable);

