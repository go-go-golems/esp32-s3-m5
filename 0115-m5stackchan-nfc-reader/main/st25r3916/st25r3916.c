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
#define I2C_TIMEOUT_MS   100 /* ESP-IDF APIs take milliseconds, not FreeRTOS ticks */

static i2c_master_dev_handle_t s_dev_high = NULL;
static i2c_master_dev_handle_t s_dev_defined = NULL;
static st25r3916_transport_backend_t s_backend = ST25R3916_BACKEND_IDF_HIGH;
static uint8_t s_last_timer_irq = 0;
static uint8_t s_last_error_irq = 0;

/* Observer-safe transaction trace ring (design doc 04). Records every I2C
 * transaction to the ST25R3916; no serial/heap/log in the hot path. */
static st25r_trace_store_t s_trace;

st25r_trace_store_t *st25r3916_trace(void) { return &s_trace; }

const char *st25r3916_transport_backend_name(st25r3916_transport_backend_t backend)
{
    switch (backend) {
    case ST25R3916_BACKEND_IDF_HIGH: return "idf-high";
    case ST25R3916_BACKEND_IDF_DEFINED: return "idf-defined";
    default: return "unknown";
    }
}

st25r3916_transport_backend_t st25r3916_get_transport_backend(void) { return s_backend; }

esp_err_t st25r3916_set_transport_backend(st25r3916_transport_backend_t backend)
{
    if (backend != ST25R3916_BACKEND_IDF_HIGH && backend != ST25R3916_BACKEND_IDF_DEFINED)
        return ESP_ERR_INVALID_ARG;
    s_backend = backend;
    st25r_trace_clear(&s_trace);
    return ESP_OK;
}

static st25r_trace_backend_t current_trace_backend(void)
{
    return s_backend == ST25R3916_BACKEND_IDF_DEFINED
        ? ST25R_TRACE_BACKEND_IDF_DEFINED
        : ST25R_TRACE_BACKEND_IDF_HIGH;
}

static void trace_phase(st25r_trace_phase_t phase)
{
    st25r_trace_set_context(&s_trace, current_trace_backend(), phase, 1);
}

/* Explicit START/address/payload/STOP jobs. The device handle is configured
 * with I2C_DEVICE_ADDRESS_NOT_USED, so address bytes are supplied here. */
static esp_err_t defined_write(const uint8_t *data, size_t len)
{
    uint8_t address_write = (uint8_t)(ST25R3916_I2C_ADDR << 1);
    i2c_operation_job_t ops[] = {
        { .command = I2C_MASTER_CMD_START },
        { .command = I2C_MASTER_CMD_WRITE,
          .write = { .ack_check = true, .data = &address_write, .total_bytes = 1 } },
        { .command = I2C_MASTER_CMD_WRITE,
          .write = { .ack_check = true, .data = (uint8_t *)data, .total_bytes = len } },
        { .command = I2C_MASTER_CMD_STOP },
    };
    return i2c_master_execute_defined_operations(s_dev_defined, ops,
                                                  sizeof(ops) / sizeof(ops[0]), I2C_TIMEOUT_MS);
}

static esp_err_t defined_write_read(const uint8_t *write_data, size_t write_len,
                                    uint8_t *read_data, size_t read_len)
{
    if (read_len == 0) return ESP_ERR_INVALID_ARG;
    uint8_t address_write = (uint8_t)(ST25R3916_I2C_ADDR << 1);
    uint8_t address_read = (uint8_t)((ST25R3916_I2C_ADDR << 1) | 1);
    i2c_operation_job_t ops[8] = {0};
    size_t n = 0;
    ops[n++].command = I2C_MASTER_CMD_START;
    ops[n++] = (i2c_operation_job_t){ .command = I2C_MASTER_CMD_WRITE,
        .write = { .ack_check = true, .data = &address_write, .total_bytes = 1 } };
    ops[n++] = (i2c_operation_job_t){ .command = I2C_MASTER_CMD_WRITE,
        .write = { .ack_check = true, .data = (uint8_t *)write_data, .total_bytes = write_len } };
    ops[n++].command = I2C_MASTER_CMD_START; /* repeated START */
    ops[n++] = (i2c_operation_job_t){ .command = I2C_MASTER_CMD_WRITE,
        .write = { .ack_check = true, .data = &address_read, .total_bytes = 1 } };
    if (read_len > 1) {
        ops[n++] = (i2c_operation_job_t){ .command = I2C_MASTER_CMD_READ,
            .read = { .ack_value = I2C_ACK_VAL, .data = read_data, .total_bytes = read_len - 1 } };
    }
    ops[n++] = (i2c_operation_job_t){ .command = I2C_MASTER_CMD_READ,
        .read = { .ack_value = I2C_NACK_VAL, .data = read_data + read_len - 1, .total_bytes = 1 } };
    ops[n++].command = I2C_MASTER_CMD_STOP;
    return i2c_master_execute_defined_operations(s_dev_defined, ops, n, I2C_TIMEOUT_MS);
}

