// PaperS3 reader-primitives firmware, Phase 1 (ESP-50-PAPERS3-EREADER-PRIMITIVES).
//
// Phase 1 scope: one UI owner task, bounded AppEvent/reply queues, a console
// that only posts messages, diagnostics, and a concurrency stress fixture.
// No display, touch, SD, or JavaScript work belongs here yet.

#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "app_console.h"
#include "app_owner.h"

namespace {

const char *kTag = "app_main";

void LogBootDiagnostics() {
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(kTag, "reader-primitives phase1 boot");
    ESP_LOGI(kTag, "chip=esp32s3 cores=%d rev=%d reset_reason=%d",
             chip.cores, chip.revision, static_cast<int>(esp_reset_reason()));
    ESP_LOGI(kTag, "heap internal_free=%u largest=%u",
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(
                 heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
    ESP_LOGI(kTag, "psram free=%u largest=%u",
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
             static_cast<unsigned>(
                 heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
    ESP_LOGI(kTag, "display backend=none (deferred; panel unqualified)");
}

}  // namespace

extern "C" void app_main(void) {
    LogBootDiagnostics();
    reader::OwnerStart();
    reader::ConsoleStart();
    ESP_LOGI(kTag, "phase1 ready: owner task + console proxy active");
}
