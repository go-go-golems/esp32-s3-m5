#pragma once

#include <stdint.h>

#include "driver/rmt_encoder.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t resolution_hz;
    uint32_t t0h_ns;
    uint32_t t0l_ns;
    uint32_t t1h_ns;
    uint32_t t1l_ns;
    uint32_t reset_us;
} ws281x_encoder_config_t;

esp_err_t rmt_new_ws281x_encoder(const ws281x_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif

