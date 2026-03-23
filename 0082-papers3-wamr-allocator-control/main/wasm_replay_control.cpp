#include "wasm_replay_control.h"

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
constexpr std::size_t kScratchBufferBytes = (960 * 540) / 2;
constexpr std::size_t kInternalScratchBytes = 32 * 1024;

uint8_t *g_persistent_psram_probe = nullptr;
std::size_t g_persistent_psram_probe_bytes = 0;
uint8_t *g_aligned_persistent_psram_probe = nullptr;
std::size_t g_aligned_persistent_psram_probe_bytes = 0;
std::size_t g_aligned_persistent_psram_probe_alignment = 0;
uint32_t g_replay_memory_probe_budget = 24;

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

void PrintReplayMemoryState(const char *stage)
{
    if (g_replay_memory_probe_budget == 0) {
        return;
    }

    g_replay_memory_probe_budget--;
    const bool flash_cache_enabled = spi_flash_cache_enabled();
    const bool internal_heap_ok = heap_caps_check_integrity(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, false);
    const bool spiram_heap_ok = heap_caps_check_integrity(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, false);
    const size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    std::printf(
        "replay_mem.stage=%s\n"
        "replay_mem.flash_cache_enabled=%s\n"
        "replay_mem.internal_heap_ok=%s\n"
        "replay_mem.spiram_heap_ok=%s\n"
        "replay_mem.internal_free=%u\n"
        "replay_mem.spiram_free=%u\n",
        stage != nullptr ? stage : "unknown", flash_cache_enabled ? "yes" : "no",
        internal_heap_ok ? "yes" : "no", spiram_heap_ok ? "yes" : "no",
        static_cast<unsigned>(internal_free), static_cast<unsigned>(spiram_free));
}

