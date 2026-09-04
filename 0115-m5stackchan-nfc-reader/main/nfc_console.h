/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * esp_console commands for the NFC reader: nfc-scan, nfc-probe, nfc-field,
 * nfc-read, nfc-poll.
 */
#pragma once
#include "driver/i2c_master.h"

/* Register all NFC console commands. Requires the I2C bus handle that the
 * ST25R3916 driver was initialized with. */
void nfc_console_register(i2c_master_bus_handle_t i2c_bus);