static esp_err_t transport_write(const uint8_t *data, size_t len)
{
    return s_backend == ST25R3916_BACKEND_IDF_DEFINED
        ? defined_write(data, len)
        : i2c_master_transmit(s_dev_high, data, len, I2C_TIMEOUT_MS);
}

static esp_err_t transport_write_read(const uint8_t *write_data, size_t write_len,
                                      uint8_t *read_data, size_t read_len)
{
    return s_backend == ST25R3916_BACKEND_IDF_DEFINED
        ? defined_write_read(write_data, write_len, read_data, read_len)
        : i2c_master_transmit_receive(s_dev_high, write_data, write_len,
                                      read_data, read_len, I2C_TIMEOUT_MS);
}

/* Record one traced transaction; computes elapsed_us from the captured t0. */
static void trace_rec(st25r_trace_op_t op, uint8_t lkey, uint8_t wkey,
                      st25r_trace_kind_t kind, uint16_t wlen, uint16_t rlen,
                      int64_t t0, esp_err_t e)
{
    st25r_trace_record(&s_trace, op, lkey, wkey, kind, wlen, rlen, t0,
                        (uint32_t)(esp_timer_get_time() - t0), (int32_t)e, 0);
}

/* ------------------------------------------------------------------ */
/* Low-level register access (mirrors PY32IOExpander_Class style)     */
/* ------------------------------------------------------------------ */

