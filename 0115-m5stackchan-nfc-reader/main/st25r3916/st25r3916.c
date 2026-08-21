/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * Minimal ESP-IDF ST25R3916 NFC reader driver (ISO14443-A, Phase 1).
 *
 * I2C register access protocol (from ST25R3916_definition.hpp / datasheet):
 *   read  reg:  write [ (reg & 0x3F) | 0x40 ], repeated-start read N bytes
 *   write reg:  write [ (reg & 0x3F) | 0x00, data... ]
 *   direct cmd: write [ cmd ] (optionally followed by data)
 *   load FIFO:  write [ 0x80, data... ]
 */
#include "st25r3916.h"
#include "st25r3916_regs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "st25r3916";

#define I2C_FREQ_HZ      400000
#define I2C_TIMEOUT_MS   100
#define I2C_TICKS        pdMS_TO_TICKS(I2C_TIMEOUT_MS)

static i2c_master_dev_handle_t s_dev = NULL;

/* ------------------------------------------------------------------ */
/* Low-level register access (mirrors PY32IOExpander_Class style)     */
/* ------------------------------------------------------------------ */

static esp_err_t rd8(uint8_t reg, uint8_t *out)
{
    uint8_t cmd = (reg & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    return i2c_master_transmit_receive(s_dev, &cmd, 1, out, 1, I2C_TICKS);
}

static esp_err_t wr8(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { (uint8_t)((reg & ST25R_OP_TRAILER_MASK) | ST25R_OP_WRITE_REGISTER), val };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), I2C_TICKS);
}

static esp_err_t direct_cmd(uint8_t c)
{
    return i2c_master_transmit(s_dev, &c, 1, I2C_TICKS);
}

static uint16_t fifo_bytes(void);  /* forward decl (used by fifo_read) */

static esp_err_t modify8(uint8_t reg, uint8_t mask, uint8_t bits)
{
    uint8_t v = 0;
    esp_err_t e = rd8(reg, &v);
    if (e != ESP_OK) return e;
    v = (v & ~mask) | (bits & mask);
    return wr8(reg, v);
}

static esp_err_t set_bits(uint8_t reg, uint8_t mask)
{
    return modify8(reg, mask, mask);
}

static esp_err_t clear_bits(uint8_t reg, uint8_t mask)
{
    return modify8(reg, mask, 0);
}

/* FIFO load: byte 0x80 then payload (per OP_LOAD_FIFO). */
static esp_err_t fifo_write(const uint8_t *data, size_t len)
{
    if (len == 0) return ESP_OK;
    uint8_t buf[1 + ST25R_MAX_FIFO_DEPTH];
    if (len > ST25R_MAX_FIFO_DEPTH) return ESP_ERR_INVALID_SIZE;
    buf[0] = ST25R_OP_LOAD_FIFO;
    memcpy(buf + 1, data, len);
    return i2c_master_transmit(s_dev, buf, 1 + len, I2C_TICKS);
}

/* Read N bytes from the FIFO using the dedicated OP_READ_FIFO command (0x9F).
 * Reads only min(fifo_bytes, want) so we never over-read the FIFO. */
static esp_err_t fifo_read(uint8_t *data, size_t want, size_t *got)
{
    if (got) *got = 0;
    uint16_t n = fifo_bytes();
    if (n == 0) return ESP_OK;
    if (n > want) n = (uint16_t)want;
    uint8_t cmd = 0x9F;  /* OP_READ_FIFO */
    esp_err_t e = i2c_master_transmit_receive(s_dev, &cmd, 1, data, n, I2C_TICKS);
    if (e == ESP_OK && got) *got = n;
    return e;
}

/* Read the FIFO byte count. Per M5 lib readFIFOSize(): the 16-bit FIFO status
 * (reg 0x1E low, 0x1F high) gives bytes = reg0x1F | ((reg0x1E & 0xC0) << 2). */
static uint16_t fifo_bytes(void)
{
    uint8_t s1 = 0, s2 = 0;
    if (rd8(ST25R_REG_FIFO_STATUS_1, &s1) != ESP_OK) return 0;
    if (rd8(ST25R_REG_FIFO_STATUS_2, &s2) != ESP_OK) return 0;
    return (uint16_t)s2 | ((uint16_t)(s1 & 0xC0) << 2);
}

