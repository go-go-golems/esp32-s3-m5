#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXERCIZER_CTRL_DATA_MAX 64
#define EXERCIZER_I2C_WRITE_MAX 32

typedef enum {
    EXERCIZER_CTRL_GPIO_SET = 1,
    EXERCIZER_CTRL_GPIO_STOP = 2,
    EXERCIZER_CTRL_I2C_CONFIG = 3,
    EXERCIZER_CTRL_I2C_TXN = 4,
    EXERCIZER_CTRL_STATUS = 5,
} exercizer_ctrl_type_t;

typedef enum {
    EXERCIZER_GPIO_MODE_HIGH = 1,
    EXERCIZER_GPIO_MODE_LOW = 2,
    EXERCIZER_GPIO_MODE_SQUARE = 3,
    EXERCIZER_GPIO_MODE_PULSE = 4,
} exercizer_gpio_mode_t;

typedef struct {
    uint8_t pin;
    uint8_t mode;
    uint16_t reserved;
    uint32_t hz;
    uint32_t width_us;
    uint32_t period_ms;
} exercizer_gpio_config_t;

typedef struct {
    int8_t port;
    uint8_t sda;
    uint8_t scl;
    uint8_t addr;
    uint32_t hz;
} exercizer_i2c_config_t;

typedef struct {
    uint8_t addr;
    uint8_t write_len;
    uint8_t read_len;
    uint8_t write[EXERCIZER_I2C_WRITE_MAX];
} exercizer_i2c_txn_t;

typedef struct {
    uint8_t type;
    int32_t arg0;
    int32_t arg1;
    uint8_t data_len;
    uint8_t data[EXERCIZER_CTRL_DATA_MAX];
} exercizer_ctrl_event_t;

#ifdef __cplusplus
}
#endif
