#pragma once

#include <cstddef>
#include <cstdint>

#include "wasm_export.h"

namespace papers3_wasm {

struct WasmRuntimeStatus {
    bool init_attempted;
    bool initialized;
    bool build_has_interpreter;
    bool build_has_aot;
    bool interpreter_supported;
    bool aot_supported;
    bool fast_jit_supported;
    bool llvm_jit_supported;
    bool mem_alloc_info_available;
    mem_alloc_type_t allocator_type;
    RunningMode requested_running_mode;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint32_t runtime_heap_total_bytes;
    uint32_t runtime_heap_free_bytes;
    uint32_t runtime_heap_highmark_bytes;
    std::size_t esp_free_heap_bytes;
    std::size_t esp_min_free_heap_bytes;
    char last_error[128];
};

bool InitWasmRuntime();

const WasmRuntimeStatus &GetWasmRuntimeStatus();

void PrintWasmRuntimeStatus();

const char *RunningModeName(RunningMode mode);

const char *AllocatorTypeName(mem_alloc_type_t type);

}  // namespace papers3_wasm