/* Read the 24-bit main interrupt register (3 bytes from 0x1A, read). */
static uint32_t read_main_irq(void)
{
    uint8_t cmd = (ST25R_REG_MAIN_INTERRUPT & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    uint8_t buf[3] = {0, 0, 0};
    if (i2c_master_transmit_receive(s_dev, &cmd, 1, buf, 3, I2C_TICKS) != ESP_OK) return 0;
    return ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[0];
}

static esp_err_t clear_interrupts(void)
{
    /* Reading the main interrupt register clears it. */
    (void)read_main_irq();
    /* Also clear error/wakeup and timer/nfc IRQ registers by reading them. */
    uint8_t cmd_a = (ST25R_REG_ERROR_AND_WAKEUP_INTERRUPT & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    uint8_t tmp[2] = {0, 0};
    (void)i2c_master_transmit_receive(s_dev, &cmd_a, 1, tmp, 2, I2C_TICKS);
    uint8_t cmd_b = (ST25R_REG_TIMER_AND_NFC_INTERRUPT & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    (void)i2c_master_transmit_receive(s_dev, &cmd_b, 1, tmp, 2, I2C_TICKS);
    return ESP_OK;
}

/* Set the number of transmitted bytes (REG_NUM_TX_BYTES_1/2 = 0x22/0x23).
 * Layout (M5 lib): value = ((bytes & 0x1FF) << 3) | (bits & 0x07), split low/high
 * across reg 0x22 / 0x23. */
static esp_err_t set_tx_bytes(uint32_t bytes, uint8_t bits)
{
    uint16_t value = (uint16_t)(((bytes & 0x1FF) << 3) | (bits & 0x07));
    esp_err_t e = wr8(ST25R_REG_NUM_TX_BYTES_1, (uint8_t)(value & 0xFF));
    if (e != ESP_OK) return e;
    return wr8(ST25R_REG_NUM_TX_BYTES_2, (uint8_t)((value >> 8) & 0xFF));
}

/* Wait up to timeout_ms for any of the wanted IRQ bits. Returns the IRQ word
 * (with only the matched bits), or 0 on timeout. Phase-1 polls the register. */
static uint32_t wait_irq(uint32_t want_mask, uint32_t timeout_ms)
{
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (esp_timer_get_time() < deadline) {
        uint32_t irq = read_main_irq();
        if (irq & want_mask) return irq & want_mask;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

esp_err_t st25r3916_init(i2c_master_bus_handle_t bus)
{
    if (!bus) return ESP_ERR_INVALID_ARG;
    i2c_device_config_t dev = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = ST25R3916_I2C_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    esp_err_t e = i2c_master_bus_add_device(bus, &dev, &s_dev);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(e));
        return e;
    }

    /* Wait for power-on to stabilize (warm boot via USB can leave chip transient). */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Chip detection with retry. */
    st25r3916_id_t id;
    bool detected = false;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (st25r3916_read_id(&id) == ESP_OK &&
            id.type == ST25R_VALID_IDENTIFY_TYPE && id.revision != 0) {
            detected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (!detected) {
        ESP_LOGE(TAG, "ST25R3916 not detected (type=0x%02x rev=0x%02x)", id.type, id.revision);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "ST25R3916 detected: type=0x%02x rev=0x%02x", id.type, id.revision);

    /* Defensive reset: stop leftover activities, clear tx/rx enables. */
    direct_cmd(ST25R_CMD_STOP_ALL_ACTIVITIES);
    modify8(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN, 0x00);
    vTaskDelay(pdMS_TO_TICKS(2));

    /* Power-on: set to default. */
    e = direct_cmd(ST25R_CMD_SET_DEFAULT);
    if (e != ESP_OK) { ESP_LOGE(TAG, "CMD_SET_DEFAULT failed"); return e; }

    /* Minimal IO config for I2C (3V supply; body board runs 3.3V).
     * IO_CONFIG_1: sup3v (0x80) + io_drv_lvl (0x04) + mcu_clk disabled (0x07) = 0x8B.
     * IO_CONFIG_2: i2c_thd0 (0x10 for 400kHz) + aat_en (0x20) = 0x30. */
    wr8(ST25R_REG_IO_CONFIGURATION_1, 0x8B);
    wr8(ST25R_REG_IO_CONFIGURATION_2, 0x30);

    /* Antenna settings — CRITICAL for RF coupling (from M5 lib begin()):
     *   TX_DRIVER (0x28) = 0xD0  (tx_am_modulation=13 << 4)
     *   ANTENNA_TUNING_CONTROL_1/2 (0x26/0x27) = 0x82
     *   External field detector thresholds (0x2A/0x2B) = 0x13 / 0x02
     * Without these the field register says "on" but the antenna is not driven. */
    wr8(ST25R_REG_TX_DRIVER, 0xD0);
    wr8(0x26, 0x82);
    wr8(0x27, 0x82);
    wr8(0x2A, 0x13);  /* field detector activation threshold */
    wr8(0x2B, 0x02);  /* field detector deactivation threshold */

    /* Clear FIFO. */
    direct_cmd(ST25R_CMD_CLEAR_FIFO);

    /* Mask all interrupts except error, clear them. */
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT, 0xFF);
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT + 1, 0xFF);
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT + 2, 0xFF);
    clear_interrupts();

    /* Enable oscillator: unmask I_osc, set the 'en' bit (0x80), wait for I_osc IRQ,
     * then remask I_osc. (Matches M5 lib enable_osc().) */
    clear_bits(ST25R_REG_MASK_MAIN_INTERRUPT, ST25R_IRQ_OSC);
    clear_interrupts();
    set_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_EN);
    {
        uint32_t irq = wait_irq(ST25R_IRQ_OSC, 50);
        set_bits(ST25R_REG_MASK_MAIN_INTERRUPT, ST25R_IRQ_OSC);
        if (!(irq & ST25R_IRQ_OSC)) {
            ESP_LOGW(TAG, "oscillator did not stabilize (no I_osc); continuing");
        }
    }
    vTaskDelay(pdMS_TO_TICKS(2));
    /* Unmask all interrupts. */
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT, 0x00);
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT + 1, 0x00);
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT + 2, 0x00);

    /* Enable external field detector automatically (en_fd = 0b11). */
    set_bits(ST25R_REG_OPERATION_CONTROL, 0x03);

    /* Adjust regulators and wait. */
    direct_cmd(ST25R_CMD_ADJUST_REGULATORS);
    vTaskDelay(pdMS_TO_TICKS(5));

    /* Configure ISO14443-A reader mode. */
    return st25r3916_configure_nfca();
}

