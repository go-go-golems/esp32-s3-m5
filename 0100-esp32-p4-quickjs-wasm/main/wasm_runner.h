/* wasm_runner.h — own embedded quickjs.wasm on a WAMR pthread and eval JS. */
#pragma once
#include <stddef.h>

bool wasm_runner_init(void);              // starts the owner pthread; load + instantiate + qjs_init there
int  wasm_runner_eval(const char *src, size_t len);  // queues eval on owner pthread; returns 0 on success, <0 on error
