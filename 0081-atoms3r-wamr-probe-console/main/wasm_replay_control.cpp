#include "wasm_replay_control.h"

#include "atoms3r_canvas.h"
#include "wasm_host_api.h"

#include <cstdio>
#include <cstring>

namespace papers3_wasm {

namespace {

constexpr int32_t kDisplayWidth = 960;
constexpr int32_t kDisplayHeight = 540;
constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kMidGray = 0x8C8C8C;

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

}  // namespace

WasmReplayControlResult RunWasmReplayControlExample(const char *name)
{
    WasmReplayControlResult result = {};
    std::snprintf(result.control_example, sizeof(result.control_example), "%s", name != nullptr ? name : "");

    if (name == nullptr || name[0] == '\0') {
        SetReplayError(&result, "lookup", "empty control example");
        return result;
    }

    ResetWasmHostFrame();
    PaperCanvasResetFrame();

    bool queued = false;
    if (std::strcmp(name, "hello-frame") == 0) {
        queued = QueueHelloFrameSequence();
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
