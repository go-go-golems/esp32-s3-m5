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

/* Add the ST25R3916 as a device on the given I2C bus and bring the chip up. */
esp_err_t st25r3916_init(i2c_master_bus_handle_t bus);

/* Read the chip identity register. */
esp_err_t st25r3916_read_id(st25r3916_id_t *out);

/* RF field control. */
esp_err_t st25r3916_field_on(void);
esp_err_t st25r3916_field_off(void);

/* Configure the chip for ISO14443-A reader mode (called by init; re-callable). */
esp_err_t st25r3916_configure_nfca(void);

/* Poll once for an ISO14443-A tag. Returns ESP_OK + out filled if a tag was found,
 * ESP_ERR_NOT_FOUND if no tag answered, ESP_FAIL on protocol error.
 * Leaves the field ON after a successful poll. */
esp_err_t st25r3916_poll_nfca(nfc_picc_t *out);

#ifdef __cplusplus
}
#endif
