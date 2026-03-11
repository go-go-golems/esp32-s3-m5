#pragma once

#include "esp_err.h"

constexpr size_t kRemoteUrlMaxLen = 160;
constexpr size_t kRemoteDeviceIdMaxLen = 32;

struct RemoteConfig {
  char url[kRemoteUrlMaxLen] = {0};
  char device_id[kRemoteDeviceIdMaxLen] = {0};
};

esp_err_t remote_config_load(RemoteConfig* out);
esp_err_t remote_config_save(const RemoteConfig& cfg);
esp_err_t remote_config_clear();
