#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// monotonic ms since boot, reduced to u32
uint32_t mled_time_local_ms(void);

// signed duration end-start (ms), interpreting wrap-around correctly for small durations
int32_t mled_time_u32_duration(uint32_t start_ms, uint32_t end_ms);

// signed diff a-b
int32_t mled_time_u32_diff(uint32_t a, uint32_t b);

// compute due(now, t) for u32 wrap clock
int mled_time_is_due(uint32_t now_ms, uint32_t execute_at_ms);

#ifdef __cplusplus
}
#endif

