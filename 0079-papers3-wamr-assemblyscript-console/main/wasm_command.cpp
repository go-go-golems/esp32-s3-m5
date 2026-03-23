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

bool ReplayExampleRequiresDisplay(const char *name)
{
    return name == nullptr || std::strcmp(name, "psram-scratch") != 0;
}

void PrintUsage()
{
    std::printf("wasm commands:\n");
    std::printf("  wasm examples\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info <name>\n");
    std::printf("  wasm replay <name>\n");
    std::printf("  wasm run-preflush-worker <name>\n");
    std::printf("  wasm run-preflush <name>\n");
    std::printf("  wasm run-worker <name>\n");
    std::printf("  wasm run <name>\n");
    std::printf("  wasm status\n");
}

void PrintExamples()
{
    std::printf("wasm examples:\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info hello-frame\n");
    std::printf("  wasm replay hello-frame\n");
    std::printf("  wasm replay psram-scratch\n");
    std::printf("  wasm run-preflush-worker hello-frame\n");
    std::printf("  wasm run-preflush hello-frame\n");
    std::printf("  wasm run-worker hello-frame\n");
    std::printf("  wasm run hello-frame\n");
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
        || std::strcmp(argv[1], "run-worker") == 0 || std::strcmp(argv[1], "run-preflush-worker") == 0) {
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

        const WasmFlushTiming flush_timing =
            (std::strcmp(argv[1], "run-preflush") == 0 || std::strcmp(argv[1], "run-preflush-worker") == 0)
                                                 ? WasmFlushTiming::BeforeCleanup
                                                 : WasmFlushTiming::AfterCleanup;
        const WasmExecutionContext execution_context =
            (std::strcmp(argv[1], "run-worker") == 0 || std::strcmp(argv[1], "run-preflush-worker") == 0)
                ? WasmExecutionContext::WorkerThread
                : WasmExecutionContext::Inline;
        const WasmExecutionResult result =
            RunEmbeddedWasmModule(*module, module->entrypoint, flush_timing, execution_context);
        PrintWasmExecutionResult(*module, result);
        return result.success ? 0 : 1;
    }

    if (std::strcmp(argv[1], "replay") == 0) {
        if (argc < 3) {
            PrintUsage();
            return 1;
        }
        if (!IsWasmDisplayHostApiEnabled() && ReplayExampleRequiresDisplay(argv[2])) {
            std::printf("display host API is disabled in this build\n");
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
    cmd.help = "PaperS3 WebAssembly runtime commands (run `wasm examples`)";
    cmd.hint = nullptr;
    cmd.func = CmdWasm;
    cmd.argtable = nullptr;
    cmd.func_w_context = nullptr;
    cmd.context = nullptr;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

}  // namespace papers3_wasm
