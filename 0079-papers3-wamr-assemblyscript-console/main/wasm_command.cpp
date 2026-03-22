#include "wasm_command.h"

#include <cstdio>
#include <cstring>

#include "wasm_runtime_service.h"

#include "esp_console.h"
#include "esp_err.h"

namespace papers3_wasm {

namespace {

struct ExampleDescriptor {
    const char *name;
    const char *summary;
};

constexpr ExampleDescriptor kExamples[] = {
    {"hello-frame", "first display hello-world frame demo"},
    {"nested-boxes", "nested rectangle composition demo"},
    {"bars", "animated bar columns demo"},
    {"checkerboard", "alternating fill pattern demo"},
    {"radar-sweep", "sweep and arc drawing demo"},
};

const ExampleDescriptor *FindExample(const char *name)
{
    for (const ExampleDescriptor &example : kExamples) {
        if (std::strcmp(example.name, name) == 0) {
            return &example;
        }
    }
    return nullptr;
}

void PrintUsage()
{
    std::printf("wasm commands:\n");
    std::printf("  wasm examples\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info <name>\n");
    std::printf("  wasm run <name>\n");
    std::printf("  wasm status\n");
}

void PrintExamples()
{
    std::printf("wasm examples:\n");
    std::printf("  wasm list\n");
    std::printf("  wasm info hello-frame\n");
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
        for (const ExampleDescriptor &example : kExamples) {
            std::printf("%s\n", example.name);
        }
        return 0;
    }

    if (std::strcmp(argv[1], "info") == 0) {
        if (argc < 3) {
            PrintUsage();
            return 1;
        }
        const ExampleDescriptor *example = FindExample(argv[2]);
        if (example == nullptr) {
            std::printf("unknown example: %s\n", argv[2]);
            return 1;
        }

        const WasmRuntimeStatus &runtime = GetWasmRuntimeStatus();
        std::printf("name=%s\n", example->name);
        std::printf("summary=%s\n", example->summary);
        std::printf("runtime=%s\n", runtime.initialized ? "ready" : "not-ready");
        std::printf("module_status=not-embedded-yet\n");
        return 0;
    }

    if (std::strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            PrintUsage();
            return 1;
        }
        const ExampleDescriptor *example = FindExample(argv[2]);
        if (example == nullptr) {
            std::printf("unknown example: %s\n", argv[2]);
            return 1;
        }

        const WasmRuntimeStatus &runtime = GetWasmRuntimeStatus();
        if (!runtime.initialized) {
            std::printf("runtime is not ready; run `wasm status` for details\n");
            return 1;
        }

        std::printf("placeholder: runtime ready but module embedding not added yet for module=%s\n",
                    example->name);
        return 0;
    }

    if (std::strcmp(argv[1], "status") == 0) {
        PrintWasmRuntimeStatus();
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
