/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * Phase-1 NFC reader firmware for the M5StackChan (ESP32-S3 + ST25R3916).
 *
 * Initializes the shared I2C bus exactly as the StackChan firmware does
 * (port 1, SDA=GPIO12, SCL=GPIO11, internal pullup), brings up the ST25R3916,
 * and starts an esp_console REPL over USB Serial/JTAG with nfc-* commands.
 */
#include <stdio.h>
#include "esp_log.h"
#include "esp_console.h"
#include "driver/i2c_master.h"
#include "nfc_console.h"
#include "st25r3916.h"

static const char *TAG = "main";

/* Board I2C bus — mirrors StackChan/firmware/main/hal/board/config.h + stackchan.cc.
 * The body module (ST25R3916, INA226, Si12T, PY32IOExpander) is on this bus. */
#define NFC_I2C_PORT   I2C_NUM_1
#define NFC_I2C_SDA    GPIO_NUM_12
#define NFC_I2C_SCL    GPIO_NUM_11

static i2c_master_bus_handle_t s_i2c_bus = NULL;

static esp_err_t i2c_init(void)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port          = NFC_I2C_PORT,
        .sda_io_num        = NFC_I2C_SDA,
        .scl_io_num        = NFC_I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority     = 0,
        .trans_queue_depth = 0,
        .flags             = { .enable_internal_pullup = 1 },
    };
    esp_err_t e = i2c_new_master_bus(&cfg, &s_i2c_bus);
    if (e != ESP_OK) ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(e));
    return e;
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP-60 NFC reader starting");

    if (i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed; aborting");
        return;
    }
    ESP_LOGI(TAG, "I2C bus ready (port1 SDA=%d SCL=%d)", NFC_I2C_SDA, NFC_I2C_SCL);

    /* Bring up the ST25R3916. Non-fatal if it fails: the console still lets you
     * run `nfc-scan` to see what is on the bus. */
    esp_err_t e = st25r3916_init(s_i2c_bus);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "ST25R3916 init failed: %s — console still available (try nfc-scan)", esp_err_to_name(e));
    }

    /* esp_console REPL over USB Serial/JTAG (ESP32-S3 console path per AGENTS.md). */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "nfc> ";
    repl_cfg.task_stack_size = 4096;

    esp_console_dev_usb_serial_jtag_config_t hw_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    nfc_console_register(s_i2c_bus);

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "console ready (try: help, nfc-scan, nfc-probe, nfc-field on, nfc-read, nfc-poll)");
}
