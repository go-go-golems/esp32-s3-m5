#include "wasm_module_runner.h"

#include "papers3_canvas.h"
#include "wasm_host_api.h"
#include "wasm_runtime_service.h"

#include <cinttypes>
#include <cstddef>
#include <pthread.h>
#include <cstdio>
#include <cstring>

#include "esp_cpu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wasm_export.h"

extern "C" {
extern volatile unsigned port_xSchedulerRunning[portNUM_PROCESSORS];
extern unsigned port_interruptNesting[portNUM_PROCESSORS];
extern BaseType_t port_uxCriticalNesting[portNUM_PROCESSORS];
extern BaseType_t port_uxOldInterruptState[portNUM_PROCESSORS];
}

namespace papers3_wasm {

namespace {

constexpr uint32_t kGuestStackBytes = 16 * 1024;
constexpr uint32_t kGuestHeapBytes = 32 * 1024;
constexpr size_t kWorkerThreadStackBytes = 24 * 1024;

struct WasmLastExecutionStatus {
    bool available;
    char module_name[48];
    WasmExecutionResult result;
};

struct WasmWorkerRunContext {
    const WasmModuleDescriptor *module;
    const char *export_name;
    WasmFlushTiming flush_timing;
    WasmInvocationMode invocation_mode;
    WasmExecutionResult result;
};

WasmLastExecutionStatus g_last_execution_status = {};

const char *FlushTimingName(WasmFlushTiming flush_timing)
{
    switch (flush_timing) {
        case WasmFlushTiming::BeforeCleanup:
            return "before-cleanup";
        case WasmFlushTiming::AfterCleanup:
            return "after-cleanup";
    }

    return "unknown";
}

const char *ExecutionContextName(WasmExecutionContext execution_context)
{
    switch (execution_context) {
        case WasmExecutionContext::Inline:
            return "inline";
        case WasmExecutionContext::WorkerThread:
            return "worker-thread";
    }

    return "unknown";
}

const char *InvocationModeName(WasmInvocationMode invocation_mode)
{
    switch (invocation_mode) {
        case WasmInvocationMode::Execute:
            return "execute";
        case WasmInvocationMode::InstantiateNoExecEnv:
            return "instantiate-no-execenv";
        case WasmInvocationMode::InstantiateOnly:
            return "instantiate-only";
    }

    return "unknown";
}

void PrintExecutionContextSnapshot(const char *stage)
{
    const BaseType_t core_id = xPortGetCoreID();
    uint32_t ps = 0;
    __asm__ volatile("rsr %0, ps" : "=a"(ps));

    std::printf(
        "exec_probe.stage=%s\n"
        "exec_probe.core=%" PRId32 "\n"
        "exec_probe.task=%p\n"
        "exec_probe.cycles=%" PRIu32 "\n"
        "exec_probe.ps=0x%08" PRIx32 "\n"
        "exec_probe.ps.intlevel=%" PRIu32 "\n"
        "exec_probe.in_isr=%s\n"
        "exec_probe.scheduler=%u\n"
        "exec_probe.interrupt_nesting=%u\n"
        "exec_probe.critical_nesting=%" PRId32 "\n"
        "exec_probe.old_interrupt_state=%" PRId32 "\n"
        "exec_probe.stack_hwm=%" PRIu32 "\n",
        stage != nullptr ? stage : "unknown",
        static_cast<int32_t>(core_id),
        xTaskGetCurrentTaskHandle(),
        static_cast<uint32_t>(esp_cpu_get_cycle_count()),
        ps,
        ps & XCHAL_PS_INTLEVEL_MASK,
        xPortInIsrContext() ? "yes" : "no",
        port_xSchedulerRunning[core_id],
        port_interruptNesting[core_id],
        static_cast<int32_t>(port_uxCriticalNesting[core_id]),
        static_cast<int32_t>(port_uxOldInterruptState[core_id]),
        static_cast<uint32_t>(uxTaskGetStackHighWaterMark(nullptr)));
}

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

void RememberLastExecutionResult(const WasmModuleDescriptor &module, const WasmExecutionResult &result)
{
    g_last_execution_status = {};
    g_last_execution_status.available = true;
    std::snprintf(g_last_execution_status.module_name, sizeof(g_last_execution_status.module_name), "%s",
                  module.name);
    g_last_execution_status.result = result;
}

WasmExecutionResult RunEmbeddedWasmModuleOnCurrentThread(const WasmModuleDescriptor &module, const char *export_name,
                                                         WasmFlushTiming flush_timing,
                                                         WasmInvocationMode invocation_mode)
{
    WasmExecutionResult result = {};
    result.flush_timing = flush_timing;
    result.invocation_mode = invocation_mode;

    wasm_module_t wasm_module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    wasm_function_inst_t function = nullptr;
    char error_buf[128] = {};
    uint32_t param_count = 0;
    uint32_t result_count = 0;
    bool flush_host_frame = false;

    ResetWasmHostFrame();
    PaperCanvasResetFrame();

    wasm_module = wasm_runtime_load(const_cast<uint8_t *>(module.start),
                                    static_cast<uint32_t>(GetWasmModuleBinarySize(module)), error_buf,
                                    sizeof(error_buf));
    if (wasm_module == nullptr) {
        SetResultError(&result, "load", error_buf);
        goto cleanup;
    }
    result.loaded = true;

    module_inst =
        wasm_runtime_instantiate(wasm_module, kGuestStackBytes, kGuestHeapBytes, error_buf, sizeof(error_buf));
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

    if (invocation_mode == WasmInvocationMode::InstantiateNoExecEnv) {
        result.success = true;
        goto cleanup;
    }

    exec_env = wasm_runtime_create_exec_env(module_inst, kGuestStackBytes);
    if (exec_env == nullptr) {
        SetResultError(&result, "exec-env", "wasm_runtime_create_exec_env failed");
        goto cleanup;
    }
    result.exec_env_created = true;

    if (invocation_mode == WasmInvocationMode::InstantiateOnly) {
        result.success = true;
        goto cleanup;
    }

    PrintExecutionContextSnapshot("before-call");
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
    PrintExecutionContextSnapshot("after-call");

    result.executed = true;
    flush_host_frame = true;

    if (flush_host_frame && flush_timing == WasmFlushTiming::BeforeCleanup) {
        PrintExecutionContextSnapshot("before-preflush");
        if (!FlushWasmHostFrame(result.error_message, sizeof(result.error_message))) {
            SetResultError(&result, "render", result.error_message);
            goto cleanup;
        }

        flush_host_frame = false;
        result.success = true;
    }

cleanup:
    if (exec_env != nullptr) {
        wasm_runtime_destroy_exec_env(exec_env);
    }
    if (module_inst != nullptr) {
        wasm_runtime_deinstantiate(module_inst);
    }
    if (wasm_module != nullptr) {
        wasm_runtime_unload(wasm_module);
    }

    if (flush_host_frame) {
        PrintExecutionContextSnapshot("before-postcleanup-flush");
        if (!FlushWasmHostFrame(result.error_message, sizeof(result.error_message))) {
            SetResultError(&result, "render", result.error_message);
        }
        else {
            result.success = true;
        }
    }

    ResetWasmHostFrame();
    PaperCanvasResetFrame();
    return result;
}

void *RunEmbeddedWasmModuleWorkerEntry(void *arg)
{
    auto *context = static_cast<WasmWorkerRunContext *>(arg);
    if (context == nullptr || context->module == nullptr || context->export_name == nullptr) {
        return nullptr;
    }

    context->result = {};
    context->result.flush_timing = context->flush_timing;
    context->result.execution_context = WasmExecutionContext::WorkerThread;
    context->result.invocation_mode = context->invocation_mode;

    if (!wasm_runtime_init_thread_env()) {
        SetResultError(&context->result, "thread-env", "wasm_runtime_init_thread_env failed");
        return nullptr;
    }

    context->result =
        RunEmbeddedWasmModuleOnCurrentThread(*context->module, context->export_name, context->flush_timing,
                                            context->invocation_mode);
    context->result.execution_context = WasmExecutionContext::WorkerThread;
    wasm_runtime_destroy_thread_env();
    return nullptr;
}

WasmExecutionResult RunEmbeddedWasmModuleOnWorkerThread(const WasmModuleDescriptor &module, const char *export_name,
                                                        WasmFlushTiming flush_timing,
                                                        WasmInvocationMode invocation_mode)
{
    WasmWorkerRunContext worker = {};
    worker.module = &module;
    worker.export_name = export_name;
    worker.flush_timing = flush_timing;
    worker.invocation_mode = invocation_mode;
    worker.result.flush_timing = flush_timing;
    worker.result.execution_context = WasmExecutionContext::WorkerThread;
    worker.result.invocation_mode = invocation_mode;

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        SetResultError(&worker.result, "thread-attr", "pthread_attr_init failed");
        return worker.result;
    }

