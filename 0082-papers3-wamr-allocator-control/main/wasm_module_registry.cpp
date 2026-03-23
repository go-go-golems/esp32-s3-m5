#include "wasm_module_registry.h"

#include <cstdio>
#include <cstring>

namespace papers3_wasm {

namespace {

extern const uint8_t return_42_wasm_start[] asm("_binary_return_42_wasm_start");
extern const uint8_t return_42_wasm_end[] asm("_binary_return_42_wasm_end");
extern const uint8_t log_only_wasm_start[] asm("_binary_log_only_wasm_start");
extern const uint8_t log_only_wasm_end[] asm("_binary_log_only_wasm_end");

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
