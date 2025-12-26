/*
 * Tutorial 0017: AtomS3R backlight helpers.
 *
 * Supports two mechanisms (board-dependent):
 * - a backlight gate GPIO (often GPIO7 active-low)
 * - an I2C brightness device (addr 0x30, brightness reg usually 0x0e)
 *
 * This module keeps the backlight off during panel init to avoid flashing garbage.
 */

#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "esp_err.h"
#include "esp_log.h"

#include "backlight.h"

static const char *TAG = "atoms3r_web_ui_0017";

static bool s_bl_i2c_inited = false;
static constexpr i2c_port_t BL_I2C_PORT = I2C_NUM_0;

static esp_err_t backlight_i2c_write_reg_u8(uint8_t reg, uint8_t value) {
#if CONFIG_TUTORIAL_0017_BACKLIGHT_I2C_ENABLE
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(
        BL_I2C_PORT,
        (uint8_t)CONFIG_TUTORIAL_0017_BL_I2C_ADDR,
        buf,
        sizeof(buf),
        pdMS_TO_TICKS(1000));
#else
    (void)reg;
    (void)value;
    return ESP_OK;
#endif
}

static esp_err_t backlight_i2c_init(void) {
#if CONFIG_TUTORIAL_0017_BACKLIGHT_I2C_ENABLE
    if (s_bl_i2c_inited) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "backlight i2c init: port=%d scl=%d sda=%d addr=0x%02x reg=0x%02x",
             (int)BL_I2C_PORT,
             CONFIG_TUTORIAL_0017_BL_I2C_SCL_GPIO,
             CONFIG_TUTORIAL_0017_BL_I2C_SDA_GPIO,
             (unsigned)CONFIG_TUTORIAL_0017_BL_I2C_ADDR,
             (unsigned)CONFIG_TUTORIAL_0017_BL_I2C_REG);

    // NOTE: We intentionally use the legacy I2C driver here because M5GFX/LovyanGFX
    // links against the legacy driver (`driver/i2c.h`). Mixing legacy + new driver_ng
    // aborts during startup via a constructor in ESP-IDF 5.x.
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.scl_io_num = (gpio_num_t)CONFIG_TUTORIAL_0017_BL_I2C_SCL_GPIO;
    conf.sda_io_num = (gpio_num_t)CONFIG_TUTORIAL_0017_BL_I2C_SDA_GPIO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;

    esp_err_t err = i2c_param_config(BL_I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(BL_I2C_PORT, conf.mode, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        // Already installed by some other component (e.g. M5GFX board init).
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    // AtomS3R backlight init sequence (mirrors M5GFX's Light_M5StackAtomS3R).
    ESP_LOGI(TAG, "backlight i2c chip init: addr=0x%02x (reg 0x00/0x08/0x70)",
             (unsigned)CONFIG_TUTORIAL_0017_BL_I2C_ADDR);

    err = backlight_i2c_write_reg_u8(0x00, 0x40);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight i2c init write reg 0x00 failed: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(1));

    err = backlight_i2c_write_reg_u8(0x08, 0x01);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight i2c init write reg 0x08 failed: %s", esp_err_to_name(err));
        return err;
    }

    err = backlight_i2c_write_reg_u8(0x70, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight i2c init write reg 0x70 failed: %s", esp_err_to_name(err));
        return err;
    }

    s_bl_i2c_inited = true;
    return ESP_OK;
#else
    return ESP_OK;
#endif
}

static esp_err_t backlight_i2c_set(uint8_t brightness) {
#if CONFIG_TUTORIAL_0017_BACKLIGHT_I2C_ENABLE
    esp_err_t err = backlight_i2c_init();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "backlight i2c set: reg=0x%02x value=%u",
             (unsigned)CONFIG_TUTORIAL_0017_BL_I2C_REG,
             (unsigned)brightness);

    err = backlight_i2c_write_reg_u8((uint8_t)CONFIG_TUTORIAL_0017_BL_I2C_REG, brightness);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_write_to_device failed: %s", esp_err_to_name(err));
    }
    return err;
#else
    (void)brightness;
    return ESP_OK;
#endif
}

static void backlight_gate_gpio_set(bool on) {
#if CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_ENABLE
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_GPIO, GPIO_MODE_OUTPUT);

#if CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_ACTIVE_LOW
    const int level = on ? 0 : 1;
#else
    const int level = on ? 1 : 0;
#endif
    ESP_LOGI(TAG, "backlight gate gpio: pin=%d active_low=%d -> %s (level=%d)",
             CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_GPIO,
             (int)(0
#if CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_ACTIVE_LOW
                   + 1
#endif
                   ),
             on ? "ON" : "OFF",
             level);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0017_BACKLIGHT_GATE_GPIO, level);
#else
    (void)on;
#endif
}

void backlight_prepare_for_init(void) {
    backlight_gate_gpio_set(false);
    (void)backlight_i2c_set(0);
}

void backlight_enable_after_init(void) {
    backlight_gate_gpio_set(true);
#if CONFIG_TUTORIAL_0017_BACKLIGHT_I2C_ENABLE
    (void)backlight_i2c_set((uint8_t)CONFIG_TUTORIAL_0017_BL_BRIGHTNESS_ON);
#endif
}

void backlight_set_brightness(uint8_t brightness) {
    (void)backlight_i2c_set(brightness);
}


