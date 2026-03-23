#include "wasm_replay_control.h"

#include "papers3_canvas.h"
#include "wasm_host_api.h"

#include <esp_heap_caps.h>
#include <esp_memory_utils.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace lgfx {
inline namespace v1 {
void debugResetPanelEpdLogBudgets(uint32_t fillrect_budget, uint32_t display_budget);
}
}  // namespace lgfx

namespace papers3_wasm {

namespace {

constexpr int32_t kDisplayWidth = 960;
constexpr int32_t kDisplayHeight = 540;
constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kMidGray = 0x8C8C8C;
constexpr std::size_t kScratchRowStride = (kDisplayWidth + 1) >> 1;
constexpr std::size_t kScratchBufferBytes = (kDisplayWidth * kDisplayHeight) / 2;

void SetReplayError(WasmReplayControlResult *result, const char *stage, const char *message)
{
    if (stage == nullptr) {
        result->error_stage[0] = '\0';
    }
    else {
        std::snprintf(result->error_stage, sizeof(result->error_stage), "%s", stage);
    }

    if (message == nullptr || message[0] == '\0') {
        result->error_message[0] = '\0';
    }
    else {
        std::snprintf(result->error_message, sizeof(result->error_message), "%s", message);
    }
}

bool QueueHelloFrameSequence()
{
    return QueueWasmHostScreenClear(kWhite)
           && QueueWasmHostDrawRect(16, 16, kDisplayWidth - 32, kDisplayHeight - 32, kBlack)
           && QueueWasmHostDrawRect(28, 28, kDisplayWidth - 56, kDisplayHeight - 56, kMidGray)
           && QueueWasmHostFillRect(56, 72, 260, 92, kBlack)
           && QueueWasmHostFillRect(70, 86, 232, 64, kWhite)
           && QueueWasmHostFillRect(kDisplayWidth - 320, kDisplayHeight - 164, 248, 76, kMidGray)
           && QueueWasmHostDrawRect(kDisplayWidth - 320, kDisplayHeight - 164, 248, 76, kBlack)
           && QueueWasmHostPresent(1)
           && QueueWasmHostLogI32(1, 79);
}

bool QueueClearOnlySequence()
{
    return QueueWasmHostScreenClear(kWhite) && QueueWasmHostPresent(1) && QueueWasmHostLogI32(2, 1);
}

bool QueueFrameWithoutClearSequence()
{
    return QueueWasmHostDrawRect(16, 16, kDisplayWidth - 32, kDisplayHeight - 32, kBlack)
           && QueueWasmHostDrawRect(28, 28, kDisplayWidth - 56, kDisplayHeight - 56, kMidGray)
           && QueueWasmHostFillRect(56, 72, 260, 92, kBlack)
           && QueueWasmHostFillRect(70, 86, 232, 64, kWhite)
           && QueueWasmHostFillRect(kDisplayWidth - 320, kDisplayHeight - 164, 248, 76, kMidGray)
           && QueueWasmHostDrawRect(kDisplayWidth - 320, kDisplayHeight - 164, 248, 76, kBlack)
           && QueueWasmHostPresent(1)
           && QueueWasmHostLogI32(2, 2);
}

bool RunPsramScratchProbe(WasmReplayControlResult *result)
{
    auto *buffer = static_cast<uint8_t *>(
        heap_caps_aligned_alloc(16, kScratchBufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer == nullptr) {
        SetReplayError(result, "psram-alloc", "heap_caps_aligned_alloc failed");
        return false;
    }

    std::memset(buffer, 0, kScratchBufferBytes);
    for (int32_t y = 0; y < kDisplayHeight; ++y) {
        auto *row = &buffer[static_cast<std::size_t>(y) * kScratchRowStride];
        for (int32_t x = 0; x < kDisplayWidth; ++x) {
            const std::size_t idx = static_cast<std::size_t>(x) >> 1;
            const uint_fast8_t shift = (x & 1) ? 0 : 4;
            const uint_fast8_t value = 0x0F << shift;
            row[idx] = (row[idx] & (0xF0 >> shift)) | value;
        }
    }

    uint32_t checksum = 0;
    for (std::size_t i = 0; i < kScratchBufferBytes; i += 1024) {
        checksum = (checksum * 131u) ^ buffer[i];
    }
    checksum = (checksum * 131u) ^ buffer[kScratchBufferBytes - 1];

    std::printf("psram_probe.buffer=%p\n", static_cast<void *>(buffer));
    std::printf("psram_probe.external=%s\n", esp_ptr_external_ram(buffer) ? "yes" : "no");
    std::printf("psram_probe.bytes=%u\n", static_cast<unsigned>(kScratchBufferBytes));
    std::printf("psram_probe.checksum=0x%08" PRIx32 "\n", checksum);

    heap_caps_free(buffer);
    return true;
}

}  // namespace

WasmReplayControlResult RunWasmReplayControlExample(const char *name)
{
    WasmReplayControlResult result = {};
    std::snprintf(result.control_example, sizeof(result.control_example), "%s", name != nullptr ? name : "");

    if (name == nullptr || name[0] == '\0') {
        SetReplayError(&result, "lookup", "empty control example");
        return result;
    }

    const bool is_display_control = std::strcmp(name, "psram-scratch") != 0;
    if (is_display_control && !IsWasmDisplayHostApiEnabled()) {
        SetReplayError(&result, "display", "display host API disabled");
        return result;
    }

    ResetWasmHostFrame();
    PaperCanvasResetFrame();
    if (is_display_control) {
        lgfx::v1::debugResetPanelEpdLogBudgets(8, 8);
    }

    bool queued = false;
    if (std::strcmp(name, "hello-frame") == 0) {
        queued = QueueHelloFrameSequence();
    }
    else if (std::strcmp(name, "clear-only") == 0) {
        queued = QueueClearOnlySequence();
    }
    else if (std::strcmp(name, "frame-no-clear") == 0) {
        queued = QueueFrameWithoutClearSequence();
    }
    else if (std::strcmp(name, "psram-scratch") == 0) {
        result.queued_commands = 0;
        if (!RunPsramScratchProbe(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        PaperCanvasResetFrame();
        return result;
    }
    else {
        SetReplayError(&result, "lookup", "unknown replay example");
        return result;
    }

    result.queued_commands = GetWasmHostQueuedCommandCount();
    if (!queued) {
        SetReplayError(&result, "queue", "host command queue overflow");
        ResetWasmHostFrame();
        return result;
    }

    if (!FlushWasmHostFrame(result.error_message, sizeof(result.error_message))) {
        SetReplayError(&result, "flush", result.error_message);
        ResetWasmHostFrame();
        PaperCanvasResetFrame();
        return result;
    }

    result.success = true;
    SetReplayError(&result, nullptr, nullptr);
    ResetWasmHostFrame();
    PaperCanvasResetFrame();
    return result;
}

void PrintWasmReplayControlResult(const WasmReplayControlResult &result)
{
    std::printf("control_example=%s\n", result.control_example);
    std::printf("queued_commands=%u\n", static_cast<unsigned>(result.queued_commands));
    std::printf("control_execution=%s\n", result.success ? "success" : "failure");
    if (result.error_stage[0] != '\0') {
        std::printf("error_stage=%s\n", result.error_stage);
    }
    if (result.error_message[0] != '\0') {
        std::printf("error_message=%s\n", result.error_message);
    }
}

}  // namespace papers3_wasm
