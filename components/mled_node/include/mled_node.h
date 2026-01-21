#pragma once

#include <stdint.h>

#include "mled_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mled_node_on_apply_fn)(uint32_t cue_id, const mled_pattern_config_t *pattern, void *ctx);

typedef struct {
    uint8_t pattern_type;   // protocol pattern_type
    uint8_t brightness_pct; // 1..100
    uint16_t frame_ms;      // optional (default 20)
} mled_node_effect_status_t;

void mled_node_set_on_apply(mled_node_on_apply_fn fn, void *ctx);
void mled_node_set_effect_status(const mled_node_effect_status_t *st);

void mled_node_start(void);
void mled_node_stop(void);

uint32_t mled_node_id(void);

#ifdef __cplusplus
}
#endif

