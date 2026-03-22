#include <inttypes.h>

#include "atoms3r_canvas.h"
#include "backlight.h"
#include "console_repl.h"
#include "display_hal.h"
#include "wasm_host_api.h"
#include "wasm_runtime_service.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

namespace {

constexpr const char *kTag = "0081_app";

void DrawBootBanner()
{
    auto &display = display_get();
    display.fillScreen(TFT_BLACK);
    display.setTextDatum(top_left);
    display.setTextSize(1);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.drawString("AtomS3R", 6, 8);
    display.drawString("WAMR probe", 6, 24);
    display.drawString("USB JTAG console", 6, 40);
    display.drawString("run: wasm examples", 6, 56);
}

}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32, esp_get_free_heap_size(),
             static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_DMA)));

    backlight_prepare_for_init();
    if (!display_init_m5gfx()) {
        ESP_LOGE(kTag, "display init failed");
        return;
    }
    backlight_enable_after_init();
    DrawBootBanner();

    papers3_wasm::InitializePaperCanvas();
    const bool runtime_ready = papers3_wasm::InitWasmRuntime();
    if (runtime_ready) {
        papers3_wasm::InitWasmHostApi();
    }
    papers3_wasm::StartConsoleRepl();
}
