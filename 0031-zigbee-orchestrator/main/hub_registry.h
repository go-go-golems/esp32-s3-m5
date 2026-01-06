#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "hub_types.h"

esp_err_t hub_registry_init(void);
esp_err_t hub_registry_add(const hub_device_t *in, hub_device_t *out_created);
esp_err_t hub_registry_get(uint32_t id, hub_device_t *out);
esp_err_t hub_registry_update(uint32_t id, const hub_device_t *in);
esp_err_t hub_registry_remove(uint32_t id);

// Snapshot API for HTTP responses.
esp_err_t hub_registry_snapshot(hub_device_t *out, size_t max_out, size_t *out_n);

