#include "wasm_replay_control.h"

#include "atoms3r_canvas.h"
#include "wasm_host_api.h"

#include <esp_cache.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <esp_private/esp_cache_private.h>
#include <esp_private/cache_utils.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace papers3_wasm {

namespace {

constexpr int32_t kDisplayWidth = 960;
constexpr int32_t kDisplayHeight = 540;
constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kMidGray = 0x8C8C8C;
constexpr std::size_t kScratchBufferBytes = (960 * 540) / 2;

uint8_t *g_persistent_psram_probe = nullptr;
std::size_t g_persistent_psram_probe_bytes = 0;

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

bool InitPersistentPsramProbe(WasmReplayControlResult *result)
{
    if (g_persistent_psram_probe != nullptr) {
        heap_caps_free(g_persistent_psram_probe);
        g_persistent_psram_probe = nullptr;
        g_persistent_psram_probe_bytes = 0;
    }

    g_persistent_psram_probe = static_cast<uint8_t *>(
        heap_caps_aligned_alloc(16, kScratchBufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_persistent_psram_probe == nullptr) {
        SetReplayError(result, "persistent-alloc", "heap_caps_aligned_alloc failed");
        return false;
    }

    g_persistent_psram_probe_bytes = kScratchBufferBytes;
    std::memset(g_persistent_psram_probe, 0x11, g_persistent_psram_probe_bytes);

    std::printf("persistent_psram_probe.buffer=%p\n", static_cast<void *>(g_persistent_psram_probe));
    std::printf("persistent_psram_probe.external=%s\n",
                esp_ptr_external_ram(g_persistent_psram_probe) ? "yes" : "no");
    std::printf("persistent_psram_probe.bytes=%u\n", static_cast<unsigned>(g_persistent_psram_probe_bytes));
    return true;
}

bool SyncPersistentPsramProbe(WasmReplayControlResult *result, const char *stage)
{
    if (g_persistent_psram_probe == nullptr || g_persistent_psram_probe_bytes != kScratchBufferBytes) {
        SetReplayError(result, stage, "persistent PSRAM probe is not initialized");
        return false;
    }

    size_t cache_alignment = 0;
    const esp_err_t align_err = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &cache_alignment);
    std::printf("persistent_psram_probe.cache_alignment_err=%s\n", esp_err_to_name(align_err));
    if (align_err == ESP_OK) {
        std::printf("persistent_psram_probe.cache_alignment=%u\n", static_cast<unsigned>(cache_alignment));
    }

    const esp_err_t sync_err =
        esp_cache_msync(g_persistent_psram_probe, g_persistent_psram_probe_bytes,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE
                            | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    std::printf("persistent_psram_probe.sync_err=%s\n", esp_err_to_name(sync_err));
    if (sync_err != ESP_OK) {
        SetReplayError(result, stage, esp_err_to_name(sync_err));
        return false;
    }

    return true;
}

bool TouchPersistentPsramProbeWithCacheSync(WasmReplayControlResult *result)
{
    if (!SyncPersistentPsramProbe(result, "persistent-touch-sync-pre-msync")) {
        return false;
    }

    for (std::size_t i = 0; i < g_persistent_psram_probe_bytes; ++i) {
        g_persistent_psram_probe[i] ^= static_cast<uint8_t>((i * 13u) + 0x3Cu);
    }

    uint32_t checksum = 0;
    for (std::size_t i = 0; i < g_persistent_psram_probe_bytes; i += 1024) {
        checksum = (checksum * 131u) ^ g_persistent_psram_probe[i];
    }
    checksum = (checksum * 131u) ^ g_persistent_psram_probe[g_persistent_psram_probe_bytes - 1];

    std::printf("persistent_psram_probe.touch_buffer=%p\n", static_cast<void *>(g_persistent_psram_probe));
    std::printf("persistent_psram_probe.touch_external=%s\n",
                esp_ptr_external_ram(g_persistent_psram_probe) ? "yes" : "no");
    std::printf("persistent_psram_probe.touch_checksum=0x%08" PRIx32 "\n", checksum);

    if (!SyncPersistentPsramProbe(result, "persistent-touch-sync-post-msync")) {
        return false;
    }

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

    if (std::strcmp(name, "psram-persistent-init") == 0) {
        result.queued_commands = 0;
        if (!InitPersistentPsramProbe(&result)) {
            return result;
        }
        result.success = true;
        return result;
    }

    if (std::strcmp(name, "psram-persistent-touch-sync") == 0) {
        result.queued_commands = 0;
        if (!TouchPersistentPsramProbeWithCacheSync(&result)) {
            return result;
        }
        result.success = true;
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
