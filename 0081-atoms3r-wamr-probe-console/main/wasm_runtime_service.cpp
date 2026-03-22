#include "wasm_runtime_service.h"

#include <inttypes.h>
#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

namespace papers3_wasm {

namespace {

constexpr const char *kTag = "0081_wamr";
constexpr RunningMode kRequestedRunningMode = Mode_Interp;
constexpr std::size_t kRuntimePoolSizeBytes = 512 * 1024;

WasmRuntimeStatus g_runtime_status = {};
void *g_runtime_pool_buffer = nullptr;

void SetLastError(const char *message)
{
    if (message == nullptr) {
        g_runtime_status.last_error[0] = '\0';
        return;
    }

    std::snprintf(g_runtime_status.last_error, sizeof(g_runtime_status.last_error), "%s", message);
}

void RefreshHeapSnapshot()
{
    g_runtime_status.esp_free_heap_bytes = esp_get_free_heap_size();
    g_runtime_status.esp_min_free_heap_bytes = esp_get_minimum_free_heap_size();

    mem_alloc_info_t mem_info = {};
    g_runtime_status.mem_alloc_info_available = wasm_runtime_get_mem_alloc_info(&mem_info);
    if (!g_runtime_status.mem_alloc_info_available) {
        g_runtime_status.runtime_heap_total_bytes = 0;
        g_runtime_status.runtime_heap_free_bytes = 0;
        g_runtime_status.runtime_heap_highmark_bytes = 0;
        return;
    }

    g_runtime_status.runtime_heap_total_bytes = mem_info.total_size;
    g_runtime_status.runtime_heap_free_bytes = mem_info.total_free_size;
    g_runtime_status.runtime_heap_highmark_bytes = mem_info.highmark_size;
}

}  // namespace

bool InitWasmRuntime()
{
    if (g_runtime_status.init_attempted) {
        RefreshHeapSnapshot();
        return g_runtime_status.initialized;
    }

    g_runtime_status = {};
    g_runtime_status.init_attempted = true;
    g_runtime_status.build_has_interpreter = CONFIG_WAMR_ENABLE_INTERP;
#if defined(CONFIG_WAMR_ENABLE_AOT)
    g_runtime_status.build_has_aot = CONFIG_WAMR_ENABLE_AOT;
#else
    g_runtime_status.build_has_aot = false;
#endif
    g_runtime_status.allocator_type = Alloc_With_Pool;
    g_runtime_status.requested_running_mode = kRequestedRunningMode;

    if (g_runtime_pool_buffer == nullptr) {
        g_runtime_pool_buffer = heap_caps_malloc(kRuntimePoolSizeBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (g_runtime_pool_buffer == nullptr) {
            g_runtime_pool_buffer = heap_caps_malloc(kRuntimePoolSizeBytes, MALLOC_CAP_8BIT);
        }
    }

    if (g_runtime_pool_buffer == nullptr) {
        SetLastError("failed to allocate WAMR pool buffer");
        RefreshHeapSnapshot();
        ESP_LOGE(kTag, "%s", g_runtime_status.last_error);
        return false;
    }

    RuntimeInitArgs init_args = {};
    init_args.mem_alloc_type = g_runtime_status.allocator_type;
    init_args.running_mode = g_runtime_status.requested_running_mode;
    init_args.mem_alloc_option.pool.heap_buf = g_runtime_pool_buffer;
    init_args.mem_alloc_option.pool.heap_size = kRuntimePoolSizeBytes;

    ESP_LOGI(kTag, "Initializing WAMR runtime (allocator=%s, mode=%s)",
             AllocatorTypeName(g_runtime_status.allocator_type),
             RunningModeName(g_runtime_status.requested_running_mode));

    if (!wasm_runtime_full_init(&init_args)) {
        SetLastError("wasm_runtime_full_init failed");
        RefreshHeapSnapshot();
        ESP_LOGE(kTag, "%s", g_runtime_status.last_error);
        return false;
    }

    wasm_runtime_set_log_level(WASM_LOG_LEVEL_WARNING);
    wasm_runtime_get_version(&g_runtime_status.version_major, &g_runtime_status.version_minor,
                             &g_runtime_status.version_patch);
    g_runtime_status.interpreter_supported = wasm_runtime_is_running_mode_supported(Mode_Interp);
    g_runtime_status.aot_supported = g_runtime_status.build_has_aot;
    g_runtime_status.fast_jit_supported = wasm_runtime_is_running_mode_supported(Mode_Fast_JIT);
    g_runtime_status.llvm_jit_supported = wasm_runtime_is_running_mode_supported(Mode_LLVM_JIT);
    g_runtime_status.initialized = true;
    SetLastError(nullptr);
    RefreshHeapSnapshot();

    ESP_LOGI(kTag,
             "WAMR ready (version=%" PRIu32 ".%" PRIu32 ".%" PRIu32
             ", interp=%s, aot=%s, fast-jit=%s, llvm-jit=%s)",
             g_runtime_status.version_major, g_runtime_status.version_minor,
             g_runtime_status.version_patch,
             g_runtime_status.interpreter_supported ? "yes" : "no",
             g_runtime_status.aot_supported ? "yes" : "no",
             g_runtime_status.fast_jit_supported ? "yes" : "no",
             g_runtime_status.llvm_jit_supported ? "yes" : "no");
    return true;
}

const WasmRuntimeStatus &GetWasmRuntimeStatus()
{
    if (g_runtime_status.init_attempted) {
        RefreshHeapSnapshot();
    }
    return g_runtime_status;
}

void PrintWasmRuntimeStatus()
{
    const WasmRuntimeStatus &status = GetWasmRuntimeStatus();

    std::printf("runtime=%s\n", status.initialized ? "ready" : "not-ready");
    std::printf("init_attempted=%s\n", status.init_attempted ? "yes" : "no");
    std::printf("version=%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n", status.version_major, status.version_minor,
                status.version_patch);
    std::printf("requested_mode=%s\n", RunningModeName(status.requested_running_mode));
    std::printf("allocator=%s\n", AllocatorTypeName(status.allocator_type));
    std::printf("build.interpreter=%s\n", status.build_has_interpreter ? "enabled" : "disabled");
    std::printf("build.aot=%s\n", status.build_has_aot ? "enabled" : "disabled");
    std::printf("supported.interpreter=%s\n", status.interpreter_supported ? "yes" : "no");
    std::printf("supported.aot=%s\n", status.aot_supported ? "yes" : "no");
    std::printf("supported.fast_jit=%s\n", status.fast_jit_supported ? "yes" : "no");
    std::printf("supported.llvm_jit=%s\n", status.llvm_jit_supported ? "yes" : "no");
    std::printf("esp.free_heap=%zu\n", status.esp_free_heap_bytes);
    std::printf("esp.min_free_heap=%zu\n", status.esp_min_free_heap_bytes);

    if (status.mem_alloc_info_available) {
        std::printf("wamr.heap_total=%" PRIu32 "\n", status.runtime_heap_total_bytes);
        std::printf("wamr.heap_free=%" PRIu32 "\n", status.runtime_heap_free_bytes);
        std::printf("wamr.heap_highmark=%" PRIu32 "\n", status.runtime_heap_highmark_bytes);
    }
    else {
        std::printf("wamr.heap=unavailable-for-system-allocator\n");
    }

    if (status.last_error[0] != '\0') {
        std::printf("last_error=%s\n", status.last_error);
    }
}

const char *RunningModeName(RunningMode mode)
{
    switch (mode) {
        case Mode_Interp:
            return "interp";
        case Mode_Fast_JIT:
            return "fast-jit";
        case Mode_LLVM_JIT:
            return "llvm-jit";
        case Mode_Multi_Tier_JIT:
            return "multi-tier-jit";
    }

    return "unknown";
}

const char *AllocatorTypeName(mem_alloc_type_t type)
{
    switch (type) {
        case Alloc_With_Pool:
            return "pool";
        case Alloc_With_Allocator:
            return "user-allocator";
        case Alloc_With_System_Allocator:
            return "system-allocator";
    }

    return "unknown";
}

}  // namespace papers3_wasm
