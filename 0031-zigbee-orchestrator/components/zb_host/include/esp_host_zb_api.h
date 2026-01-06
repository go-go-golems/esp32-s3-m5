#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Minimal public surface for the Zigbee host/NCP protocol layer.
//
// The upstream example component keeps these symbols header-less (or private),
// but the 0031 orchestrator needs to invoke specific host protocol requests.
esp_err_t esp_host_zb_output(uint16_t id, const void *buffer, uint16_t len, void *output, uint16_t *outlen);

#ifdef __cplusplus
}
#endif

