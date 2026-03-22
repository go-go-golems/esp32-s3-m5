#pragma once

#include <cstddef>

namespace papers3_wasm {

bool InitWasmHostApi();

bool IsWasmHostApiReady();

void ResetWasmHostFrame();

bool FlushWasmHostFrame(char *error_message, std::size_t error_message_size);

void PrintWasmHostApiStatus();

}  // namespace papers3_wasm
