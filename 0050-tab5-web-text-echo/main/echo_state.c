#include "echo_state.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static struct {
    SemaphoreHandle_t mutex;
    uint32_t version;
    size_t len;
    char text[ECHO_STATE_MAX_TEXT_BYTES + 1];
} s_state;

size_t echo_state_max_text_bytes(void) {
    return ECHO_STATE_MAX_TEXT_BYTES;
}

esp_err_t echo_state_init(void) {
    memset(&s_state, 0, sizeof(s_state));
    s_state.mutex = xSemaphoreCreateMutex();
    if (!s_state.mutex) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static esp_err_t echo_state_set_locked(const char *text, size_t len) {
    if (len > ECHO_STATE_MAX_TEXT_BYTES) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(s_state.text, text, len);
    s_state.text[len] = '\0';
    s_state.len = len;
    s_state.version += 1;
    return ESP_OK;
}

esp_err_t echo_state_set(const char *text, size_t len) {
    if (!s_state.mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!text && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state.mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    const esp_err_t err = echo_state_set_locked(text ? text : "", len);
    xSemaphoreGive(s_state.mutex);
    return err;
}

esp_err_t echo_state_clear(void) {
    return echo_state_set("", 0);
}

esp_err_t echo_state_snapshot(echo_state_snapshot_t *out) {
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_state.mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_state.mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    out->version = s_state.version;
    out->len = s_state.len;
    memcpy(out->text, s_state.text, s_state.len + 1);
    xSemaphoreGive(s_state.mutex);
    return ESP_OK;
}
