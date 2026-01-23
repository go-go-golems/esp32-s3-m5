#pragma once

#include "esp_err.h"

#include "sim_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_server_start(sim_engine_t *engine);

#ifdef __cplusplus
}  // extern "C"
#endif

