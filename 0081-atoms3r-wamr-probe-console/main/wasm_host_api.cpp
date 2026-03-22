#include "wasm_host_api.h"

#include "atoms3r_canvas.h"
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

constexpr const char *kTag = "0081_host_api";
constexpr const char *kHostModuleName = "host";
constexpr std::size_t kMaxQueuedCommands = 256;

struct WasmHostApiStatus {
    bool init_attempted;
    bool ready;
    char last_error[128];
    std::size_t queued_commands;
    std::size_t last_flushed_commands;
    bool command_queue_overflowed;
};

enum class HostCommandType : uint8_t {
    LogI32,
    DelayMs,
    ScreenClear,
    DrawRect,
    FillRect,
    Present,
};

struct HostCommand {
    HostCommandType type;
    int32_t tag;
    int32_t value;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    uint32_t color;
    int32_t mode;
};

WasmHostApiStatus g_host_api_status = {};
HostCommand g_host_commands[kMaxQueuedCommands] = {};

void SetLastError(const char *message)
{
    if (message == nullptr) {
        g_host_api_status.last_error[0] = '\0';
        return;
    }

    std::snprintf(g_host_api_status.last_error, sizeof(g_host_api_status.last_error), "%s", message);
}

bool QueueHostCommand(const HostCommand &command)
{
    if (g_host_api_status.queued_commands >= kMaxQueuedCommands) {
        g_host_api_status.command_queue_overflowed = true;
        return false;
    }

    g_host_commands[g_host_api_status.queued_commands++] = command;
    return true;
}

void HostLogI32(wasm_exec_env_t, int32_t tag, int32_t value)
{
    QueueWasmHostLogI32(tag, value);
}

void HostDelayMs(wasm_exec_env_t, int32_t ms)
{
    QueueWasmHostDelayMs(ms);
}

void HostScreenClear(wasm_exec_env_t, int32_t color)
{
    QueueWasmHostScreenClear(static_cast<uint32_t>(color));
}

void HostDrawRect(wasm_exec_env_t, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color)
{
    QueueWasmHostDrawRect(x, y, w, h, static_cast<uint32_t>(color));
}

void HostFillRect(wasm_exec_env_t, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color)
{
    QueueWasmHostFillRect(x, y, w, h, static_cast<uint32_t>(color));
}

void HostPresent(wasm_exec_env_t, int32_t mode)
{
    QueueWasmHostPresent(mode);
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

void ResetWasmHostFrame()
{
    g_host_api_status.queued_commands = 0;
    g_host_api_status.command_queue_overflowed = false;
}

bool QueueWasmHostLogI32(int32_t tag, int32_t value)
{
    return QueueHostCommand({ HostCommandType::LogI32, tag, value, 0, 0, 0, 0, 0, 0 });
}

bool QueueWasmHostDelayMs(int32_t ms)
{
    return QueueHostCommand({ HostCommandType::DelayMs, 0, ms, 0, 0, 0, 0, 0, 0 });
}

bool QueueWasmHostScreenClear(uint32_t color)
{
    return QueueHostCommand({ HostCommandType::ScreenClear, 0, 0, 0, 0, 0, 0, color, 0 });
}

bool QueueWasmHostDrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    return QueueHostCommand({ HostCommandType::DrawRect, 0, 0, x, y, w, h, color, 0 });
}

bool QueueWasmHostFillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    return QueueHostCommand({ HostCommandType::FillRect, 0, 0, x, y, w, h, color, 0 });
}

bool QueueWasmHostPresent(int32_t mode)
{
    return QueueHostCommand({ HostCommandType::Present, 0, 0, 0, 0, 0, 0, 0, mode });
}

std::size_t GetWasmHostQueuedCommandCount()
{
    return g_host_api_status.queued_commands;
}

bool FlushWasmHostFrame(char *error_message, std::size_t error_message_size)
{
    if (error_message != nullptr && error_message_size > 0) {
        error_message[0] = '\0';
    }

    if (g_host_api_status.command_queue_overflowed) {
        if (error_message != nullptr && error_message_size > 0) {
            std::snprintf(error_message, error_message_size, "host command queue overflow");
        }
        return false;
    }

    for (std::size_t i = 0; i < g_host_api_status.queued_commands; ++i) {
        const HostCommand &command = g_host_commands[i];
        switch (command.type) {
            case HostCommandType::LogI32:
                ESP_LOGI(kTag, "guest_log tag=%" PRId32 " value=%" PRId32, command.tag, command.value);
                break;
            case HostCommandType::DelayMs:
                if (command.value > 0) {
                    const int32_t clamped_ms = command.value > 10000 ? 10000 : command.value;
                    vTaskDelay(pdMS_TO_TICKS(clamped_ms));
                }
                break;
            case HostCommandType::ScreenClear:
                PaperCanvasScreenClear(command.color);
                break;
            case HostCommandType::DrawRect:
                PaperCanvasDrawRect(command.x, command.y, command.w, command.h, command.color);
                break;
            case HostCommandType::FillRect:
                PaperCanvasFillRect(command.x, command.y, command.w, command.h, command.color);
                break;
            case HostCommandType::Present:
                PaperCanvasPresent(command.mode);
                break;
        }
    }

    g_host_api_status.last_flushed_commands = g_host_api_status.queued_commands;
    return true;
}

void PrintWasmHostApiStatus()
{
    std::printf("host_api=%s\n", g_host_api_status.ready ? "ready" : "not-ready");
    std::printf("host_api.init_attempted=%s\n", g_host_api_status.init_attempted ? "yes" : "no");
    std::printf("host_api.module=%s\n", kHostModuleName);
    std::printf("host_api.symbols=%u\n", static_cast<unsigned>(sizeof(kHostSymbols) / sizeof(kHostSymbols[0])));
    std::printf("host_api.canvas.width=%" PRId32 "\n", PaperCanvasWidth());
    std::printf("host_api.canvas.height=%" PRId32 "\n", PaperCanvasHeight());
    std::printf("host_api.command_queue.capacity=%u\n", static_cast<unsigned>(kMaxQueuedCommands));
    std::printf("host_api.command_queue.queued=%u\n", static_cast<unsigned>(g_host_api_status.queued_commands));
    std::printf("host_api.command_queue.last_flush=%u\n",
                static_cast<unsigned>(g_host_api_status.last_flushed_commands));
    std::printf("host_api.command_queue.overflow=%s\n", g_host_api_status.command_queue_overflowed ? "yes" : "no");
    if (g_host_api_status.last_error[0] != '\0') {
        std::printf("host_api.last_error=%s\n", g_host_api_status.last_error);
    }
}

}  // namespace papers3_wasm
