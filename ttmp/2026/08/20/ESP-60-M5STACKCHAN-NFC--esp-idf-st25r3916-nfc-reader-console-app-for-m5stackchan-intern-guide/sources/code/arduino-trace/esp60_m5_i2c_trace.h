/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 *
 * In-memory trace ABI used by the instrumented official Arduino comparison.
 * Recording performs no serial I/O. Detect.ino drains records only after an
 * NFC operation so observation does not add delays between I2C transactions.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP60_M5_I2C_WRITE = 1,
    ESP60_M5_I2C_READ = 2,
    ESP60_M5_I2C_WRITE_READ = 3,
} esp60_m5_i2c_kind_t;

typedef enum {
    ESP60_M5_I2C_FAIL_NONE = 0,
    ESP60_M5_I2C_FAIL_START = 1U << 0,
    ESP60_M5_I2C_FAIL_RESTART = 1U << 1,
    ESP60_M5_I2C_FAIL_WRITE = 1U << 2,
    ESP60_M5_I2C_FAIL_READ = 1U << 3,
    ESP60_M5_I2C_FAIL_STOP = 1U << 4,
} esp60_m5_i2c_failure_stage_t;

typedef struct __attribute__((packed)) {
    uint32_t sequence;
    uint32_t timestamp_us;
    uint32_t elapsed_us;
    uint16_t write_len;
    uint16_t read_len;
    uint8_t kind;
    uint8_t key;
    uint8_t failure_stage;
} esp60_m5_i2c_event_t;

typedef struct {
    uint32_t total;
    uint32_t succeeded;
    uint32_t failed;
    uint32_t dropped;
    uint32_t buffered;
} esp60_m5_i2c_stats_t;

void esp60_m5_i2c_trace_reset(void);
void esp60_m5_i2c_trace_get_stats(esp60_m5_i2c_stats_t* out);
size_t esp60_m5_i2c_trace_drain(esp60_m5_i2c_event_t* out, size_t capacity);

#ifdef __cplusplus
}
#endif