static esp_err_t rd8(uint8_t reg, uint8_t *out)
{
    uint8_t cmd = (reg & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write_read(&cmd, 1, out, 1);
    trace_rec(ST25R_OP_READ_A, reg, cmd, ST25R_TRACE_KIND_WRITE_READ, 1, 1, t0, e);
    return e;
}

static esp_err_t wr8(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { (uint8_t)((reg & ST25R_OP_TRAILER_MASK) | ST25R_OP_WRITE_REGISTER), val };
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write(buf, sizeof(buf));
    trace_rec(ST25R_OP_WRITE_A, reg, buf[0], ST25R_TRACE_KIND_WRITE, 2, 0, t0, e);
    return e;
}

static esp_err_t direct_cmd(uint8_t c)
{
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write(&c, 1);
    trace_rec(ST25R_OP_DIRECT_CMD, c, c, ST25R_TRACE_KIND_WRITE, 1, 0, t0, e);
    return e;
}

static esp_err_t direct_cmd_data(uint8_t c, const uint8_t *data, size_t len)
{
    if (len > 2) return ESP_ERR_INVALID_SIZE;
    uint8_t buf[3] = {c, 0, 0};
    if (len) memcpy(buf + 1, data, len);
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write(buf, len + 1);
    trace_rec(ST25R_OP_DIRECT_CMD_DATA, c, c, ST25R_TRACE_KIND_WRITE, (uint16_t)(len + 1), 0, t0, e);
    return e;
}

/* Space-B I2C write: 0xFB prefix, Space-B register address, value. */
static esp_err_t wr8b(uint8_t reg, uint8_t val)
{
    uint8_t buf[3] = {ST25R_CMD_REGISTER_SPACE_B_ACCESS,
                      (uint8_t)(reg & ST25R_OP_TRAILER_MASK), val};
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write(buf, sizeof(buf));
    trace_rec(ST25R_OP_WRITE_B, reg, buf[0], ST25R_TRACE_KIND_WRITE, 3, 0, t0, e);
    return e;
}

static esp_err_t rd8b(uint8_t reg, uint8_t *out)
{
    uint8_t cmd[2] = {ST25R_CMD_REGISTER_SPACE_B_ACCESS,
                      (uint8_t)((reg & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER)};
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write_read(cmd, sizeof(cmd), out, 1);
    trace_rec(ST25R_OP_READ_B, reg, cmd[0], ST25R_TRACE_KIND_WRITE_READ, 2, 1, t0, e);
    return e;
}

static esp_err_t fifo_status(uint16_t *bytes, uint8_t *bits);
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
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write(buf, 1 + len);
    trace_rec(ST25R_OP_FIFO_WRITE, ST25R_OP_LOAD_FIFO, ST25R_OP_LOAD_FIFO,
             ST25R_TRACE_KIND_WRITE, (uint16_t)(1 + len), 0, t0, e);
    return e;
}

/* Read N bytes from the FIFO using the dedicated OP_READ_FIFO command (0x9F).
 * Reads only min(fifo_bytes, want) so we never over-read the FIFO. */
static esp_err_t fifo_read(uint8_t *data, size_t want, size_t *got)
{
    if (got) *got = 0;
    uint16_t n = 0;
    esp_err_t e = fifo_status(&n, NULL);
    if (e != ESP_OK) return e;
    if (n == 0) return ESP_OK;
    if (n > want) n = (uint16_t)want;
    uint8_t cmd = 0x9F;  /* OP_READ_FIFO */
    int64_t t0 = esp_timer_get_time();
    e = transport_write_read(&cmd, 1, data, n);
    trace_rec(ST25R_OP_FIFO_READ, 0x9F, 0x9F, ST25R_TRACE_KIND_WRITE_READ, 1, n, t0, e);
    if (e == ESP_OK && got) *got = n;
    return e;
}

/* Read FIFO byte + partial-bit counts. M5 readFIFOSize() interprets
 * status1 as count bits 7:0, status2 bits 7:6 as count bits 9:8, and
 * status2 bits 3:1 as the number of valid bits in the incomplete byte. */
static esp_err_t fifo_status(uint16_t *bytes, uint8_t *bits)
{
    uint8_t status1 = 0, status2 = 0;
    esp_err_t e = rd8(ST25R_REG_FIFO_STATUS_1, &status1);
    if (e != ESP_OK) return e;
    e = rd8(ST25R_REG_FIFO_STATUS_2, &status2);
    if (e != ESP_OK) return e;
    if (bytes) *bytes = (uint16_t)status1 | ((uint16_t)(status2 & 0xC0) << 2);
    if (bits) *bits = (status2 >> 1) & 0x07;
    return ESP_OK;
}

static uint16_t fifo_bytes(void)
{
    uint16_t bytes = 0;
    return fifo_status(&bytes, NULL) == ESP_OK ? bytes : 0;
}

/* Preserve the error IRQ before reading Main: per M5Unit-NFC/ST25R3916,
 * reading Main clears the Error/Wakeup register. Then read Main + Timer/NFC.
 * The public return layout remains main in bits 7:0 and timer in bits 15:8. */
static uint32_t read_main_irq(void)
{
    uint8_t error = 0;
    (void)rd8(ST25R_REG_ERROR_AND_WAKEUP_INTERRUPT, &error);

    uint8_t cmd = (ST25R_REG_MAIN_INTERRUPT & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    uint8_t buf[2] = {0, 0};
    int64_t t0 = esp_timer_get_time();
    esp_err_t e = transport_write_read(&cmd, 1, buf, 2);
    trace_rec(ST25R_OP_IRQ_READ, ST25R_REG_MAIN_INTERRUPT, cmd, ST25R_TRACE_KIND_WRITE_READ, 1, 2, t0, e);
    if (e != ESP_OK) return 0;
    s_last_error_irq = error;
    s_last_timer_irq = buf[1];
    return ((uint32_t)buf[1] << 8) | (uint32_t)buf[0];
}

static esp_err_t clear_interrupts(void)
{
    /* Reading the main interrupt register clears it. */
    (void)read_main_irq();
    /* Also clear error/wakeup and timer/nfc IRQ registers by reading them. */
    uint8_t cmd_a = (ST25R_REG_ERROR_AND_WAKEUP_INTERRUPT & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    uint8_t tmp[2] = {0, 0};
    {
        int64_t t0 = esp_timer_get_time();
        esp_err_t e = transport_write_read(&cmd_a, 1, tmp, 2);
        trace_rec(ST25R_OP_IRQ_READ, ST25R_REG_ERROR_AND_WAKEUP_INTERRUPT, cmd_a, ST25R_TRACE_KIND_WRITE_READ, 1, 2, t0, e);
        (void)e;
    }
    uint8_t cmd_b = (ST25R_REG_TIMER_AND_NFC_INTERRUPT & ST25R_OP_TRAILER_MASK) | ST25R_OP_READ_REGISTER;
    {
        int64_t t0 = esp_timer_get_time();
        esp_err_t e = transport_write_read(&cmd_b, 1, tmp, 2);
        trace_rec(ST25R_OP_IRQ_READ, ST25R_REG_TIMER_AND_NFC_INTERRUPT, cmd_b, ST25R_TRACE_KIND_WRITE_READ, 1, 2, t0, e);
        (void)e;
    }
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

/* Configure the frame-waiting/no-response timer exactly like M5Unit-NFC's
 * write_fwt_timer(). NRT is ceil(timeout / step), where the step is selected
 * by TIMER_AND_EMV_CONTROL.nrt_step: 64/fc or 4096/fc, fc=13.56 MHz. */
static esp_err_t set_frame_wait_time(uint32_t timeout_ms)
{
    uint8_t timer_ctrl = 0;
    esp_err_t e = rd8(ST25R_REG_TIMER_AND_EMV_CONTROL, &timer_ctrl);
    if (e != ESP_OK) return e;

    const uint64_t step_num = (timer_ctrl & ST25R_TIMER_NRT_STEP)
        ? (4096ULL * 1000000ULL)
        : (64ULL * 1000000ULL);
    const uint64_t timeout_us = (uint64_t)timeout_ms * 1000ULL;
    uint64_t nrt = (timeout_us * 13560000ULL + step_num - 1ULL) / step_num;
    if (nrt < 1ULL) nrt = 1ULL;
    if (nrt > 0xFFFFULL) nrt = 0xFFFFULL;

    /* ST25R3916 16-bit register writes are big-endian: reg 0x10 is MSB. */
    e = wr8(ST25R_REG_NO_RESPONSE_TIMER_1, (uint8_t)(nrt >> 8));
    if (e != ESP_OK) return e;
    return wr8(ST25R_REG_NO_RESPONSE_TIMER_2, (uint8_t)nrt);
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
    /* Start the trace ring in ALL mode so the full init sequence (including any
     * pre-REQA transport failure) is captured from the first transaction. */
    st25r_trace_init(&s_trace);
    st25r_trace_set_mode(&s_trace, ST25R_TRACE_MODE_ALL);
    s_backend = ST25R3916_BACKEND_IDF_HIGH;
    trace_phase(ST25R_PHASE_INIT_IDENTITY);
    i2c_device_config_t dev_high = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = ST25R3916_I2C_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    esp_err_t e = i2c_master_bus_add_device(bus, &dev_high, &s_dev_high);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "add idf-high device failed: %s", esp_err_to_name(e));
        return e;
    }
    i2c_device_config_t dev_defined = dev_high;
    dev_defined.device_address = I2C_DEVICE_ADDRESS_NOT_USED;
    e = i2c_master_bus_add_device(bus, &dev_defined, &s_dev_defined);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "add idf-defined device failed: %s", esp_err_to_name(e));
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

    trace_phase(ST25R_PHASE_INIT_RESET);
    /* Defensive reset: stop leftover activities, clear tx/rx enables. */
    direct_cmd(ST25R_CMD_STOP_ALL_ACTIVITIES);
    modify8(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN, 0x00);
    vTaskDelay(pdMS_TO_TICKS(2));

    /* Power-on: set to default. */
    e = direct_cmd(ST25R_CMD_SET_DEFAULT);
    if (e != ESP_OK) { ESP_LOGE(TAG, "CMD_SET_DEFAULT failed"); return e; }

    /* Mandatory post-reset protection frame from M5 begin()/ST documentation:
     * FC 04 10 prevents premature internal overheat protection triggering. */
    const uint8_t protection[] = {0x04, 0x10};
    e = direct_cmd_data(ST25R_CMD_TEST_ACCESS, protection, sizeof(protection));
    if (e != ESP_OK) return e;

    trace_phase(ST25R_PHASE_INIT_CONFIG);
    /* Exact M5 I2C configuration after its modify/set operations:
     * IO_CONFIG_1=0x17: i2c_thd0 (400 kHz) + MCU clock disabled (low bits 111).
     * IO_CONFIG_2=0xA4: sup3v + aat_en + io_drv_lvl.
     * Earlier 0x8B/0x30 incorrectly assigned these bitfields to opposite registers. */
    e = wr8(ST25R_REG_IO_CONFIGURATION_1, 0x17);
    if (e != ESP_OK) return e;
    e = wr8(ST25R_REG_IO_CONFIGURATION_2, 0xA4);
    if (e != ESP_OK) return e;

    /* Analog setup from M5 begin(): minimum then normal non-overlap, NFCIP FDT,
     * passive-target/EMD defaults, antenna tuning, and field thresholds. */
    e = wr8b(ST25R_REGB_RESISTIVE_AM_MODULATION, 0x80);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_RESISTIVE_AM_MODULATION, 0x00);
    if (e != ESP_OK) return e;
    e = modify8(ST25R_REG_NFCIP1_PASSIVE_TARGET, 0xF0, 0x50);
    if (e != ESP_OK) return e;
    e = wr8(ST25R_REG_PASSIVE_TARGET_MODULATION, 0x5F);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_EMD_SUPPRESSION_CONFIGURATION, 0x40);
    if (e != ESP_OK) return e;

    /* Antenna settings — CRITICAL for RF coupling (from M5 lib begin()):
     *   TX_DRIVER (0x28) = 0xD0  (tx_am_modulation=13 << 4)
     *   ANTENNA_TUNING_CONTROL_1/2 (0x26/0x27) = 0x82
     *   External field detector thresholds (0x2A/0x2B) = 0x13 / 0x02
     * Without these the field register says "on" but the antenna is not driven. */
    e = wr8(ST25R_REG_TX_DRIVER, 0xD0);
    if (e != ESP_OK) return e;
    e = wr8(0x26, 0x82);
    if (e != ESP_OK) return e;
    e = wr8(0x27, 0x82);
    if (e != ESP_OK) return e;
    e = wr8(0x2A, 0x13);  /* field detector activation threshold */
    if (e != ESP_OK) return e;
    e = wr8(0x2B, 0x02);  /* field detector deactivation threshold */
    if (e != ESP_OK) return e;

    /* Clear FIFO. */
    direct_cmd(ST25R_CMD_CLEAR_FIFO);

    /* Mask all interrupts except error, clear them. */
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT, 0xFF);
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT + 1, 0xFF);
    wr8(ST25R_REG_MASK_MAIN_INTERRUPT + 2, 0xFF);
    clear_interrupts();

    /* Enable oscillator: unmask I_osc, set the 'en' bit (0x80), wait for I_osc IRQ,
     * then remask I_osc. (Matches M5 lib enable_osc().) */
    trace_phase(ST25R_PHASE_INIT_OSCILLATOR);
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
    trace_phase(ST25R_PHASE_INIT_ANALOG);
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
    trace_phase(ST25R_PHASE_FIELD_ON);
    esp_err_t e = direct_cmd(ST25R_CMD_NFC_INITIAL_FIELD_ON);
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));
    /* Match M5Unit-NFC nfc_initial_field_on(): after the field-on guard time,
     * enable both the transmitter and receiver. The previous implementation
     * cleared these bits, leaving the RF request path unable to observe tags. */
    return set_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
}

