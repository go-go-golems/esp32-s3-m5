#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char host[64];
    uint16_t port;
    bool has_saved;
    bool has_runtime;
} stream_target_t;

esp_err_t stream_client_init(void);
esp_err_t stream_client_set_target(const char *host, uint16_t port, bool save_to_nvs);
esp_err_t stream_client_clear_target(void);
void stream_client_get_target(stream_target_t *out);
void stream_client_start(void);
void stream_client_stop(void);
bool stream_client_is_running(void);
bool stream_client_is_connected(void);

#ifdef __cplusplus
}
#endif
