#pragma once

#include <stddef.h>

#include <string>

#include "esp_err.h"

#include "photo_timer_types.h"

esp_err_t preset_store_init(bool format_if_mount_failed);

esp_err_t preset_store_load_or_seed(TimerConfig* out_cfg);
esp_err_t preset_store_save(const TimerConfig& cfg);

esp_err_t preset_store_parse_json(const char* json, size_t len, TimerConfig* out_cfg, std::string* err_detail);
std::string preset_store_to_json(const TimerConfig& cfg);

const char* preset_store_path();
