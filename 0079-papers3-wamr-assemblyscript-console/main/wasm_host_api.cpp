#include "wasm_host_api.h"

#include "papers3_canvas.h"
#include "wasm_runtime_service.h"

#include "sdkconfig.h"

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
constexpr std::size_t kMaxQueuedCommands = 256;
#if CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
constexpr bool kDisplayHostEnabled = true;
#else
constexpr bool kDisplayHostEnabled = false;
#endif

struct WasmHostApiStatus {
    bool init_attempted;
    bool ready;
    bool display_host_enabled;
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

#if CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
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
#endif

static NativeSymbol kHostSymbols[] = {
    { "host_log_i32", reinterpret_cast<void *>(HostLogI32), "(ii)", nullptr },
    { "host_delay_ms", reinterpret_cast<void *>(HostDelayMs), "(i)", nullptr },
#if CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
    { "host_screen_clear", reinterpret_cast<void *>(HostScreenClear), "(i)", nullptr },
    { "host_draw_rect", reinterpret_cast<void *>(HostDrawRect), "(iiiii)", nullptr },
    { "host_fill_rect", reinterpret_cast<void *>(HostFillRect), "(iiiii)", nullptr },
    { "host_present", reinterpret_cast<void *>(HostPresent), "(i)", nullptr },
#endif
};

}  // namespace

bool InitWasmHostApi()
{
    if (g_host_api_status.init_attempted) {
        return g_host_api_status.ready;
    }

    g_host_api_status = {};
    g_host_api_status.init_attempted = true;
    g_host_api_status.display_host_enabled = kDisplayHostEnabled;

    const WasmRuntimeStatus &runtime = GetWasmRuntimeStatus();
    if (!runtime.initialized) {
        SetLastError("runtime not initialized");
        return false;
    }

#if CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
    InitializePaperCanvas();
#endif

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

bool IsWasmDisplayHostApiEnabled()
{
    return kDisplayHostEnabled;
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
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
    static_cast<void>(color);
    return false;
#else
    return QueueHostCommand({ HostCommandType::ScreenClear, 0, 0, 0, 0, 0, 0, color, 0 });
#endif
}

bool QueueWasmHostDrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
    static_cast<void>(x);
    static_cast<void>(y);
    static_cast<void>(w);
    static_cast<void>(h);
    static_cast<void>(color);
    return false;
#else
    return QueueHostCommand({ HostCommandType::DrawRect, 0, 0, x, y, w, h, color, 0 });
#endif
}

bool QueueWasmHostFillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
    static_cast<void>(x);
    static_cast<void>(y);
    static_cast<void>(w);
    static_cast<void>(h);
    static_cast<void>(color);
    return false;
#else
    return QueueHostCommand({ HostCommandType::FillRect, 0, 0, x, y, w, h, color, 0 });
#endif
}

bool QueueWasmHostPresent(int32_t mode)
{
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
    static_cast<void>(mode);
    return false;
#else
    return QueueHostCommand({ HostCommandType::Present, 0, 0, 0, 0, 0, 0, 0, mode });
#endif
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
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
                std::snprintf(error_message, error_message_size, "display host API disabled");
                return false;
#else
                PaperCanvasScreenClear(command.color);
                break;
#endif
            case HostCommandType::DrawRect:
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
                std::snprintf(error_message, error_message_size, "display host API disabled");
                return false;
#else
                PaperCanvasDrawRect(command.x, command.y, command.w, command.h, command.color);
                break;
#endif
            case HostCommandType::FillRect:
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
                std::snprintf(error_message, error_message_size, "display host API disabled");
                return false;
#else
                PaperCanvasFillRect(command.x, command.y, command.w, command.h, command.color);
                break;
#endif
            case HostCommandType::Present:
#if !CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK
                std::snprintf(error_message, error_message_size, "display host API disabled");
                return false;
#else
                PaperCanvasPresent(command.mode);
                break;
#endif
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
    std::printf("host_api.display=%s\n", g_host_api_status.display_host_enabled ? "enabled" : "disabled");
    std::printf("host_api.symbols=%u\n", static_cast<unsigned>(sizeof(kHostSymbols) / sizeof(kHostSymbols[0])));
    if (g_host_api_status.display_host_enabled) {
        std::printf("host_api.canvas.width=%" PRId32 "\n", PaperCanvasWidth());
        std::printf("host_api.canvas.height=%" PRId32 "\n", PaperCanvasHeight());
    }
    else {
        std::printf("host_api.canvas.width=disabled\n");
        std::printf("host_api.canvas.height=disabled\n");
    }
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
