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
    std::printf("  wasm load-only <name>\n");
    std::printf("  wasm load-only-copy-internal <name>\n");
    std::printf("  wasm load-only-copy-spiram <name>\n");
    std::printf("  wasm load-only-keepalive <name>\n");
    std::printf("  wasm instantiate-bare <name>\n");
    std::printf("  wasm instantiate-bare-keepalive <name>\n");
    std::printf("  wasm instantiate-no-execenv <name>\n");
    std::printf("  wasm instantiate-only <name>\n");
    std::printf("  wasm run <name>\n");
    std::printf("  wasm status\n");
}

void PrintExamples()
{
    std::printf("wasm examples:\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info return-42\n");
    std::printf("  wasm replay psram-scratch\n");
    std::printf("  wasm replay internal-scratch\n");
    std::printf("  wasm replay psram-persistent-init\n");
    std::printf("  wasm replay psram-persistent-touch\n");
    std::printf("  wasm replay psram-persistent-touch-sync\n");
    std::printf("  wasm replay psram-persistent-free\n");
    std::printf("  wasm load-only return-42\n");
    std::printf("  wasm load-only-copy-internal return-42\n");
    std::printf("  wasm load-only-copy-spiram return-42\n");
    std::printf("  wasm load-only-keepalive return-42\n");
    std::printf("  wasm instantiate-bare return-42\n");
    std::printf("  wasm instantiate-bare-keepalive return-42\n");
    std::printf("  wasm instantiate-no-execenv return-42\n");
    std::printf("  wasm instantiate-only return-42\n");
    std::printf("  wasm run return-42\n");
    std::printf("  wasm run log-only\n");
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

    if (std::strcmp(argv[1], "run") == 0) {
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

        const WasmExecutionResult result =
            RunEmbeddedWasmModule(*module, module->entrypoint, WasmFlushTiming::AfterCleanup,
                                  WasmExecutionContext::Inline);
        PrintWasmExecutionResult(*module, result);
        return result.success ? 0 : 1;
    }

    if (std::strcmp(argv[1], "load-only") == 0 || std::strcmp(argv[1], "load-only-copy-internal") == 0
        || std::strcmp(argv[1], "load-only-copy-spiram") == 0
        || std::strcmp(argv[1], "load-only-keepalive") == 0 || std::strcmp(argv[1], "instantiate-bare") == 0
        || std::strcmp(argv[1], "instantiate-bare-keepalive") == 0 || std::strcmp(argv[1], "instantiate-no-execenv") == 0
        || std::strcmp(argv[1], "instantiate-only") == 0) {
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

        const bool load_only_copy_internal = std::strcmp(argv[1], "load-only-copy-internal") == 0;
        const bool load_only_copy_spiram = std::strcmp(argv[1], "load-only-copy-spiram") == 0;
        const WasmBinarySource binary_source =
            load_only_copy_internal
                ? WasmBinarySource::CopiedToInternalRam
                : (load_only_copy_spiram ? WasmBinarySource::CopiedToSpiram : WasmBinarySource::Embedded);
        const WasmInvocationMode invocation_mode =
            (std::strcmp(argv[1], "load-only") == 0 || load_only_copy_internal || load_only_copy_spiram)
                ? WasmInvocationMode::LoadOnly
                : (std::strcmp(argv[1], "load-only-keepalive") == 0
                       ? WasmInvocationMode::LoadOnlyKeepAlive
                       : (std::strcmp(argv[1], "instantiate-bare") == 0
                              ? WasmInvocationMode::InstantiateBare
                              : (std::strcmp(argv[1], "instantiate-bare-keepalive") == 0
                                     ? WasmInvocationMode::InstantiateBareKeepAlive
                                     : (std::strcmp(argv[1], "instantiate-no-execenv") == 0
                                            ? WasmInvocationMode::InstantiateNoExecEnv
                                            : WasmInvocationMode::InstantiateOnly))));
        const WasmExecutionResult result =
            RunEmbeddedWasmModule(*module, module->entrypoint, WasmFlushTiming::AfterCleanup,
                                  WasmExecutionContext::Inline, invocation_mode, binary_source);
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
    cmd.help = "PaperS3 WAMR allocator-control commands (run `wasm examples`)";
    cmd.hint = nullptr;
    cmd.func = CmdWasm;
    cmd.argtable = nullptr;
    cmd.func_w_context = nullptr;
    cmd.context = nullptr;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

}  // namespace papers3_wasm