esp_err_t st25r3916_read_id(st25r3916_id_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    uint8_t v = 0;
    esp_err_t e = rd8(ST25R_REG_IC_IDENTITY, &v);
    if (e != ESP_OK) return e;
    out->type = (v >> 3) & 0x0F;
    out->revision = v & 0x07;
    return ESP_OK;
}

esp_err_t st25r3916_field_on(void)
{
    esp_err_t e = direct_cmd(ST25R_CMD_NFC_INITIAL_FIELD_ON);
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));
    /* Per M5 lib nfc_initial_field_on(): clear tx_en|rx_en after field-on; the
     * direct transmit commands manage tx/rx themselves. */
    return clear_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
}

esp_err_t st25r3916_field_off(void)
{
    /* Stop RF and clear tx/rx enables. */
    direct_cmd(ST25R_CMD_STOP_ALL_ACTIVITIES);
    return clear_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
}

uint8_t st25r3916_measure_amplitude(void)
{
    /* CMD_MEASURE_AMPLITUDE (0xD3) measures the amplitude of the signal on RFI.
     * Result is in REG_AMPLITUDE_MEASUREMENT_DISPLAY (0x36). */
    direct_cmd(0xD3);
    vTaskDelay(pdMS_TO_TICKS(8));  /* measurement takes a few ms */
    uint8_t v = 0;
    rd8(0x36, &v);
    return v;
}

void st25r3916_set_tx_rx(bool on)
{
    if (on) set_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
    else    clear_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
}

void st25r3916_debug_dump(void)
{
    uint8_t opc=0, mode=0, iso=0, rssi=0, aux=0, rxc1=0, rxc2=0;
    rd8(ST25R_REG_OPERATION_CONTROL, &opc);
    rd8(ST25R_REG_MODE_DEFINITION, &mode);
    rd8(ST25R_REG_ISO14443A_SETTINGS, &iso);
    rd8(ST25R_REG_AUXILIARY_DEFINITION, &aux);
    rd8(ST25R_REG_RECEIVER_CONFIGURATION_1, &rxc1);
    rd8(ST25R_REG_RECEIVER_CONFIGURATION_2, &rxc2);
    rd8(0x2D /* REG_RSSI_DISPLAY */, &rssi);
    uint32_t irq = read_main_irq();
    uint16_t fb = fifo_bytes();
    ESP_LOGI(TAG, "regs: OPC=%02X MODE=%02X ISO=%02X AUX=%02X RX1=%02X RX2=%02X RSSI=%02X",
             opc, mode, iso, aux, rxc1, rxc2, rssi);
    ESP_LOGI(TAG, "      MAIN_IRQ=%06X FIFO_bytes=%u", (unsigned)irq, (unsigned)fb);
}

