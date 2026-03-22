#pragma once

#include "wasm_module_registry.h"

#include <cstdint>

namespace papers3_wasm {

struct WasmExecutionResult {
    bool success;
    bool loaded;
    bool instantiated;
    bool export_found;
    bool exec_env_created;
    bool executed;
    int32_t return_value;
    char error_stage[32];
    char error_message[160];
};

WasmExecutionResult RunEmbeddedWasmModule(const WasmModuleDescriptor &module, const char *export_name);

void PrintWasmExecutionResult(const WasmModuleDescriptor &module, const WasmExecutionResult &result);

}  // namespace papers3_wasm
