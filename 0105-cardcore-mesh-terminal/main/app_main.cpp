#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "cardputer_bsp/bringup.h"
#include "meshcore_compat/runtime.h"

namespace {
constexpr char kTag[] = "cardcore";
}

extern "C" void app_main(void) {
    const esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(nvs_err);
    }

    ESP_ERROR_CHECK(cardcore::meshcore_compat::initialize_runtime());

    const esp_err_t board_err = cardcore::bsp::initialize_bringup_diagnostics();
    if (board_err != ESP_OK) {
        ESP_LOGE(kTag, "board bring-up diagnostics failed: %s", esp_err_to_name(board_err));
    }
    ESP_LOGI(kTag, "Cardcore boot: native IDF app with isolated Arduino compatibility runtime");
}
