/* wasm_runtime_service.h — WAMR runtime init + status (pool in PSRAM). */
#pragma once
#include <stdbool.h>
#include <stddef.h>

bool init_wasm_runtime(void);
bool wasm_runtime_ready(void);

void print_wasm_runtime_status(void);
