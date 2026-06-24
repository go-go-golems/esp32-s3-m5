#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Same-position RPico-socket adapter mapping:
//   Pico physical pin  9 / GP6 / SDA1 -> Waveshare left position  9 / GPIO50
//   Pico physical pin 10 / GP7 / SCL1 -> Waveshare left position 10 / GPIO49
#define PICOCALC_KBD_I2C_SDA_GPIO      50
#define PICOCALC_KBD_I2C_SCL_GPIO      49
#define PICOCALC_KBD_I2C_SPEED_HZ      10000
#define PICOCALC_KBD_I2C_ADDR          0x1F

#define PICOCALC_KBD_REG_STATUS        0x04
#define PICOCALC_KBD_REG_FIFO          0x09
#define PICOCALC_KBD_COUNT_MASK        0x1F
#define PICOCALC_KBD_CAPS_LOCK_MASK    0x20
#define PICOCALC_KBD_NUM_LOCK_MASK     0x40

#define PICOCALC_KBD_STATE_PRESSED     1
#define PICOCALC_KBD_STATE_REPEATED    2
#define PICOCALC_KBD_STATE_RELEASED    3

typedef struct {
    uint8_t state;
    uint8_t key;
    bool valid;
} picocalc_key_event_t;

typedef struct {
    bool initialized;
    uint8_t last_status;
    uint32_t error_count;
    uint32_t recover_count;
    esp_err_t last_error;
} picocalc_keyboard_diag_t;

esp_err_t picocalc_keyboard_init(void);
esp_err_t picocalc_keyboard_recover(void);
esp_err_t picocalc_keyboard_probe_address(uint8_t addr, int timeout_ms);
esp_err_t picocalc_keyboard_read_register(uint8_t reg, uint8_t *dst, size_t len);
esp_err_t picocalc_keyboard_read_status(uint8_t *status);
uint8_t picocalc_keyboard_fifo_count(uint8_t status);
esp_err_t picocalc_keyboard_poll_event(picocalc_key_event_t *event);
void picocalc_keyboard_get_diag(picocalc_keyboard_diag_t *diag);
const char *picocalc_keyboard_key_name(uint8_t key);
const char *picocalc_keyboard_state_name(uint8_t state);

#ifdef __cplusplus
}
#endif
