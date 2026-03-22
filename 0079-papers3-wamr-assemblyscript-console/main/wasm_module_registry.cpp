#include "wasm_module_registry.h"

#include <cstdio>
#include <cstring>

namespace papers3_wasm {

namespace {

extern const uint8_t return_42_wasm_start[] asm("_binary_return_42_wasm_start");
extern const uint8_t return_42_wasm_end[] asm("_binary_return_42_wasm_end");
extern const uint8_t log_only_wasm_start[] asm("_binary_log_only_wasm_start");
extern const uint8_t log_only_wasm_end[] asm("_binary_log_only_wasm_end");
extern const uint8_t hello_frame_wasm_start[] asm("_binary_hello_frame_wasm_start");
extern const uint8_t hello_frame_wasm_end[] asm("_binary_hello_frame_wasm_end");
extern const uint8_t nested_boxes_wasm_start[] asm("_binary_nested_boxes_wasm_start");
extern const uint8_t nested_boxes_wasm_end[] asm("_binary_nested_boxes_wasm_end");
extern const uint8_t bars_wasm_start[] asm("_binary_bars_wasm_start");
extern const uint8_t bars_wasm_end[] asm("_binary_bars_wasm_end");
extern const uint8_t checkerboard_wasm_start[] asm("_binary_checkerboard_wasm_start");
extern const uint8_t checkerboard_wasm_end[] asm("_binary_checkerboard_wasm_end");
extern const uint8_t radar_sweep_wasm_start[] asm("_binary_radar_sweep_wasm_start");
extern const uint8_t radar_sweep_wasm_end[] asm("_binary_radar_sweep_wasm_end");

const WasmModuleDescriptor kModules[] = {
    {
        .name = "return-42",
        .summary = "minimal wasm probe with no host imports",
        .entrypoint = "run",
        .source_path = "wasm-src/return-42/assembly/index.ts",
        .start = return_42_wasm_start,
        .end = return_42_wasm_end,
    },
    {
        .name = "log-only",
        .summary = "minimal wasm probe with logging import only",
        .entrypoint = "run",
        .source_path = "wasm-src/log-only/assembly/index.ts",
        .start = log_only_wasm_start,
        .end = log_only_wasm_end,
    },
    {
        .name = "hello-frame",
        .summary = "first display hello-world frame demo",
        .entrypoint = "run",
        .source_path = "wasm-src/hello-frame/assembly/index.ts",
        .start = hello_frame_wasm_start,
        .end = hello_frame_wasm_end,
    },
    {
        .name = "nested-boxes",
        .summary = "nested rectangle composition demo",
        .entrypoint = "run",
        .source_path = "wasm-src/nested-boxes/assembly/index.ts",
        .start = nested_boxes_wasm_start,
        .end = nested_boxes_wasm_end,
    },
    {
        .name = "bars",
        .summary = "animated bar columns demo",
        .entrypoint = "run",
        .source_path = "wasm-src/bars/assembly/index.ts",
        .start = bars_wasm_start,
        .end = bars_wasm_end,
    },
    {
        .name = "checkerboard",
        .summary = "alternating fill pattern demo",
        .entrypoint = "run",
        .source_path = "wasm-src/checkerboard/assembly/index.ts",
        .start = checkerboard_wasm_start,
        .end = checkerboard_wasm_end,
    },
    {
        .name = "radar-sweep",
        .summary = "sweep and arc drawing demo",
        .entrypoint = "run",
        .source_path = "wasm-src/radar-sweep/assembly/index.ts",
        .start = radar_sweep_wasm_start,
        .end = radar_sweep_wasm_end,
    },
};

}  // namespace

const WasmModuleDescriptor *GetWasmModules(std::size_t *count)
{
    if (count != nullptr) {
        *count = sizeof(kModules) / sizeof(kModules[0]);
    }
    return kModules;
}

const WasmModuleDescriptor *FindWasmModule(const char *name)
{
    if (name == nullptr) {
        return nullptr;
    }

    for (const WasmModuleDescriptor &module : kModules) {
        if (std::strcmp(module.name, name) == 0) {
            return &module;
        }
    }

    return nullptr;
}

std::size_t GetWasmModuleBinarySize(const WasmModuleDescriptor &module)
{
    return static_cast<std::size_t>(module.end - module.start);
}

void PrintWasmModuleList()
{
    for (const WasmModuleDescriptor &module : kModules) {
        std::printf("%s (%zu bytes)\n", module.name, GetWasmModuleBinarySize(module));
    }
}

void PrintWasmModuleInfo(const WasmModuleDescriptor &module)
{
    std::printf("name=%s\n", module.name);
    std::printf("summary=%s\n", module.summary);
    std::printf("entrypoint=%s\n", module.entrypoint);
    std::printf("source=%s\n", module.source_path);
    std::printf("binary_size=%zu\n", GetWasmModuleBinarySize(module));
    std::printf("module_status=embedded\n");
}

}  // namespace papers3_wasm
