#include "wasm_command.h"

#include <cstdio>
#include <cstring>

#include "wasm_host_api.h"
#include "wasm_module_registry.h"
#include "wasm_module_runner.h"
#include "wasm_replay_control.h"
#include "wasm_runtime_service.h"

#include "esp_console.h"
#include "esp_err.h"

namespace papers3_wasm {

namespace {

void PrintUsage()
{
    std::printf("wasm commands:\n");
    std::printf("  wasm examples\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info <name>\n");
    std::printf("  wasm replay <name>\n");
    std::printf("  wasm load-only-embedded-direct <name>\n");
    std::printf("  wasm load-only-embedded-direct-freeable <name>\n");
    std::printf("  wasm run-preflush <name>\n");
    std::printf("  wasm run <name>\n");
    std::printf("  wasm status\n");
}

void PrintExamples()
{
    std::printf("wasm examples:\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info return-42\n");
    std::printf("  wasm replay psram-persistent-init\n");
    std::printf("  wasm replay psram-persistent-touch-sync\n");
    std::printf("  wasm load-only-embedded-direct empty-module\n");
    std::printf("  wasm load-only-embedded-direct return-42\n");
    std::printf("  wasm load-only-embedded-direct-freeable return-42\n");
    std::printf("  wasm run-preflush return-42\n");
    std::printf("  wasm run return-42\n");
    std::printf("  wasm status\n");
}

int CmdWasm(int argc, char **argv)
{
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    if (std::strcmp(argv[1], "examples") == 0) {
        PrintExamples();
        return 0;
    }

    if (std::strcmp(argv[1], "list") == 0) {
        PrintWasmModuleList();
        return 0;
    }

    if (std::strcmp(argv[1], "info") == 0) {
        if (argc < 3) {
            PrintUsage();
            return 1;
        }
        const WasmModuleDescriptor *module = FindWasmModule(argv[2]);
        if (module == nullptr) {
            std::printf("unknown example: %s\n", argv[2]);
            return 1;
        }

        const WasmRuntimeStatus &runtime = GetWasmRuntimeStatus();
        PrintWasmModuleInfo(*module);
        std::printf("runtime=%s\n", runtime.initialized ? "ready" : "not-ready");
        return 0;
    }

    if (std::strcmp(argv[1], "run") == 0 || std::strcmp(argv[1], "run-preflush") == 0
        || std::strcmp(argv[1], "load-only-embedded-direct") == 0
        || std::strcmp(argv[1], "load-only-embedded-direct-freeable") == 0) {
        if (argc < 3) {
            PrintUsage();
            return 1;
        }
        const WasmModuleDescriptor *module = FindWasmModule(argv[2]);
        if (module == nullptr) {
            std::printf("unknown example: %s\n", argv[2]);
            return 1;
        }

        const WasmRuntimeStatus &runtime = GetWasmRuntimeStatus();
        if (!runtime.initialized) {
            std::printf("runtime is not ready; run `wasm status` for details\n");
            return 1;
        }

        const bool is_load_only = std::strcmp(argv[1], "load-only-embedded-direct") == 0
                                  || std::strcmp(argv[1], "load-only-embedded-direct-freeable") == 0;
        const WasmFlushTiming flush_timing = std::strcmp(argv[1], "run-preflush") == 0
                                                 ? WasmFlushTiming::BeforeCleanup
                                                 : WasmFlushTiming::AfterCleanup;
        const WasmInvocationMode invocation_mode =
            is_load_only ? WasmInvocationMode::LoadOnly : WasmInvocationMode::Execute;
        const WasmLoadMethod load_method =
            std::strcmp(argv[1], "load-only-embedded-direct-freeable") == 0
                ? WasmLoadMethod::RuntimeLoadExBinaryFreeable
                : WasmLoadMethod::RuntimeLoad;
        const char *export_name = is_load_only ? module->entrypoint : module->entrypoint;
        const WasmExecutionResult result =
            RunEmbeddedWasmModule(*module, export_name, flush_timing, invocation_mode, load_method);
        PrintWasmExecutionResult(*module, result);
        return result.success ? 0 : 1;
    }

    if (std::strcmp(argv[1], "replay") == 0) {
        if (argc < 3) {
            PrintUsage();
            return 1;
        }

        const WasmReplayControlResult result = RunWasmReplayControlExample(argv[2]);
        PrintWasmReplayControlResult(result);
        return result.success ? 0 : 1;
    }

    if (std::strcmp(argv[1], "status") == 0) {
        PrintWasmRuntimeStatus();
        PrintWasmHostApiStatus();
        PrintLastWasmExecutionStatus();
        return 0;
    }

    PrintUsage();
    return 1;
}

}  // namespace

void RegisterWasmCommands()
{
    esp_console_cmd_t cmd = {};
    cmd.command = "wasm";
    cmd.help = "AtomS3R WebAssembly runtime commands (run `wasm examples`)";
    cmd.hint = nullptr;
    cmd.func = CmdWasm;
    cmd.argtable = nullptr;
    cmd.func_w_context = nullptr;
    cmd.context = nullptr;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

}  // namespace papers3_wasm
