#pragma once

#include "wasm_module_registry.h"

#include <cstdint>

namespace papers3_wasm {

enum class WasmFlushTiming : uint8_t {
    BeforeCleanup,
    AfterCleanup,
};

enum class WasmLoadMethod : uint8_t {
    RuntimeLoad,
    RuntimeLoadExBinaryFreeable,
};

enum class WasmInvocationMode : uint8_t {
    Execute,
    LoadOnly,
};

struct WasmExecutionResult {
    bool success;
    bool loaded;
    bool instantiated;
    bool export_found;
    bool exec_env_created;
    bool executed;
    int32_t return_value;
    WasmLoadMethod load_method;
    WasmFlushTiming flush_timing;
    WasmInvocationMode invocation_mode;
    char error_stage[32];
    char error_message[160];
};

WasmExecutionResult RunEmbeddedWasmModule(const WasmModuleDescriptor &module, const char *export_name,
                                         WasmFlushTiming flush_timing = WasmFlushTiming::AfterCleanup,
                                         WasmInvocationMode invocation_mode = WasmInvocationMode::Execute,
                                         WasmLoadMethod load_method = WasmLoadMethod::RuntimeLoad);

const char *LoadMethodName(WasmLoadMethod load_method);
void PrintWasmExecutionResult(const WasmModuleDescriptor &module, const WasmExecutionResult &result);
void PrintLastWasmExecutionStatus();

}  // namespace papers3_wasm