bool RunPsramScratchProbe(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("psram-scratch-before-alloc");
    auto *buffer = static_cast<uint8_t *>(
        heap_caps_aligned_alloc(16, kScratchBufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer == nullptr) {
        SetReplayError(result, "psram-alloc", "heap_caps_aligned_alloc failed");
        return false;
    }

    std::memset(buffer, 0, kScratchBufferBytes);
    for (std::size_t i = 0; i < kScratchBufferBytes; ++i) {
        buffer[i] = static_cast<uint8_t>((i * 13u) ^ 0xA5u);
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
    PrintReplayMemoryState("psram-scratch-before-free");

    heap_caps_free(buffer);
    return true;
}

bool RunInternalScratchProbe(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("internal-scratch-before-alloc");
    auto *buffer = static_cast<uint8_t *>(
        heap_caps_aligned_alloc(16, kInternalScratchBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (buffer == nullptr) {
        SetReplayError(result, "internal-alloc", "heap_caps_aligned_alloc failed");
        return false;
    }

    std::memset(buffer, 0, kInternalScratchBytes);
    for (std::size_t i = 0; i < kInternalScratchBytes; ++i) {
        buffer[i] = static_cast<uint8_t>((i * 17u) ^ 0x5Au);
    }

    uint32_t checksum = 0;
    for (std::size_t i = 0; i < kInternalScratchBytes; i += 1024) {
        checksum = (checksum * 131u) ^ buffer[i];
    }
    checksum = (checksum * 131u) ^ buffer[kInternalScratchBytes - 1];

    std::printf("internal_probe.buffer=%p\n", static_cast<void *>(buffer));
    std::printf("internal_probe.external=%s\n", esp_ptr_external_ram(buffer) ? "yes" : "no");
    std::printf("internal_probe.bytes=%u\n", static_cast<unsigned>(kInternalScratchBytes));
    std::printf("internal_probe.checksum=0x%08" PRIx32 "\n", checksum);
    PrintReplayMemoryState("internal-scratch-before-free");

    heap_caps_free(buffer);
    return true;
}

bool InitPersistentPsramProbe(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("persistent-init-before-alloc");
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
    PrintReplayMemoryState("persistent-init-after-alloc");
    return true;
}

bool InitAlignedPersistentPsramProbe(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("aligned-persistent-init-before-alloc");
    if (g_aligned_persistent_psram_probe != nullptr) {
        heap_caps_free(g_aligned_persistent_psram_probe);
        g_aligned_persistent_psram_probe = nullptr;
        g_aligned_persistent_psram_probe_bytes = 0;
        g_aligned_persistent_psram_probe_alignment = 0;
    }

    size_t cache_alignment = 0;
    const esp_err_t align_err = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &cache_alignment);
    std::printf("aligned_persistent_psram_probe.cache_alignment_err=%s\n",
                esp_err_to_name(align_err));
    if (align_err != ESP_OK || cache_alignment == 0) {
        SetReplayError(result, "aligned-persistent-align", esp_err_to_name(align_err));
        return false;
    }

    g_aligned_persistent_psram_probe = static_cast<uint8_t *>(heap_caps_aligned_alloc(
        cache_alignment, kScratchBufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_aligned_persistent_psram_probe == nullptr) {
        SetReplayError(result, "aligned-persistent-alloc", "heap_caps_aligned_alloc failed");
        return false;
    }

    g_aligned_persistent_psram_probe_bytes = kScratchBufferBytes;
    g_aligned_persistent_psram_probe_alignment = cache_alignment;
    std::memset(g_aligned_persistent_psram_probe, 0x22, g_aligned_persistent_psram_probe_bytes);

    std::printf("aligned_persistent_psram_probe.buffer=%p\n",
                static_cast<void *>(g_aligned_persistent_psram_probe));
    std::printf("aligned_persistent_psram_probe.external=%s\n",
                esp_ptr_external_ram(g_aligned_persistent_psram_probe) ? "yes" : "no");
    std::printf("aligned_persistent_psram_probe.bytes=%u\n",
                static_cast<unsigned>(g_aligned_persistent_psram_probe_bytes));
    std::printf("aligned_persistent_psram_probe.alignment=%u\n",
                static_cast<unsigned>(g_aligned_persistent_psram_probe_alignment));
    PrintReplayMemoryState("aligned-persistent-init-after-alloc");
    return true;
}

bool TouchPersistentPsramProbe(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("persistent-touch-before-write");
    if (g_persistent_psram_probe == nullptr || g_persistent_psram_probe_bytes != kScratchBufferBytes) {
        SetReplayError(result, "persistent-touch", "persistent PSRAM probe is not initialized");
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
    PrintReplayMemoryState("persistent-touch-after-write");
    return true;
}

bool TouchAlignedPersistentPsramProbeWithCacheSync(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("aligned-persistent-touch-before-msync");
    if (g_aligned_persistent_psram_probe == nullptr
        || g_aligned_persistent_psram_probe_bytes != kScratchBufferBytes) {
        SetReplayError(result, "aligned-persistent-touch", "aligned persistent PSRAM probe is not initialized");
        return false;
    }

    const esp_err_t pre_sync_err = esp_cache_msync(g_aligned_persistent_psram_probe,
                                                   g_aligned_persistent_psram_probe_bytes,
                                                   ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    std::printf("aligned_persistent_psram_probe.pre_sync_err=%s\n",
                esp_err_to_name(pre_sync_err));
    if (pre_sync_err != ESP_OK) {
        SetReplayError(result, "aligned-persistent-pre-msync", esp_err_to_name(pre_sync_err));
        return false;
    }

    for (std::size_t i = 0; i < g_aligned_persistent_psram_probe_bytes; ++i) {
        g_aligned_persistent_psram_probe[i] ^= static_cast<uint8_t>((i * 7u) + 0x51u);
    }

    uint32_t checksum = 0;
    for (std::size_t i = 0; i < g_aligned_persistent_psram_probe_bytes; i += 1024) {
        checksum = (checksum * 131u) ^ g_aligned_persistent_psram_probe[i];
    }
    checksum =
        (checksum * 131u) ^ g_aligned_persistent_psram_probe[g_aligned_persistent_psram_probe_bytes - 1];

    const esp_err_t post_sync_err =
        esp_cache_msync(g_aligned_persistent_psram_probe, g_aligned_persistent_psram_probe_bytes,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    std::printf("aligned_persistent_psram_probe.post_sync_err=%s\n",
                esp_err_to_name(post_sync_err));
    std::printf("aligned_persistent_psram_probe.touch_checksum=0x%08" PRIx32 "\n", checksum);
    PrintReplayMemoryState("aligned-persistent-touch-after-write");
    if (post_sync_err != ESP_OK) {
        SetReplayError(result, "aligned-persistent-post-msync", esp_err_to_name(post_sync_err));
        return false;
    }
    return true;
}

bool SyncPersistentPsramProbe(WasmReplayControlResult *result, const char *stage, int flags)
{
    if (g_persistent_psram_probe == nullptr || g_persistent_psram_probe_bytes != kScratchBufferBytes) {
        SetReplayError(result, stage, "persistent PSRAM probe is not initialized");
        return false;
    }

    size_t cache_alignment = 0;
    const esp_err_t align_err = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &cache_alignment);
    std::printf("persistent_psram_probe.cache_alignment_err=%s\n", esp_err_to_name(align_err));
    if (align_err == ESP_OK) {
        std::printf("persistent_psram_probe.cache_alignment=%u\n",
                    static_cast<unsigned>(cache_alignment));
    }

    const esp_err_t sync_err =
        esp_cache_msync(g_persistent_psram_probe, g_persistent_psram_probe_bytes, flags);
    std::printf("persistent_psram_probe.sync_flags=0x%x\n", flags);
    std::printf("persistent_psram_probe.sync_err=%s\n", esp_err_to_name(sync_err));
    if (sync_err != ESP_OK) {
        SetReplayError(result, stage, esp_err_to_name(sync_err));
        return false;
    }

    return true;
}

bool TouchPersistentPsramProbeWithCacheSync(WasmReplayControlResult *result)
{
    PrintReplayMemoryState("persistent-touch-sync-before-msync");
    if (!SyncPersistentPsramProbe(
            result, "persistent-touch-sync-pre-msync",
            ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE
                | ESP_CACHE_MSYNC_FLAG_UNALIGNED)) {
        return false;
    }

    if (!TouchPersistentPsramProbe(result)) {
        return false;
    }

    PrintReplayMemoryState("persistent-touch-sync-before-post-msync");
    if (!SyncPersistentPsramProbe(
            result, "persistent-touch-sync-post-msync",
            ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE
                | ESP_CACHE_MSYNC_FLAG_UNALIGNED)) {
        return false;
    }

    PrintReplayMemoryState("persistent-touch-sync-after-post-msync");
    return true;
}

bool FreePersistentPsramProbe()
{
    if (g_persistent_psram_probe != nullptr) {
        heap_caps_free(g_persistent_psram_probe);
        g_persistent_psram_probe = nullptr;
        g_persistent_psram_probe_bytes = 0;
    }
    if (g_aligned_persistent_psram_probe != nullptr) {
        heap_caps_free(g_aligned_persistent_psram_probe);
        g_aligned_persistent_psram_probe = nullptr;
        g_aligned_persistent_psram_probe_bytes = 0;
        g_aligned_persistent_psram_probe_alignment = 0;
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

    if (std::strcmp(name, "psram-scratch") == 0) {
        result.queued_commands = 0;
        if (!RunPsramScratchProbe(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "internal-scratch") == 0) {
        result.queued_commands = 0;
        if (!RunInternalScratchProbe(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "psram-persistent-init") == 0) {
        result.queued_commands = 0;
        if (!InitPersistentPsramProbe(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "psram-persistent-touch") == 0) {
        result.queued_commands = 0;
        if (!TouchPersistentPsramProbe(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "psram-persistent-touch-sync") == 0) {
        result.queued_commands = 0;
        if (!TouchPersistentPsramProbeWithCacheSync(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "psram-cacheline-persistent-init") == 0) {
        result.queued_commands = 0;
        if (!InitAlignedPersistentPsramProbe(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "psram-cacheline-persistent-touch-sync") == 0) {
        result.queued_commands = 0;
        if (!TouchAlignedPersistentPsramProbeWithCacheSync(&result)) {
            return result;
        }
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else if (std::strcmp(name, "psram-persistent-free") == 0) {
        result.queued_commands = 0;
        FreePersistentPsramProbe();
        result.success = true;
        SetReplayError(&result, nullptr, nullptr);
        return result;
    }
    else {
        SetReplayError(&result, "lookup", "unknown replay example");
        return result;
    }
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
