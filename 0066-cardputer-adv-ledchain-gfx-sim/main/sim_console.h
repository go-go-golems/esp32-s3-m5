#pragma once

#include "sim_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

void sim_console_register_commands(sim_engine_t *engine);
void sim_console_start(sim_engine_t *engine);

#ifdef __cplusplus
}
#endif
