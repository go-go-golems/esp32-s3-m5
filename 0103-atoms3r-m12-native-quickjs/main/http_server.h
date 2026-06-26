#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_server_start(uint16_t port);
esp_err_t http_server_stop(void);
void register_http_commands(void);

#ifdef __cplusplus
}
#endif
