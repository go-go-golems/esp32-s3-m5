#include "config_store.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace {
const char *TAG = "config_store";
const char *kNamespace = "ppadial";

bool read_str(nvs_handle_t h, const char *key, std::string &out) {
    size_t len = 0;
    esp_err_t err = nvs_get_str(h, key, nullptr, &len);
    if (err != ESP_OK || len == 0) return false;
    out.resize(len);
    err = nvs_get_str(h, key, out.data(), &len);
    if (err != ESP_OK) return false;
    out.resize(len - 1); // drop trailing NUL
    return true;
}
} // namespace

bool config_store_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

bool config_store_load(std::string &ssid, std::string &pass, std::string &presets) {
    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READONLY, &h) != ESP_OK) return false;
    read_str(h, "ssid", ssid);
    read_str(h, "pass", pass);
    read_str(h, "presets", presets);
    nvs_close(h);
    return true;
}

bool config_store_save(const std::string &ssid, const std::string &pass,
                       const std::string &presets) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(kNamespace, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }
    bool ok = nvs_set_str(h, "ssid", ssid.c_str()) == ESP_OK &&
              nvs_set_str(h, "pass", pass.c_str()) == ESP_OK &&
              nvs_set_str(h, "presets", presets.c_str()) == ESP_OK &&
              nvs_commit(h) == ESP_OK;
    nvs_close(h);
    if (!ok) ESP_LOGE(TAG, "config save failed");
    return ok;
}