esp_err_t st25r3916_configure_nfca(void)
{
    /* Mode definition: initiator, ISO14443-A (0x01<<3=0x08) | nfc_ar8_auto (0x01) = 0x09.
     * (writeInitiatorOperationMode: value = mode | (0x07 & optional); writeModeDefinition writes reg 0x03.) */
    esp_err_t e = wr8(ST25R_REG_MODE_DEFINITION, 0x09);
    if (e != ESP_OK) return e;
    /* Bitrate: tx=rx=106 kbps (0x00). */
    e = wr8(ST25R_REG_BITRATE_DEFINITION, 0x00);
    if (e != ESP_OK) return e;
    /* ISO14443A settings: 0x00 (default). */
    e = wr8(ST25R_REG_ISO14443A_SETTINGS, 0x00);
    if (e != ESP_OK) return e;

    /* Receiver configuration (stability-focused, from M5 lib configure_nfc_a).
     *   RX_CONFIG_1 (0x0B) = z_600k (0x08)
     *   RX_CONFIG_2 (0x0C) = sqm_dyn|agc_en|agc_m|agc6_3 = 0x2D
     *   RX_CONFIG_3 (0x0D) = 0xD8   RX_CONFIG_4 (0x0E) = 0x22  (stability recv gain) */
    wr8(ST25R_REG_RECEIVER_CONFIGURATION_1, 0x08);
    wr8(ST25R_REG_RECEIVER_CONFIGURATION_2, 0x2D);
    wr8(ST25R_REG_RECEIVER_CONFIGURATION_3, 0xD8);
    wr8(ST25R_REG_RECEIVER_CONFIGURATION_4, 0x22);
    direct_cmd(ST25R_CMD_RESET_RX_GAIN);

    /* Clear correlator disable (dis_corr=0x04) for ISO14443-A. */
    clear_bits(ST25R_REG_AUXILIARY_DEFINITION, 0x04);

    /* Initial field on + enable tx/rx. */
    e = st25r3916_field_on();
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "configure_nfca: field_on failed: %s", esp_err_to_name(e));
        return e;
    }
    return ESP_OK;
}

