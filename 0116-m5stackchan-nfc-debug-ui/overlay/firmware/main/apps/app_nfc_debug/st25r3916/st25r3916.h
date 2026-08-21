/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * Minimal ESP-IDF driver for the ST25R3916 NFC reader IC over I2C.
 *
 * Scope (Phase 1): ISO14443-A read only — REQA -> ATQA -> anticollision -> UID/SAK.
 * Mirrors the I2C style of the StackChan firmware's PY32IOExpander_Class driver
 * (driver/i2c_master.h: i2c_master_bus_add_device + transmit/transmit_receive).
 *
 * Reference sequence: M5Unit-NFC/src/unit/unit_ST25R3916.cpp::begin() and
 * unit_ST25R3916_nfca.cpp::nfca_request_wakeup / nfca_anti_collision / select.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t type;      /* IC type; should be ST25R_VALID_IDENTIFY_TYPE (0x05) */
    uint8_t revision; /* IC revision */
} st25r3916_id_t;

typedef struct {
    uint8_t  uid[10];
    uint8_t  uid_len;   /* 4, 7, or 10 */
    uint16_t atqa;
    uint8_t  sak;
    char     type_str[24]; /* provisional type guess from SAK */
} nfc_picc_t;

typedef enum {
    ST25R_TRANSPORT_NONE = 0,
    ST25R_TRANSPORT_READ_A,
    ST25R_TRANSPORT_WRITE_A,
    ST25R_TRANSPORT_READ_B,
    ST25R_TRANSPORT_WRITE_B,
    ST25R_TRANSPORT_DIRECT_COMMAND,
    ST25R_TRANSPORT_FIFO_READ,
    ST25R_TRANSPORT_FIFO_WRITE,
} st25r3916_transport_operation_t;

typedef struct {
    uint32_t total;
    uint32_t succeeded;
    uint32_t failed;
    uint32_t timeouts;
    uint32_t invalid_state;
    uint32_t other_errors;
    st25r3916_transport_operation_t last_operation;
    uint8_t last_key;
    esp_err_t last_error;
    uint32_t last_elapsed_us;
} st25r3916_transport_stats_t;

typedef struct {
    uint8_t operation_control;
    uint8_t rssi;
    uint16_t nrt;
    uint32_t main_irq;
    uint8_t timer_irq;
    uint8_t error_irq;
    uint8_t collision;
    uint16_t fifo_bytes;
    uint8_t capacitance;
} st25r3916_diagnostics_t;

typedef struct {
    char name[12];
    bool space_b;
    uint8_t address;
    uint8_t expected;
    uint8_t actual;
    esp_err_t error;
} st25r3916_register_check_t;

/* Add the ST25R3916 as a device on the given I2C bus and bring the chip up. */
esp_err_t st25r3916_init(i2c_master_bus_handle_t bus);

/* Remove the device from the shared bus. Call only after the worker has stopped. */
void st25r3916_deinit(void);

/* Read the chip identity register. */
esp_err_t st25r3916_read_id(st25r3916_id_t *out);

/* Copy low-level transport counters without performing another transaction. */
void st25r3916_get_transport_stats(st25r3916_transport_stats_t *out);
void st25r3916_reset_transport_stats(void);

/* Capture current RF/timer state plus the last IRQ/FIFO/collision evidence. */
esp_err_t st25r3916_get_diagnostics(st25r3916_diagnostics_t *out);

/* Read the stable expected Space-A/Space-B configuration set once. */
esp_err_t st25r3916_verify_configuration(st25r3916_register_check_t *checks,
                                         size_t capacity, size_t *count);

/* RF field control. */
esp_err_t st25r3916_field_on(void);
esp_err_t st25r3916_field_off(void);

/* Configure the chip for ISO14443-A reader mode (called by init; re-callable). */
esp_err_t st25r3916_configure_nfca(void);

/* Print key register values for debugging (operation control, mode, IRQ, RSSI, FIFO). */
void st25r3916_debug_dump(void);

/* Dump ALL Space-A registers 0x00-0x3F (one line each) for full comparison
 * against the M5 lib dump_regs() reference. */
void st25r3916_dump_all(void);

/* Measure the RF amplitude on the RFI inputs (CMD_MEASURE_AMPLITUDE).
 * Returns the 8-bit amplitude display value (reg 0x36); higher when a tag
 * loads the field. Useful as a "metal detector" to locate the coil. */
uint8_t st25r3916_measure_amplitude(void);

/* Enable/disable the transmitter and receiver (OPERATION_CONTROL tx_en|rx_en).
 * Some measurements (amplitude) need the receiver enabled. */
void st25r3916_set_tx_rx(bool on);

/* Send ISO14443-A REQA once and return the ATQA. Returns ESP_OK + atqa filled if a
 * tag answered, ESP_ERR_NOT_FOUND if no tag, ESP_FAIL on error. */
esp_err_t st25r3916_reqa(uint16_t *atqa);

/* Send ISO14443-A WUPA (Wake-Up All) — wakes HALTED tags that REQA will not.
 * Clears any halt state first (CMD_STOP_ALL_ACTIVITIES + field on), then WUPA.
 * Use this when a tag may have been halted by a prior SELECT. */
esp_err_t st25r3916_wupa(uint16_t *atqa);

/* Force the RF field on: disable the external field detector (en_fd=0), issue
 * NFC_INITIAL_FIELD_ON, then enable tx+rx. Use for diagnostics when the auto
 * field detector might block field-on. */
esp_err_t st25r3916_force_field_on(void);

/* Measure the capacitance on the CSO/CSI antenna pins (CMD_MEASURE_CAPACITANCE).
 * Returns the 8-bit ADC output (reg 0x25). A stable non-zero value means the
 * antenna coil is connected; flat/0 means the antenna feed is open. */
uint8_t st25r3916_measure_capacitance(void);

/* Poll once for an ISO14443-A tag. Returns ESP_OK + out filled if a tag was found,
 * ESP_ERR_NOT_FOUND if no tag answered, ESP_FAIL on protocol error.
 * Leaves the field ON after a successful poll. */
esp_err_t st25r3916_poll_nfca(nfc_picc_t *out);

#ifdef __cplusplus
}
#endif
