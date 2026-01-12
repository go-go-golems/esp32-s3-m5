#pragma once

#include <stdbool.h>

#include "exercizer/ControlPlaneTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void exercizer_control_set_default(void *ctrl);
bool exercizer_control_send(const exercizer_ctrl_event_t *ev);

#ifdef __cplusplus
}
#endif