/* ---- REQA: get ATQA ---- */
esp_err_t st25r3916_reqa(uint16_t *atqa)
{
    *atqa = 0;
    /* FWT (frame waiting time) not strictly required for REQA in Phase-1; the
     * chip handles REQA timing internally. Configure ISO14443A settings with
     * anticollision (antcl) and no CRC on RX. */
    wr8(ST25R_REG_ISO14443A_SETTINGS, 0x01); /* antcl */
    set_bits(ST25R_REG_AUXILIARY_DEFINITION, 0x80); /* no_crc_rx */
    clear_interrupts();
    direct_cmd(ST25R_CMD_CLEAR_FIFO);

    esp_err_t e = direct_cmd(ST25R_CMD_TRANSMIT_REQA);
    if (e != ESP_OK) return e;

    uint32_t irq = wait_irq(ST25R_IRQ_RXE | ST25R_IRQ_RXS | ST25R_IRQ_COL, 50);
    if (!(irq & ST25R_IRQ_RXE)) {
        /* If only RX-start (RXS) fired, poll FIFO for 2 bytes briefly. */
        if (irq & ST25R_IRQ_RXS) {
            int64_t dl = esp_timer_get_time() + 50 * 1000;
            while (esp_timer_get_time() < dl) {
                if (fifo_bytes() >= 2) { irq |= ST25R_IRQ_RXE; break; }
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        if (!(irq & ST25R_IRQ_RXE)) return ESP_ERR_NOT_FOUND;
    }

    uint8_t rbuf[2] = {0, 0};
    size_t got = 0;
    e = fifo_read(rbuf, 2, &got);
    if (e != ESP_OK || got != 2) return ESP_FAIL;
    *atqa = ((uint16_t)rbuf[1] << 8) | (uint16_t)rbuf[0];
    return ESP_OK;
}

/* ---- Anticollision + select for one cascade level ---- */
static esp_err_t nfca_anticoll_select(uint8_t sel, uint8_t *uid_out, uint8_t *sak_out)
{
    /* ANTICOLLISION: SEL, NVB=0x20 (request full UID, all bits). */
    uint8_t frame[7] = { sel, 0x20 };
    wr8(ST25R_REG_ISO14443A_SETTINGS, 0x01); /* antcl */
    clear_bits(ST25R_REG_AUXILIARY_DEFINITION, 0x80); /* re-enable CRC for select */
    clear_interrupts();
    direct_cmd(ST25R_CMD_CLEAR_FIFO);
    fifo_write(frame, 2);
    set_tx_bytes(2, 0);
    direct_cmd(ST25R_CMD_TRANSMIT_WITHOUT_CRC);

    uint32_t irq = wait_irq(ST25R_IRQ_RXE | ST25R_IRQ_COL, 50);
    if (irq & ST25R_IRQ_COL) {
        /* Phase-1 single-tag assumption: a collision means two+ tags; report it. */
        return ESP_ERR_INVALID_STATE;
    }
    if (!(irq & ST25R_IRQ_RXE)) return ESP_FAIL;

    /* Expect UID (4 bytes) + BCC (1 byte) = 5 bytes for one cascade level. */
    uint8_t rbuf[5] = {0};
    size_t got = 0;
    fifo_read(rbuf, 5, &got);
    if (got != 5) return ESP_FAIL;

    /* SELECT: SEL, NVB=0x70, UID(4) + BCC(1) — with CRC (chip appends). */
    uint8_t sel_frame[7] = { sel, 0x70, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4] };
    clear_interrupts();
    direct_cmd(ST25R_CMD_CLEAR_FIFO);
    fifo_write(sel_frame, 7);
    set_tx_bytes(7, 0);
    direct_cmd(ST25R_CMD_TRANSMIT_WITH_CRC);

    irq = wait_irq(ST25R_IRQ_RXE | ST25R_IRQ_COL, 50);
    if (!(irq & ST25R_IRQ_RXE)) return ESP_FAIL;

    /* SAK (1 byte) + 2 CRC bytes come back = 3 bytes; we read 3 and take rbuf[0]. */
    uint8_t sakbuf[3] = {0};
    size_t sgot = 0;
    fifo_read(sakbuf, 3, &sgot);
    *sak_out = sakbuf[0];
    /* Copy UID bytes (drop cascade tag 0x88 if present in first byte). */
    memcpy(uid_out, rbuf, 4);
    return ESP_OK;
}

static void sak_to_type(uint8_t sak, char *out, size_t n)
{
    const char *t = "Unknown";
    if (sak == 0x00) t = "MIFARE Ultralight/NTAG";
    else if (sak == 0x08) t = "MIFARE Classic 1K";
    else if (sak == 0x09) t = "MIFARE Mini";
    else if (sak == 0x10) t = "MIFARE Plus 2K";
    else if (sak == 0x11) t = "MIFARE Plus 4K";
    else if (sak == 0x18) t = "MIFARE Classic 4K";
    else if (sak == 0x20) t = "MIFARE DESFire/Plus";
    else if (sak & 0x04) t = "Multi-level UID (SAK cascade)";
    snprintf(out, n, "%s", t);
}

esp_err_t st25r3916_poll_nfca(nfc_picc_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    /* Make sure field is on. */
    esp_err_t e = st25r3916_field_on();
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));

    /* REQA -> ATQA. */
    uint16_t atqa = 0;
    e = st25r3916_reqa(&atqa);
    if (e != ESP_OK) return e; /* no tag */
    out->atqa = atqa;

    /* Anticollision + select, cascade levels CL1 (0x93), CL2 (0x95), CL3 (0x97). */
    uint8_t sel = 0x93;
    uint8_t level = 0;
    out->uid_len = 0;
    while (level < 3) {
        uint8_t uid4[4] = {0};
        uint8_t sak = 0;
        e = nfca_anticoll_select(sel, uid4, &sak);
        if (e != ESP_OK) return e;

        /* Drop the cascade tag (0x88) when copying. */
        uint8_t drop = (uid4[0] == 0x88) ? 1 : 0;
        memcpy(out->uid + out->uid_len, uid4 + drop, 4 - drop);
        out->uid_len += (uint8_t)(4 - drop);

        if (!(sak & 0x04)) {
            /* No more cascade levels. */
            out->sak = sak;
            sak_to_type(sak, out->type_str, sizeof(out->type_str));
            return ESP_OK;
        }
        /* Cascade bit set -> next level. */
        sel = (sel == 0x93) ? 0x95 : 0x97;
        level++;
    }
    return ESP_FAIL;
}
