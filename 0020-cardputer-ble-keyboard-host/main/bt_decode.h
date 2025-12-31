#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_bt_defs.h"
#include "esp_gatt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t value;
    const char *name;
    const char *desc;
} bt_decode_entry_t;

const char *bt_decode_gatt_status_name(esp_gatt_status_t status);
const char *bt_decode_gatt_status_desc(esp_gatt_status_t status);

const char *bt_decode_gatt_conn_reason_name(esp_gatt_conn_reason_t reason);
const char *bt_decode_gatt_conn_reason_desc(esp_gatt_conn_reason_t reason);

const char *bt_decode_bt_status_name(esp_bt_status_t status);
const char *bt_decode_bt_status_desc(esp_bt_status_t status);

void bt_decode_print_all(void);

#ifdef __cplusplus
}
#endif

