// PULP OS v2 (ESP-51): JS-first PaperS3 firmware on the shared s3paper
// components. Phase 4 scope: owner task + bounded queues, console proxy,
// input pipeline, power lifecycle, native home page.

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
    ESP_LOGI(kTag, "pulp-os boot");
    ESP_LOGI(kTag, "chip=esp32s3 cores=%d rev=%d reset_reason=%d",
             chip.cores, chip.revision,
             static_cast<int>(esp_reset_reason()));
    ESP_LOGI(kTag, "heap internal_free=%u largest=%u",
             static_cast<unsigned>(
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(
                 heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
    ESP_LOGI(kTag, "psram free=%u largest=%u",
             static_cast<unsigned>(
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
             static_cast<unsigned>(
                 heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
}

}  // namespace

extern "C" void app_main(void) {
    LogBootDiagnostics();
    pulp::OwnerStart();
    pulp::ConsoleStart();
    ESP_LOGI(kTag, "pulp-os ready: owner task + console proxy active");
}
