#pragma once

#include <cstddef>

namespace papers3_wasm {

struct WasmReplayControlResult {
    bool success;
    std::size_t queued_commands;
    char control_example[32];
    char error_stage[32];
    char error_message[160];
};

WasmReplayControlResult RunWasmReplayControlExample(const char *name);

void PrintWasmReplayControlResult(const WasmReplayControlResult &result);

}  // namespace papers3_wasm
