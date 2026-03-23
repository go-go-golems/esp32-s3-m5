#pragma once

#include <cstddef>
#include <cstdint>

namespace papers3_wasm {

struct WasmModuleDescriptor {
    const char *name;
    const char *summary;
    const char *entrypoint;
    const char *source_path;
    const uint8_t *start;
    const uint8_t *end;
};

const WasmModuleDescriptor *GetWasmModules(std::size_t *count);

const WasmModuleDescriptor *FindWasmModule(const char *name);

std::size_t GetWasmModuleBinarySize(const WasmModuleDescriptor &module);

void PrintWasmModuleList();

void PrintWasmModuleInfo(const WasmModuleDescriptor &module);

}  // namespace papers3_wasm