esp_err_t st25r3916_field_off(void)
{
    /* Stop RF and clear tx/rx enables. */
    direct_cmd(ST25R_CMD_STOP_ALL_ACTIVITIES);
    return clear_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
}

uint8_t st25r3916_measure_amplitude(void)
{
    trace_phase(ST25R_PHASE_DIAGNOSTIC);
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

esp_err_t st25r3916_force_field_on(void)
{
    trace_phase(ST25R_PHASE_FIELD_ON);
    /* Disable the external field detector (en_fd = 0b00) so NFC_INITIAL_FIELD_ON
     * always switches the field on (no collision-avoidance veto). */
    clear_bits(ST25R_REG_OPERATION_CONTROL, 0x03);
    esp_err_t e = direct_cmd(ST25R_CMD_NFC_INITIAL_FIELD_ON);
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));
    set_bits(ST25R_REG_OPERATION_CONTROL, ST25R_OPCTRL_TX_EN | ST25R_OPCTRL_RX_EN);
    return ESP_OK;
}

uint8_t st25r3916_measure_capacitance(void)
{
    trace_phase(ST25R_PHASE_DIAGNOSTIC);
    /* CMD_MEASURE_CAPACITANCE (0xDE) measures capacitance between CSO/CSI.
     * Result is in REG_AD_CONVERTER_OUTPUT (0x25). */
    direct_cmd(0xDE);
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t v = 0;
    rd8(0x25, &v);
    return v;
}

