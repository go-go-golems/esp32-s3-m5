#pragma once

#include "mled_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void mled_effect_led_start(void);
void mled_effect_led_apply_pattern(const mled_pattern_config_t *p);

#ifdef __cplusplus
}
#endif