    const int stack_error = pthread_attr_setstacksize(&attr, kWorkerThreadStackBytes);
    if (stack_error != 0) {
        pthread_attr_destroy(&attr);
        SetResultError(&worker.result, "thread-attr", std::strerror(stack_error));
        return worker.result;
    }

    pthread_t thread = {};
    const int create_error = pthread_create(&thread, &attr, RunEmbeddedWasmModuleWorkerEntry, &worker);
    pthread_attr_destroy(&attr);
    if (create_error != 0) {
        SetResultError(&worker.result, "thread-create", std::strerror(create_error));
        return worker.result;
    }

    const int join_error = pthread_join(thread, nullptr);
    if (join_error != 0) {
        SetResultError(&worker.result, "thread-join", std::strerror(join_error));
    }

    return worker.result;
}

}  // namespace

WasmExecutionResult RunEmbeddedWasmModule(const WasmModuleDescriptor &module, const char *export_name,
                                          WasmFlushTiming flush_timing, WasmExecutionContext execution_context,
                                          WasmInvocationMode invocation_mode)
{
    WasmExecutionResult result = {};
    result.flush_timing = flush_timing;
    result.execution_context = execution_context;
    result.invocation_mode = invocation_mode;

    if (!GetWasmRuntimeStatus().initialized) {
        SetResultError(&result, "runtime", "runtime not initialized");
        RememberLastExecutionResult(module, result);
        return result;
    }

    if (!IsWasmHostApiReady()) {
        SetResultError(&result, "host-api", "host API not registered");
        RememberLastExecutionResult(module, result);
        return result;
    }

    if (export_name == nullptr || export_name[0] == '\0') {
        SetResultError(&result, "lookup", "empty export name");
        RememberLastExecutionResult(module, result);
        return result;
    }

    if (execution_context == WasmExecutionContext::WorkerThread) {
        result = RunEmbeddedWasmModuleOnWorkerThread(module, export_name, flush_timing, invocation_mode);
    }
    else {
        result = RunEmbeddedWasmModuleOnCurrentThread(module, export_name, flush_timing, invocation_mode);
        result.execution_context = WasmExecutionContext::Inline;
    }

    RememberLastExecutionResult(module, result);
    return result;
}

void PrintWasmExecutionResult(const WasmModuleDescriptor &module, const WasmExecutionResult &result)
{
    std::printf("module=%s\n", module.name);
    std::printf("entrypoint=%s\n", module.entrypoint);
    std::printf("flush_timing=%s\n", FlushTimingName(result.flush_timing));
    std::printf("execution_context=%s\n", ExecutionContextName(result.execution_context));
    std::printf("invocation_mode=%s\n", InvocationModeName(result.invocation_mode));
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

void PrintLastWasmExecutionStatus()
{
    if (!g_last_execution_status.available) {
        std::printf("last_run=none\n");
        return;
    }

    std::printf("last_run=available\n");
    std::printf("last_run.module=%s\n", g_last_execution_status.module_name);
    std::printf("last_run.success=%s\n", g_last_execution_status.result.success ? "yes" : "no");
    std::printf("last_run.error_stage=%s\n",
                g_last_execution_status.result.error_stage[0] != '\0'
                    ? g_last_execution_status.result.error_stage
                    : "none");
    std::printf("last_run.error_message=%s\n",
                g_last_execution_status.result.error_message[0] != '\0'
                    ? g_last_execution_status.result.error_message
                    : "none");
}

}  // namespace papers3_wasm