void st25r3916_dump_all(void)
{
    trace_phase(ST25R_PHASE_DIAGNOSTIC);
    printf("dump_all (Space-A 0x00-0x3F):\n");
    for (uint8_t reg = 0x00; reg <= 0x3F; reg++) {
        uint8_t v = 0;
        esp_err_t e = rd8(reg, &v);
        printf("  0x%02X: %s%02X\n", reg, (e == ESP_OK) ? "" : "?? ", v);
    }
}

void st25r3916_debug_dump(void)
{
    trace_phase(ST25R_PHASE_DIAGNOSTIC);
    uint8_t opc=0, mode=0, iso=0, rssi=0, aux=0, rxc1=0, rxc2=0;
    uint8_t ant1=0, ant2=0, txd=0, nrt1=0, nrt2=0, temv=0;
    uint8_t os1=0, os2=0, us1=0, us2=0, corr1=0, corr2=0, emd=0;
    rd8(ST25R_REG_OPERATION_CONTROL, &opc);
    rd8(ST25R_REG_MODE_DEFINITION, &mode);
    rd8(ST25R_REG_ISO14443A_SETTINGS, &iso);
    rd8(ST25R_REG_AUXILIARY_DEFINITION, &aux);
    rd8(ST25R_REG_RECEIVER_CONFIGURATION_1, &rxc1);
    rd8(ST25R_REG_RECEIVER_CONFIGURATION_2, &rxc2);
    rd8(0x26, &ant1);
    rd8(0x27, &ant2);
    rd8(ST25R_REG_TX_DRIVER, &txd);
    rd8(ST25R_REG_NO_RESPONSE_TIMER_1, &nrt1);
    rd8(ST25R_REG_NO_RESPONSE_TIMER_2, &nrt2);
    rd8(ST25R_REG_TIMER_AND_EMV_CONTROL, &temv);
    rd8(0x2D /* REG_RSSI_DISPLAY */, &rssi);
    rd8b(ST25R_REGB_OVERSHOOT_PROTECTION_CONFIG_1, &os1);
    rd8b(ST25R_REGB_OVERSHOOT_PROTECTION_CONFIG_2, &os2);
    rd8b(ST25R_REGB_UNDERSHOOT_PROTECTION_CONFIG_1, &us1);
    rd8b(ST25R_REGB_UNDERSHOOT_PROTECTION_CONFIG_2, &us2);
    rd8b(ST25R_REGB_CORRELATOR_CONFIGURATION_1, &corr1);
    rd8b(ST25R_REGB_CORRELATOR_CONFIGURATION_2, &corr2);
    rd8b(ST25R_REGB_EMD_SUPPRESSION_CONFIGURATION, &emd);
    uint32_t irq = read_main_irq();
    uint16_t fb = fifo_bytes();
    ESP_LOGI(TAG, "regs: OPC=%02X MODE=%02X ISO=%02X AUX=%02X RX1=%02X RX2=%02X RSSI=%02X",
             opc, mode, iso, aux, rxc1, rxc2, rssi);
    ESP_LOGI(TAG, "      ANT1=%02X ANT2=%02X TXD=%02X NRT=%02X%02X TEMV=%02X MAIN_IRQ=%06X FIFO_bytes=%u",
             ant1, ant2, txd, nrt1, nrt2, temv, (unsigned)irq, (unsigned)fb);
    ESP_LOGI(TAG, "      SpaceB: OS=%02X/%02X US=%02X/%02X CORR=%02X/%02X EMD=%02X",
             os1, os2, us1, us2, corr1, corr2, emd);
}

