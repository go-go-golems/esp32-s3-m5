#include "meshcore_compat/runtime.h"

#include <Arduino.h>

#include "esp_log.h"

namespace cardcore::meshcore_compat {
namespace {
constexpr char kTag[] = "meshcore_compat";
bool s_initialized = false;
} // namespace

esp_err_t initialize_runtime() {
    if (s_initialized) {
        return ESP_OK;
    }

    initArduino();
    s_initialized = true;
    ESP_LOGI(kTag, "Arduino compatibility runtime initialized");
    return ESP_OK;
}

} // namespace cardcore::meshcore_compat
