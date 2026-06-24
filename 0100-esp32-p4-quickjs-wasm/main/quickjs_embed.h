/* quickjs_embed.h — symbols for the embedded quickjs.wasm blob (EMBED_FILES). */
#pragma once
#include <stdint.h>
#include <stddef.h>

extern const uint8_t quickjs_wasm_start[] asm("_binary_quickjs_wasm_start");
extern const uint8_t quickjs_wasm_end[]   asm("_binary_quickjs_wasm_end");

static inline const uint8_t *quickjs_wasm_data(void) { return quickjs_wasm_start; }
static inline size_t quickjs_wasm_size(void) { return (size_t)(quickjs_wasm_end - quickjs_wasm_start); }