esp_err_t st25r3916_configure_nfca(void)
{
    trace_phase(ST25R_PHASE_INIT_CONFIG);
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

    /* Reader receive-path Space-B settings from M5 configure_nfc_a(). These
     * shape overshoot/undershoot protection and ISO14443-A correlation. */
    e = wr8b(ST25R_REGB_OVERSHOOT_PROTECTION_CONFIG_1, 0x40);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_OVERSHOOT_PROTECTION_CONFIG_2, 0x03);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_UNDERSHOOT_PROTECTION_CONFIG_1, 0x40);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_UNDERSHOOT_PROTECTION_CONFIG_2, 0x03);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_CORRELATOR_CONFIGURATION_1, 0x47);
    if (e != ESP_OK) return e;
    e = wr8b(ST25R_REGB_CORRELATOR_CONFIGURATION_2, 0x00);
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
    trace_phase(ST25R_PHASE_FIELD_ON);
    e = st25r3916_field_on();
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "configure_nfca: field_on failed: %s", esp_err_to_name(e));
        return e;
    }
    return ESP_OK;
}

/* ---- REQA/WUPA: get ATQA (shared helper) ---- */
static esp_err_t nfca_wake(uint16_t *atqa, uint8_t wake_cmd)
{
    *atqa = 0;
    trace_phase(ST25R_PHASE_REQUEST_SETUP);
    esp_err_t e = set_frame_wait_time(4); /* M5 TIMEOUT_REQ_WUP */
    if (e != ESP_OK) return e;
    e = wr8(ST25R_REG_ISO14443A_SETTINGS, 0x01); /* antcl */
    if (e != ESP_OK) return e;
    e = set_bits(ST25R_REG_AUXILIARY_DEFINITION, 0x80); /* no_crc_rx */
    if (e != ESP_OK) return e;
    clear_interrupts();
    direct_cmd(ST25R_CMD_CLEAR_FIFO);

    trace_phase(ST25R_PHASE_REQUEST_TRANSMIT);
    e = direct_cmd(wake_cmd);
    if (e != ESP_OK) return e;

    trace_phase(ST25R_PHASE_IRQ_WAIT);
    uint32_t irq = wait_irq(ST25R_IRQ_RXE | ST25R_IRQ_RXS | ST25R_IRQ_COL, 50);
    const char *wn = (wake_cmd == ST25R_CMD_TRANSMIT_REQA) ? "reqa" : "wupa";
    uint8_t fifo_status1 = 0, fifo_status2 = 0, collision = 0;
    rd8(ST25R_REG_FIFO_STATUS_1, &fifo_status1);
    rd8(ST25R_REG_FIFO_STATUS_2, &fifo_status2);
    rd8(ST25R_REG_COLLISION_DISPLAY, &collision);
    ESP_LOGI(TAG, "%s: irq=%06X timer=%02X error=%02X fifo=%u raw=%02X/%02X coll=%02X rxs=%d rxe=%d col=%d",
             wn, (unsigned)irq, s_last_timer_irq, s_last_error_irq, (unsigned)fifo_bytes(),
             fifo_status1, fifo_status2, collision,
             !!(irq & ST25R_IRQ_RXS), !!(irq & ST25R_IRQ_RXE), !!(irq & ST25R_IRQ_COL));
    if (!(irq & ST25R_IRQ_RXE)) {
        if (irq & ST25R_IRQ_RXS) {
            int64_t dl = esp_timer_get_time() + 50 * 1000;
            while (esp_timer_get_time() < dl) {
                if (fifo_bytes() >= 2) { irq |= ST25R_IRQ_RXE; break; }
                vTaskDelay(1); /* one real FreeRTOS tick; pdMS_TO_TICKS(1) is 0 at 100 Hz */
            }
        }
        if (!(irq & (ST25R_IRQ_RXE | ST25R_IRQ_COL))) return ESP_ERR_NOT_FOUND;
    }
    trace_phase(ST25R_PHASE_FIFO_READ);
    uint8_t rbuf[2] = {0, 0};
    size_t got = 0;
    e = fifo_read(rbuf, 2, &got);
    if (e != ESP_OK) return e;
    if (got == 2) {
        *atqa = ((uint16_t)rbuf[1] << 8) | (uint16_t)rbuf[0];
        return ESP_OK;
    }
    /* Multiple tags can collide before a reliable two-byte ATQA is available.
     * COL is positive RF evidence and must transition into anticollision rather
     * than being collapsed into ESP_FAIL. ATQA remains 0/unknown in this case. */
    if (irq & ST25R_IRQ_COL) {
        ESP_LOGI(TAG, "%s: collision without complete ATQA; continue to anticollision", wn);
        return ESP_OK;
    }
    return ESP_FAIL;
}

