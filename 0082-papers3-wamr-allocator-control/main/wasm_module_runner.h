#pragma once

#include "wasm_module_registry.h"

#include <cstdint>

namespace papers3_wasm {

enum class WasmFlushTiming : uint8_t {
    BeforeCleanup,
    AfterCleanup,
};

enum class WasmExecutionContext : uint8_t {
    Inline,
    WorkerThread,
};

enum class WasmInvocationMode : uint8_t {
    Execute,
    LoadOnly,
    LoadOnlyKeepAlive,
    InstantiateBare,
    InstantiateBareKeepAlive,
    InstantiateNoExecEnv,
    InstantiateOnly,
};

struct WasmExecutionResult {
    bool success;
    bool loaded;
    bool instantiated;
    bool export_found;
    bool exec_env_created;
    bool executed;
    int32_t return_value;
    WasmFlushTiming flush_timing;
    WasmExecutionContext execution_context;
    WasmInvocationMode invocation_mode;
    char error_stage[32];
    char error_message[160];
};

WasmExecutionResult RunEmbeddedWasmModule(const WasmModuleDescriptor &module, const char *export_name,
                                         WasmFlushTiming flush_timing = WasmFlushTiming::AfterCleanup,
                                         WasmExecutionContext execution_context = WasmExecutionContext::Inline,
                                         WasmInvocationMode invocation_mode = WasmInvocationMode::Execute);

void PrintWasmExecutionResult(const WasmModuleDescriptor &module, const WasmExecutionResult &result);
void PrintLastWasmExecutionStatus();

}  // namespace papers3_wasm
