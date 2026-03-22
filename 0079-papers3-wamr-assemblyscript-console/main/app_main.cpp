#include "console_repl.h"
#include "papers3_canvas.h"
#include "wasm_host_api.h"
#include "wasm_runtime_service.h"

#include <M5Unified.hpp>

namespace {

void InitBoard()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(0x000000, 0xFFFFFF);
    M5.Display.fillScreen(0xFFFFFF);
    M5.Display.drawString("PaperS3 WAMR AssemblyScript Console", 16, 16);
    M5.Display.drawString("Use USB Serial/JTAG and run: help / wasm examples", 16, 40);
}

}  // namespace

extern "C" void app_main(void)
{
    InitBoard();
    papers3_wasm::InitializePaperCanvas();
    const bool runtime_ready = papers3_wasm::InitWasmRuntime();
    if (runtime_ready) {
        papers3_wasm::InitWasmHostApi();
    }
    papers3_wasm::StartConsoleRepl();
}
