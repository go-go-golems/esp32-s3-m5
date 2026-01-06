#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t gw_zb_start(void);
bool gw_zb_started(void);

// Low-level ZNSP request helper (host↔NCP protocol).
esp_err_t gw_zb_znsp_request(uint16_t id,
                             const void *buffer,
                             uint16_t len,
                             void *output,
                             uint16_t *outlen,
                             TickType_t timeout);

// Primary channel mask (applied before network formation; also persisted in NVS).
// Zigbee channels are 11..26 and map to bits in a uint32 mask: (1U << channel).
esp_err_t gw_zb_set_primary_channel_mask(uint32_t mask, bool persist, bool apply_now, TickType_t timeout);
uint32_t gw_zb_get_primary_channel_mask(void);

esp_err_t gw_zb_request_permit_join(uint16_t seconds, uint64_t req_id, TickType_t timeout);
esp_err_t gw_zb_request_onoff(uint16_t short_addr, uint8_t dst_ep, uint8_t cmd_id, uint64_t req_id, TickType_t timeout);
