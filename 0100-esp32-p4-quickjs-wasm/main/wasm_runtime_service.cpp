/* wasm_runtime_service.cpp — WAMR runtime init with a PSRAM pool.
 * Ported from 0079/main/wasm_runtime_service.cpp and the Phase-0 host_test.c.
 * The pool is large (16 MB) because quickjs.wasm has an 8 MB initial linear
 * memory; PSRAM (32 MB) has the room. */
#include "wasm_runtime_service.h"

#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "wasm_export.h"

namespace {
constexpr const char *kTag = "0100_qjs";
constexpr size_t kRuntimePoolBytes = 16 * 1024 * 1024;  // 16 MB in PSRAM

bool g_ready = false;
void *g_pool = nullptr;
}

bool init_wasm_runtime(void)
{
    if (g_ready) return true;

    if (g_pool == nullptr) {
        g_pool = heap_caps_malloc(kRuntimePoolBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (g_pool == nullptr) {
            g_pool = heap_caps_malloc(kRuntimePoolBytes, MALLOC_CAP_8BIT);  // fallback internal
        }
    }
    if (g_pool == nullptr) {
        ESP_LOGE(kTag, "failed to allocate WAMR pool (%zu bytes)", kRuntimePoolBytes);
        return false;
    }

    RuntimeInitArgs args = {};
    args.mem_alloc_type = Alloc_With_Pool;
    args.running_mode = Mode_Interp;
    args.mem_alloc_option.pool.heap_buf = g_pool;
    args.mem_alloc_option.pool.heap_size = kRuntimePoolBytes;
    // native symbols are registered separately by wasm_host_api (module "env")
    if (!wasm_runtime_full_init(&args)) {
        ESP_LOGE(kTag, "wasm_runtime_full_init failed");
        return false;
    }
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_WARNING);
    g_ready = true;
    ESP_LOGI(kTag, "WAMR ready: pool=%zu bytes, pool_external=%s",
             kRuntimePoolBytes, esp_ptr_external_ram(g_pool) ? "yes" : "no");
    return true;
}

bool wasm_runtime_ready(void) { return g_ready; }

void print_wasm_runtime_status(void)
{
    printf("runtime=%s\n", g_ready ? "ready" : "not-ready");
    printf("pool=%p external=%s size=%zu\n", g_pool,
           g_pool && esp_ptr_external_ram(g_pool) ? "yes" : "no", kRuntimePoolBytes);
    if (g_ready) {
        mem_alloc_info_t info = {};
        if (wasm_runtime_get_mem_alloc_info(&info)) {
            printf("wamr.heap_total=%lu heap_free=%lu highmark=%lu\n",
                   (unsigned long)info.total_size, (unsigned long)info.total_free_size,
                   (unsigned long)info.highmark_size);
        }
    }
}
