#include "wasm_host_api.h"

#include "papers3_canvas.h"
#include "wasm_runtime_service.h"

#include <inttypes.h>

#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wasm_export.h"

namespace papers3_wasm {

namespace {

constexpr const char *kTag = "0079_host_api";
constexpr const char *kHostModuleName = "host";

struct WasmHostApiStatus {
    bool init_attempted;
    bool ready;
    char last_error[128];
};

WasmHostApiStatus g_host_api_status = {};

void SetLastError(const char *message)
{
    if (message == nullptr) {
        g_host_api_status.last_error[0] = '\0';
        return;
    }

    std::snprintf(g_host_api_status.last_error, sizeof(g_host_api_status.last_error), "%s", message);
}

void HostLogI32(wasm_exec_env_t, int32_t tag, int32_t value)
{
    ESP_LOGI(kTag, "guest_log tag=%" PRId32 " value=%" PRId32, tag, value);
}

void HostDelayMs(wasm_exec_env_t, int32_t ms)
{
    if (ms <= 0) {
        return;
    }

    if (ms > 10000) {
        ms = 10000;
    }

    vTaskDelay(pdMS_TO_TICKS(ms));
}

void HostScreenClear(wasm_exec_env_t, int32_t color)
{
    PaperCanvasScreenClear(static_cast<uint32_t>(color));
}

void HostDrawRect(wasm_exec_env_t, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color)
{
    PaperCanvasDrawRect(x, y, w, h, static_cast<uint32_t>(color));
}

void HostFillRect(wasm_exec_env_t, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color)
{
    PaperCanvasFillRect(x, y, w, h, static_cast<uint32_t>(color));
}

void HostPresent(wasm_exec_env_t, int32_t mode)
{
    PaperCanvasPresent(mode);
}

static NativeSymbol kHostSymbols[] = {
    { "host_log_i32", reinterpret_cast<void *>(HostLogI32), "(ii)", nullptr },
    { "host_delay_ms", reinterpret_cast<void *>(HostDelayMs), "(i)", nullptr },
    { "host_screen_clear", reinterpret_cast<void *>(HostScreenClear), "(i)", nullptr },
    { "host_draw_rect", reinterpret_cast<void *>(HostDrawRect), "(iiiii)", nullptr },
    { "host_fill_rect", reinterpret_cast<void *>(HostFillRect), "(iiiii)", nullptr },
    { "host_present", reinterpret_cast<void *>(HostPresent), "(i)", nullptr },
};

}  // namespace

bool InitWasmHostApi()
{
    if (g_host_api_status.init_attempted) {
        return g_host_api_status.ready;
    }

    g_host_api_status = {};
    g_host_api_status.init_attempted = true;

    const WasmRuntimeStatus &runtime = GetWasmRuntimeStatus();
    if (!runtime.initialized) {
        SetLastError("runtime not initialized");
        return false;
    }

    InitializePaperCanvas();

    if (!wasm_runtime_register_natives(kHostModuleName, kHostSymbols,
                                       sizeof(kHostSymbols) / sizeof(kHostSymbols[0]))) {
        SetLastError("wasm_runtime_register_natives failed");
        ESP_LOGE(kTag, "%s", g_host_api_status.last_error);
        return false;
    }

    g_host_api_status.ready = true;
    SetLastError(nullptr);
    ESP_LOGI(kTag, "Registered %u host symbols for module `%s`",
             static_cast<unsigned>(sizeof(kHostSymbols) / sizeof(kHostSymbols[0])), kHostModuleName);
    return true;
}

bool IsWasmHostApiReady()
{
    return g_host_api_status.ready;
}

void PrintWasmHostApiStatus()
{
    std::printf("host_api=%s\n", g_host_api_status.ready ? "ready" : "not-ready");
    std::printf("host_api.init_attempted=%s\n", g_host_api_status.init_attempted ? "yes" : "no");
    std::printf("host_api.module=%s\n", kHostModuleName);
    std::printf("host_api.symbols=%u\n", static_cast<unsigned>(sizeof(kHostSymbols) / sizeof(kHostSymbols[0])));
    std::printf("host_api.canvas.width=%" PRId32 "\n", PaperCanvasWidth());
    std::printf("host_api.canvas.height=%" PRId32 "\n", PaperCanvasHeight());
    if (g_host_api_status.last_error[0] != '\0') {
        std::printf("host_api.last_error=%s\n", g_host_api_status.last_error);
    }
}

}  // namespace papers3_wasm
