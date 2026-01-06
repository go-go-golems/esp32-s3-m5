#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_event.h"

#include "gw_types.h"

esp_err_t gw_bus_start(void);
esp_event_loop_handle_t gw_bus_get_loop(void);

// Posting helpers (for console-driven testing).
esp_err_t gw_bus_post_permit_join(uint16_t seconds, uint64_t req_id, TickType_t timeout);
esp_err_t gw_bus_post_onoff(uint16_t short_addr, uint8_t dst_ep, uint8_t cmd_id, uint64_t req_id, TickType_t timeout);

// Monitoring/telemetry.
void gw_bus_set_monitor_enabled(bool enabled);
bool gw_bus_monitor_enabled(void);
uint64_t gw_bus_monitor_dropped(void);

const char *gw_event_id_to_str(int32_t id);
