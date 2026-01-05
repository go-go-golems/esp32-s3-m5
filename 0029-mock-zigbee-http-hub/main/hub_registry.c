/*
 * In-memory device registry for tutorial 0029.
 *
 * Keeps the demo simple: a bounded list protected by a mutex.
 * Persistence (NVS snapshot) is intentionally deferred for a later step.
 */

#include "hub_registry.h"

#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"

static const char *TAG = "hub_registry_0029";

static SemaphoreHandle_t s_mu = NULL;

static hub_device_t s_devices[32];
static size_t s_n = 0;
static uint32_t s_next_id = 1;

esp_err_t hub_registry_init(void) {
    if (!s_mu) {
        s_mu = xSemaphoreCreateMutex();
    }
    if (!s_mu) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static int find_idx_by_id(uint32_t id) {
    for (size_t i = 0; i < s_n; i++) {
        if (s_devices[i].id == id) {
            return (int)i;
        }
    }
    return -1;
}

esp_err_t hub_registry_add(const hub_device_t *in, hub_device_t *out_created) {
    if (!s_mu || !in || !out_created) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_mu, portMAX_DELAY);
    if (s_n >= (sizeof(s_devices) / sizeof(s_devices[0]))) {
        xSemaphoreGive(s_mu);
        return ESP_ERR_NO_MEM;
    }

    hub_device_t d = *in;
    d.id = s_next_id++;

    // Ensure name is always NUL-terminated.
    d.name[sizeof(d.name) - 1] = '\0';

    s_devices[s_n++] = d;
    *out_created = d;
    xSemaphoreGive(s_mu);

    ESP_LOGI(TAG, "added device id=%" PRIu32 " type=%d caps=0x%08" PRIx32 " name=%s",
             d.id, (int)d.type, d.caps, d.name);
    return ESP_OK;
}

esp_err_t hub_registry_get(uint32_t id, hub_device_t *out) {
    if (!s_mu || !out) {
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreTake(s_mu, portMAX_DELAY);
    int idx = find_idx_by_id(id);
    if (idx < 0) {
        xSemaphoreGive(s_mu);
        return ESP_ERR_NOT_FOUND;
    }
    *out = s_devices[(size_t)idx];
    xSemaphoreGive(s_mu);
    return ESP_OK;
}

esp_err_t hub_registry_update(uint32_t id, const hub_device_t *in) {
    if (!s_mu || !in) {
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreTake(s_mu, portMAX_DELAY);
    int idx = find_idx_by_id(id);
    if (idx < 0) {
        xSemaphoreGive(s_mu);
        return ESP_ERR_NOT_FOUND;
    }

    hub_device_t d = *in;
    d.id = id;
    d.name[sizeof(d.name) - 1] = '\0';
    s_devices[(size_t)idx] = d;

    xSemaphoreGive(s_mu);
    return ESP_OK;
}

esp_err_t hub_registry_remove(uint32_t id) {
    if (!s_mu) {
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreTake(s_mu, portMAX_DELAY);
    int idx = find_idx_by_id(id);
    if (idx < 0) {
        xSemaphoreGive(s_mu);
        return ESP_ERR_NOT_FOUND;
    }
    const size_t i = (size_t)idx;
    s_devices[i] = s_devices[s_n - 1];
    s_n--;
    xSemaphoreGive(s_mu);
    return ESP_OK;
}

esp_err_t hub_registry_snapshot(hub_device_t *out, size_t max_out, size_t *out_n) {
    if (!s_mu || !out_n) {
        return ESP_ERR_INVALID_STATE;
    }
    if (max_out > 0 && !out) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mu, portMAX_DELAY);
    const size_t n = (s_n < max_out) ? s_n : max_out;
    for (size_t i = 0; i < n; i++) {
        out[i] = s_devices[i];
    }
    *out_n = n;
    xSemaphoreGive(s_mu);
    return ESP_OK;
}
