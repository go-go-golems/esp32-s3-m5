#include "console_repl.h"
#include "wasm_host_api.h"
#include "wasm_runtime_service.h"

extern "C" void app_main(void)
{
    const bool runtime_ready = papers3_wasm::InitWasmRuntime();
    if (runtime_ready) {
        papers3_wasm::InitWasmHostApi();
    }
    papers3_wasm::StartConsoleRepl();
}
