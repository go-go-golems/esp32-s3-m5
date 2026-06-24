/* wasm_runner.cpp — load the embedded quickjs.wasm, instantiate once at boot,
 * call qjs_init, and evaluate JavaScript on demand. Mirrors the Phase-0
 * host_test.c flow (proven). */
#include "wasm_runner.h"

#include "quickjs_embed.h"

#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "wasm_export.h"

namespace {
constexpr const char *kTag = "0100_run";
constexpr uint32_t kGuestStack = 32 * 1024;
constexpr uint32_t kGuestHeap  = 512 * 1024;

wasm_module_t      g_mod  = nullptr;
wasm_module_inst_t g_inst = nullptr;
wasm_exec_env_t    g_env  = nullptr;
}  // namespace

bool wasm_runner_init(void)
{
    char err[256] = {0};

    g_mod = wasm_runtime_load((uint8_t *)quickjs_wasm_data(), quickjs_wasm_size(),
                              err, sizeof(err));
    if (!g_mod) {
        ESP_LOGE(kTag, "wasm_runtime_load failed: %s", err);
        return false;
    }

    g_inst = wasm_runtime_instantiate(g_mod, kGuestStack, kGuestHeap, err, sizeof(err));
    if (!g_inst) {
        ESP_LOGE(kTag, "wasm_runtime_instantiate failed: %s", err);
        wasm_runtime_unload(g_mod);
        g_mod = nullptr;
        return false;
    }

    g_env = wasm_runtime_create_exec_env(g_inst, kGuestStack);
    if (!g_env) {
        ESP_LOGE(kTag, "wasm_runtime_create_exec_env failed");
        return false;
    }

    wasm_function_inst_t finit = wasm_runtime_lookup_function(g_inst, "qjs_init");
    if (!finit) {
        ESP_LOGE(kTag, "qjs_init export not found");
        return false;
    }
    if (!wasm_runtime_call_wasm(g_env, finit, 0, nullptr)) {
        ESP_LOGE(kTag, "qjs_init call failed: %s", wasm_runtime_get_exception(g_inst));
        return false;
    }

    ESP_LOGI(kTag, "QuickJS ready (qjs_init ok)");
    return true;
}

int wasm_runner_eval(const char *src, size_t len)
{
    if (!g_inst) {
        printf("wasm session not initialized\n");
        return -10;
    }
    wasm_function_inst_t feval = wasm_runtime_lookup_function(g_inst, "qjs_eval");
    if (!feval) {
        printf("qjs_eval export not found\n");
        return -11;
    }

    /* Copy the JS source into the guest's linear memory (+1 for NUL so the
       guest can echo it if needed). Pass the guest-space address + length. */
    uint64_t wptr = wasm_runtime_module_dup_data(g_inst, src, len + 1);
    if (!wptr) {
        printf("wasm_runtime_module_dup_data failed\n");
        return -12;
    }

    uint32_t argv[2] = { (uint32_t)wptr, (uint32_t)len };
    bool ok = wasm_runtime_call_wasm(g_env, feval, 2, argv);
    wasm_runtime_module_free(g_inst, wptr);

    if (!ok) {
        printf("eval exception: %s\n", wasm_runtime_get_exception(g_inst));
        return -1;
    }
    return (int)argv[0];  // qjs_eval returns 0 on success, -1 on JS exception
}
