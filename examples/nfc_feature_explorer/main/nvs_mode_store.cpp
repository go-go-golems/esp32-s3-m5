// SPDX-License-Identifier: MIT
//
// NVS mode store — application-layer persistence for the selected NFC boot
// mode. The component core has no NVS dependency; this adapter lives in the
// example.

#include "nvs_mode_store.hpp"

#include "esp_err.h"
#include "nvs_flash.h"

namespace gogolem::nfc::example {

static constexpr const char* NAMESPACE = "gogolem_nfc";
static constexpr const char* KEY = "boot_mode";

gogolem::nfc::Mode load_boot_mode() {
    nvs_handle_t handle{};
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return Mode::Reader;
    }
    uint8_t value = 0;
    nvs_get_u8(handle, KEY, &value);
    nvs_close(handle);
    switch (value) {
        case 1: return Mode::EmulationUltralight;
        case 2: return Mode::EmulationNtag213;
        default: return Mode::Reader;
    }
}

esp_err_t store_boot_mode(Mode mode) {
    nvs_handle_t handle{};
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_u8(handle, KEY, static_cast<uint8_t>(mode));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

const char* mode_name(Mode mode) {
    switch (mode) {
        case Mode::Reader: return "reader";
        case Mode::EmulationUltralight: return "emulation-ultralight";
        case Mode::EmulationNtag213: return "emulation-ntag213";
        default: return "unknown";
    }
}

}  // namespace gogolem::nfc::example
