#include "remote_config.h"

#include <cstring>

#include "nvs.h"

namespace {

constexpr const char* kNamespace = "remote";
constexpr const char* kKeyUrl = "url";
constexpr const char* kKeyDeviceId = "device_id";
constexpr const char* kKeyScriptEnabled = "script_enabled";

esp_err_t open_nvs(nvs_handle_t* out) {
  if (!out) {
    return ESP_ERR_INVALID_ARG;
  }
  return nvs_open(kNamespace, NVS_READWRITE, out);
}

void load_string(nvs_handle_t nvs, const char* key, char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  size_t len = out_len;
  if (nvs_get_str(nvs, key, out, &len) != ESP_OK) {
    out[0] = '\0';
  }
}

}  // namespace

esp_err_t remote_config_load(RemoteConfig* out) {
  if (!out) {
    return ESP_ERR_INVALID_ARG;
  }
  std::memset(out, 0, sizeof(*out));

  nvs_handle_t nvs = 0;
  esp_err_t err = open_nvs(&nvs);
  if (err != ESP_OK) {
    return err;
  }

  load_string(nvs, kKeyUrl, out->url, sizeof(out->url));
  load_string(nvs, kKeyDeviceId, out->device_id, sizeof(out->device_id));
  uint8_t script_enabled = 0;
  if (nvs_get_u8(nvs, kKeyScriptEnabled, &script_enabled) == ESP_OK) {
    out->remote_script_enabled = script_enabled != 0;
  }
  nvs_close(nvs);
  return ESP_OK;
}

esp_err_t remote_config_save(const RemoteConfig& cfg) {
  nvs_handle_t nvs = 0;
  esp_err_t err = open_nvs(&nvs);
  if (err != ESP_OK) {
    return err;
  }

  err = nvs_set_str(nvs, kKeyUrl, cfg.url);
  if (err == ESP_OK) {
    err = nvs_set_str(nvs, kKeyDeviceId, cfg.device_id);
  }
  if (err == ESP_OK) {
    err = nvs_set_u8(nvs, kKeyScriptEnabled, cfg.remote_script_enabled ? 1 : 0);
  }
  if (err == ESP_OK) {
    err = nvs_commit(nvs);
  }
  nvs_close(nvs);
  return err;
}

esp_err_t remote_config_clear() {
  nvs_handle_t nvs = 0;
  esp_err_t err = open_nvs(&nvs);
  if (err != ESP_OK) {
    return err;
  }

  (void)nvs_erase_key(nvs, kKeyUrl);
  (void)nvs_erase_key(nvs, kKeyDeviceId);
  (void)nvs_erase_key(nvs, kKeyScriptEnabled);
  err = nvs_commit(nvs);
  nvs_close(nvs);
  return err;
}
