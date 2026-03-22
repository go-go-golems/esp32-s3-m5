#pragma once

#include <cstdint>
#include <cstddef>

namespace papers3_wasm {

bool InitWasmHostApi();

bool IsWasmHostApiReady();

void ResetWasmHostFrame();

bool QueueWasmHostLogI32(int32_t tag, int32_t value);

bool QueueWasmHostDelayMs(int32_t ms);

bool QueueWasmHostScreenClear(uint32_t color);

bool QueueWasmHostDrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

bool QueueWasmHostFillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

bool QueueWasmHostPresent(int32_t mode);

std::size_t GetWasmHostQueuedCommandCount();

bool FlushWasmHostFrame(char *error_message, std::size_t error_message_size);

void PrintWasmHostApiStatus();

}  // namespace papers3_wasm
