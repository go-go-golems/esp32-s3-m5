/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <Arduino.h>

#define MODULE_QRCODE_DEBUG 0

#define _LOG_DEBUG(x, ...)        \
    if (MODULE_QRCODE_DEBUG) {    \
        printf("[debug] ");       \
        printf(x, ##__VA_ARGS__); \
    }

#define _LOG_ERROR(x, ...) \
    printf("[error] ");    \
    printf(x, ##__VA_ARGS__);

inline void debug_print_buffer(const uint8_t* buffer, size_t len)
{
#if MODULE_QRCODE_DEBUG
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
#endif
}