/* ---- REQA: get ATQA ---- */
esp_err_t st25r3916_reqa(uint16_t *atqa)
{
    return nfca_wake(atqa, ST25R_CMD_TRANSMIT_REQA);
}

/* ---- WUPA: wake halted tags, get ATQA ---- */
esp_err_t st25r3916_wupa(uint16_t *atqa)
{
    /* Keep the carrier continuously established. WUPA itself wakes a halted
     * PICC; STOP_ALL_ACTIVITIES + field restart here would depower the tag.
     * This matches M5Unit-NFC's nfca_request_wakeup() sequence. */
    return nfca_wake(atqa, ST25R_CMD_TRANSMIT_WUPA);
}

/* ---- Anticollision + select for one cascade level ---- */
static esp_err_t nfca_anticoll_select(uint8_t sel, uint8_t *uid_out, uint8_t *sak_out)
{
    trace_phase(ST25R_PHASE_ANTICOLLISION);
    esp_err_t e = set_frame_wait_time(8); /* M5 TIMEOUT_ANTICOLL */
    if (e != ESP_OK) return e;
    e = wr8(ST25R_REG_ISO14443A_SETTINGS, 0x01); /* antcl */
    if (e != ESP_OK) return e;
    e = clear_bits(ST25R_REG_AUXILIARY_DEFINITION, 0x80); /* CRC off for anticollision */
    if (e != ESP_OK) return e;

    /* Port of M5Unit-NFC nfca_anti_collision(): on each collision, read the
     * partial UID bytes and collision position, choose the 1 branch, extend
     * NVB, and retry. This deterministically selects one PICC among many. */
    uint8_t frame[7] = { sel, 0x20 };
    uint8_t rbuf[5] = {0};
    uint8_t send_bytes = 2;
    uint8_t send_bits = 0;
    uint8_t rbuf_offset = 0;
    uint8_t collision_byte = 1;
    bool collision = false;

    for (unsigned retry = 0; retry < 32; ++retry) {
        e = clear_interrupts();
        if (e != ESP_OK) return e;
        e = direct_cmd(ST25R_CMD_CLEAR_FIFO);
        if (e != ESP_OK) return e;
        e = fifo_write(frame, send_bytes + (send_bits != 0));
        if (e != ESP_OK) return e;
        e = set_tx_bytes(send_bytes, send_bits);
        if (e != ESP_OK) return e;
        e = direct_cmd(ST25R_CMD_TRANSMIT_WITHOUT_CRC);
        if (e != ESP_OK) return e;

        uint32_t irq = wait_irq(ST25R_IRQ_RXE | ST25R_IRQ_COL, 50);
        collision = (irq & ST25R_IRQ_COL) != 0;
        if (!collision && !(irq & ST25R_IRQ_RXE)) return ESP_FAIL;

        size_t actual = 0;
        e = fifo_read(rbuf + rbuf_offset, sizeof(rbuf) - rbuf_offset, &actual);
        if (e != ESP_OK || actual == 0) return e != ESP_OK ? e : ESP_FAIL;
        uint8_t collision_display = 0;
        e = rd8(ST25R_REG_COLLISION_DISPLAY, &collision_display);
        if (e != ESP_OK) return e;

        ESP_LOGI(TAG, "anticoll sel=%02X retry=%u irq=%04X fifo=%u coll_disp=%02X",
                 sel, retry, (unsigned)irq, (unsigned)actual, collision_display);

        if (collision) {
            const uint8_t collision_bytes = (collision_display >> 4) & 0x0F;
            const uint8_t collision_bits = (collision_display >> 1) & 0x07;
            collision_byte = rbuf[rbuf_offset + actual - 1] | (uint8_t)(1U << collision_bits);

            send_bytes = collision_bytes + (collision_bits == 0x07);
            send_bits = (collision_bits + 1) & 0x07;
            frame[1] = (uint8_t)((send_bytes << 4) | send_bits); /* NVB */
            memcpy(frame + 2 + rbuf_offset, rbuf + rbuf_offset, actual);
            frame[send_bytes] = collision_byte;
            rbuf_offset = (uint8_t)(actual - 1);
        }

        if (send_bits) {
            rbuf[rbuf_offset] >>= send_bits;
            rbuf[rbuf_offset] <<= send_bits;
            rbuf[rbuf_offset] |= collision_byte;
        }
        if (!collision) break;
        vTaskDelay(1);
    }
    if (collision) return ESP_ERR_TIMEOUT;

    /* One cascade-level UID plus BCC has been resolved. */
    if ((uint8_t)(rbuf[0] ^ rbuf[1] ^ rbuf[2] ^ rbuf[3]) != rbuf[4]) {
        ESP_LOGE(TAG, "anticoll BCC mismatch: %02X %02X %02X %02X / %02X",
                 rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);
        return ESP_FAIL;
    }

    /* SELECT: SEL, NVB=0x70, UID(4) + BCC(1), chip appends CRC. */
    trace_phase(ST25R_PHASE_SELECT);
    uint8_t sel_frame[7] = { sel, 0x70, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4] };
    e = clear_interrupts();
    if (e != ESP_OK) return e;
    e = direct_cmd(ST25R_CMD_CLEAR_FIFO);
    if (e != ESP_OK) return e;
    e = fifo_write(sel_frame, sizeof(sel_frame));
    if (e != ESP_OK) return e;
    e = set_tx_bytes(sizeof(sel_frame), 0);
    if (e != ESP_OK) return e;
    e = direct_cmd(ST25R_CMD_TRANSMIT_WITH_CRC);
    if (e != ESP_OK) return e;

    uint32_t irq = wait_irq(ST25R_IRQ_RXE | ST25R_IRQ_COL, 50);
    if (!(irq & ST25R_IRQ_RXE) || (irq & ST25R_IRQ_COL)) return ESP_FAIL;

    uint8_t sakbuf[3] = {0};
    size_t sgot = 0;
    e = fifo_read(sakbuf, sizeof(sakbuf), &sgot);
    if (e != ESP_OK || sgot != sizeof(sakbuf)) return e != ESP_OK ? e : ESP_FAIL;
    *sak_out = sakbuf[0];
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

    trace_phase(ST25R_PHASE_IDENTIFY);
    /* Establish the field before each high-level read. nfc_initial_field_on()
     * enables TX/RX after its guard interval, matching M5Unit-NFC. Keep the
     * field sequence intact across REQA -> WUPA -> anticollision. */
    esp_err_t e = st25r3916_field_on();
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));

    uint16_t atqa = 0;
    e = st25r3916_reqa(&atqa);
    if (e == ESP_ERR_NOT_FOUND) e = st25r3916_wupa(&atqa);
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
