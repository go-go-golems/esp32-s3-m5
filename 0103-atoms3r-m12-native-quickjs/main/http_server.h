#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_server_start(uint16_t port);
esp_err_t http_server_stop(void);
esp_err_t http_server_get_status(bool *running, uint16_t *port);
esp_err_t http_server_add_static_mount(const char *url_prefix, const char *virtual_root);
esp_err_t http_server_clear_static_mounts(void);
void register_http_commands(void);

#ifdef __cplusplus
}
#endif
