#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void mled_node_start(void);
void mled_node_stop(void);

uint32_t mled_node_id(void);

#ifdef __cplusplus
}
#endif

