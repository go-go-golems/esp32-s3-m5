#pragma once

#include "esp_err.h"

#include "sim_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_server_start(sim_engine_t *engine);
esp_err_t http_server_ws_broadcast_text(const char *text);

#ifdef __cplusplus
}  // extern "C"
#endif
