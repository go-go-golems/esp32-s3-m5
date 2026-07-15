// Native side of the spike's generation-safe widget handles (task dygk).
// The suite (app_main) inspects these counters to verify finalization.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SpikeWidgetsReset(void);
uint32_t SpikeWidgetsLive(void);
uint32_t SpikeWidgetsCreated(void);
uint32_t SpikeWidgetsFinalized(void);
uint32_t SpikeWidgetsStaleHits(void);

#ifdef __cplusplus
}
#endif
