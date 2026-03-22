#include "wasm_module_runner.h"

#include "papers3_canvas.h"
#include "wasm_host_api.h"
#include "wasm_runtime_service.h"

#include <cinttypes>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "wasm_export.h"

namespace papers3_wasm {

namespace {

constexpr uint32_t kGuestStackBytes = 16 * 1024;
constexpr uint32_t kGuestHeapBytes = 32 * 1024;

void SetResultError(WasmExecutionResult *result, const char *stage, const char *message)
{
    if (stage == nullptr) {
        result->error_stage[0] = '\0';
    }
    else {
        std::snprintf(result->error_stage, sizeof(result->error_stage), "%s", stage);
    }

    if (message == nullptr || message[0] == '\0') {
        result->error_message[0] = '\0';
    }
    else {
        std::snprintf(result->error_message, sizeof(result->error_message), "%s", message);
    }
}

}  // namespace

WasmExecutionResult RunEmbeddedWasmModule(const WasmModuleDescriptor &module, const char *export_name)
{
    WasmExecutionResult result = {};

    if (!GetWasmRuntimeStatus().initialized) {
        SetResultError(&result, "runtime", "runtime not initialized");
        return result;
    }

    if (!IsWasmHostApiReady()) {
        SetResultError(&result, "host-api", "host API not registered");
        return result;
    }

    if (export_name == nullptr || export_name[0] == '\0') {
        SetResultError(&result, "lookup", "empty export name");
        return result;
    }

    wasm_module_t wasm_module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    wasm_function_inst_t function = nullptr;
    char error_buf[128] = {};
    uint32_t param_count = 0;
    uint32_t result_count = 0;

    PaperCanvasResetFrame();

    wasm_module = wasm_runtime_load(const_cast<uint8_t *>(module.start), static_cast<uint32_t>(GetWasmModuleBinarySize(module)),
                                    error_buf, sizeof(error_buf));
    if (wasm_module == nullptr) {
        SetResultError(&result, "load", error_buf);
        goto cleanup;
    }
    result.loaded = true;

    module_inst = wasm_runtime_instantiate(wasm_module, kGuestStackBytes, kGuestHeapBytes, error_buf, sizeof(error_buf));
    if (module_inst == nullptr) {
        SetResultError(&result, "instantiate", error_buf);
        goto cleanup;
    }
    result.instantiated = true;

    function = wasm_runtime_lookup_function(module_inst, export_name);
    if (function == nullptr) {
        SetResultError(&result, "lookup", "export not found");
        goto cleanup;
    }
    result.export_found = true;

    param_count = wasm_func_get_param_count(function, module_inst);
    result_count = wasm_func_get_result_count(function, module_inst);
    if (param_count != 0 || result_count > 1) {
        SetResultError(&result, "signature", "only zero-argument exports with at most one result are supported");
        goto cleanup;
    }

    exec_env = wasm_runtime_create_exec_env(module_inst, kGuestStackBytes);
    if (exec_env == nullptr) {
        SetResultError(&result, "exec-env", "wasm_runtime_create_exec_env failed");
        goto cleanup;
    }
    result.exec_env_created = true;

    if (result_count == 0) {
        if (!wasm_runtime_call_wasm(exec_env, function, 0, nullptr)) {
            SetResultError(&result, "execute", wasm_runtime_get_exception(module_inst));
            goto cleanup;
        }
    }
    else {
        wasm_val_t call_results[1] = {};
        call_results[0].kind = WASM_I32;
        call_results[0].of.i32 = 0;
        if (!wasm_runtime_call_wasm_a(exec_env, function, 1, call_results, 0, nullptr)) {
            SetResultError(&result, "execute", wasm_runtime_get_exception(module_inst));
            goto cleanup;
        }
        result.return_value = call_results[0].of.i32;
    }

    result.executed = true;
    result.success = true;

cleanup:
    PaperCanvasResetFrame();

    if (exec_env != nullptr) {
        wasm_runtime_destroy_exec_env(exec_env);
    }
    if (module_inst != nullptr) {
        wasm_runtime_deinstantiate(module_inst);
    }
    if (wasm_module != nullptr) {
        wasm_runtime_unload(wasm_module);
    }

    return result;
}

void PrintWasmExecutionResult(const WasmModuleDescriptor &module, const WasmExecutionResult &result)
{
    std::printf("module=%s\n", module.name);
    std::printf("entrypoint=%s\n", module.entrypoint);
    std::printf("execution=%s\n", result.success ? "success" : "failure");
    std::printf("loaded=%s\n", result.loaded ? "yes" : "no");
    std::printf("instantiated=%s\n", result.instantiated ? "yes" : "no");
    std::printf("export_found=%s\n", result.export_found ? "yes" : "no");
    std::printf("exec_env=%s\n", result.exec_env_created ? "yes" : "no");
    std::printf("executed=%s\n", result.executed ? "yes" : "no");
    std::printf("return_value=%" PRId32 "\n", result.return_value);
    if (result.error_stage[0] != '\0') {
        std::printf("error_stage=%s\n", result.error_stage);
    }
    if (result.error_message[0] != '\0') {
        std::printf("error_message=%s\n", result.error_message);
    }
}

}  // namespace papers3_wasm
