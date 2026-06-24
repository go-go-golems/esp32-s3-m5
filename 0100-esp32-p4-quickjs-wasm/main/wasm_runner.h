/* wasm_runner.h — load embedded quickjs.wasm, instantiate once, eval JS. */
#pragma once
#include <stddef.h>

bool wasm_runner_init(void);              // load + instantiate + qjs_init (call once at boot)
int  wasm_runner_eval(const char *src, size_t len);  // returns 0 on success, <0 on error
