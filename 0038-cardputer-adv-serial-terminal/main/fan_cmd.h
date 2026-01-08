#pragma once

#include "fan_control.h"

typedef void (*fan_cmd_out_fn)(void *ctx, const char *line);

int fan_cmd_exec(fan_control_t *fan, int argc, const char *const *argv, fan_cmd_out_fn out, void *ctx);

