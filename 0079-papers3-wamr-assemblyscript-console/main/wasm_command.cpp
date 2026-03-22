#include "wasm_command.h"

#include <cstdio>
#include <cstring>

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
        std::printf("hello-frame\n");
        std::printf("nested-boxes\n");
        std::printf("bars\n");
        std::printf("checkerboard\n");
        std::printf("radar-sweep\n");
        return 0;
    }

    if (std::strcmp(argv[1], "info") == 0) {
        if (argc < 3) {
            return 1;
        }
        std::printf("name=%s\n", argv[2]);
        std::printf("status=placeholder\n");
        std::printf("runtime=not-integrated-yet\n");
        return 0;
    }

    if (std::strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            return 1;
        }
        std::printf("placeholder: runtime integration not added yet for module=%s\n", argv[2]);
        return 0;
    }

    if (std::strcmp(argv[1], "status") == 0) {
        std::printf("ok: console=ready runtime=placeholder registry=static-list\n");
        return 0;
    }

    PrintUsage();
    return 1;
}

}  // namespace

void RegisterWasmCommands()
{
    const esp_console_cmd_t cmd = {
        .command = "wasm",
        .help = "PaperS3 WebAssembly runtime commands (run `wasm examples`)",
        .func = CmdWasm,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

}  // namespace papers3_wasm
