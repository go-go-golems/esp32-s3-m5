/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * ST25R3916 register + direct-command constants.
 * Extracted from M5Unit-NFC/src/unit/ST25R3916_definition.hpp (the authoritative
 * machine-usable register map; cross-reference against the ST25R3916B datasheet).
 * Only the subset needed for ISO14443-A polling is included here.
 */
#pragma once
#include <stdint.h>

/* ---- I2C address ---- */
#define ST25R3916_I2C_ADDR          0x50

/* ---- Operation-mode command bytes (prefix before register address) ---- */
#define ST25R_OP_TRAILER_MASK        0x3F
#define ST25R_OP_WRITE_REGISTER      0x00   /* 00xxxxxx */
#define ST25R_OP_READ_REGISTER       0x40   /* 01xxxxxx */
#define ST25R_OP_LOAD_FIFO           0x80   /* load FIFO data: byte 0x80 then data */

/* ---- Registers (Space A) ---- */
#define ST25R_REG_IO_CONFIGURATION_1            0x00
#define ST25R_REG_IO_CONFIGURATION_2            0x01
#define ST25R_REG_OPERATION_CONTROL             0x02
#define ST25R_REG_MODE_DEFINITION               0x03
#define ST25R_REG_BITRATE_DEFINITION            0x04
#define ST25R_REG_ISO14443A_SETTINGS            0x05
#define ST25R_REG_AUXILIARY_DEFINITION          0x0A
#define ST25R_REG_RECEIVER_CONFIGURATION_1      0x0B
#define ST25R_REG_RECEIVER_CONFIGURATION_2      0x0C
#define ST25R_REG_RECEIVER_CONFIGURATION_3      0x0D
#define ST25R_REG_RECEIVER_CONFIGURATION_4      0x0E
#define ST25R_REG_NO_RESPONSE_TIMER_1           0x10
#define ST25R_REG_NO_RESPONSE_TIMER_2           0x11
#define ST25R_REG_TIMER_AND_EMV_CONTROL         0x12
#define ST25R_REG_MASK_MAIN_INTERRUPT           0x16
#define ST25R_REG_MASK_TIMER_AND_NFC_INTERRUPT  0x17
#define ST25R_REG_MASK_ERROR_AND_WAKEUP_IRQ     0x18
#define ST25R_REG_MAIN_INTERRUPT                0x1A
#define ST25R_REG_TIMER_AND_NFC_INTERRUPT       0x1B
#define ST25R_REG_ERROR_AND_WAKEUP_INTERRUPT    0x1C
#define ST25R_REG_FIFO_STATUS_1                 0x1E
#define ST25R_REG_FIFO_STATUS_2                 0x1F
#define ST25R_REG_COLLISION_DISPLAY             0x20
#define ST25R_REG_NUM_TX_BYTES_1                0x22
#define ST25R_REG_NUM_TX_BYTES_2                0x23
#define ST25R_REG_TX_DRIVER                      0x28
#define ST25R_REG_REGULATOR_VOLTAGE_CONTROL     0x2C
#define ST25R_REG_IC_IDENTITY                    0x3F

/* ---- Direct commands ---- */
#define ST25R_CMD_SET_DEFAULT                  0xC1
#define ST25R_CMD_STOP_ALL_ACTIVITIES           0xC2
#define ST25R_CMD_TRANSMIT_WITH_CRC             0xC4
#define ST25R_CMD_TRANSMIT_WITHOUT_CRC          0xC5
#define ST25R_CMD_TRANSMIT_REQA                 0xC6
#define ST25R_CMD_TRANSMIT_WUPA                 0xC7
#define ST25R_CMD_NFC_INITIAL_FIELD_ON          0xC8
#define ST25R_CMD_RESET_RX_GAIN                 0xD5
#define ST25R_CMD_ADJUST_REGULATORS            0xD6
#define ST25R_CMD_CLEAR_FIFO                    0xDB

/* ---- Misc constants ---- */
#define ST25R_VALID_IDENTIFY_TYPE              0x05   /* ST25R3916/7 */
#define ST25R_MAX_FIFO_DEPTH                   512

/* ---- REG_OPERATION_CONTROL bits (0x02) ---- */
#define ST25R_OPCTRL_TX_EN                     0x08   /* Enables Tx operation */
#define ST25R_OPCTRL_RX_EN                     0x40   /* Enables Rx operation */
#define ST25R_OPCTRL_EN                        0x80   /* Enables oscillator + regulator (Ready mode) */
#define ST25R_OPCTRL_WU                        0x04   /* Enables Wake-up mode */

/* ---- REG_TIMER_AND_EMV_CONTROL bits (0x12) ---- */
#define ST25R_TIMER_NRT_STEP                   0x01   /* 0: 64/fc, 1: 4096/fc */

/* ---- REG_MAIN_INTERRUPT (0x1A) bit values (low byte of the 24-bit IRQ word) ---- */
#define ST25R_IRQ_OSC                          0x80   /* oscillator frequency stable */
#define ST25R_IRQ_RXS                          0x20   /* start of receive */
#define ST25R_IRQ_RXE                          0x10   /* end of receive */
#define ST25R_IRQ_COL                          0x04   /* bit collision */
