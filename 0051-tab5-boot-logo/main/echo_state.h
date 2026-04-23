#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { ECHO_STATE_MAX_TEXT_BYTES = 512 };

typedef struct {
    uint32_t version;
    size_t len;
    char text[ECHO_STATE_MAX_TEXT_BYTES + 1];
} echo_state_snapshot_t;

esp_err_t echo_state_init(void);
esp_err_t echo_state_set(const char *text, size_t len);
esp_err_t echo_state_clear(void);
esp_err_t echo_state_snapshot(echo_state_snapshot_t *out);
size_t echo_state_max_text_bytes(void);

#ifdef __cplusplus
}
#endif
