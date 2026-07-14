// NVS-backed config: WiFi credentials + raw presets.json text (kept verbatim).
#pragma once

#include <string>

bool config_store_init();
bool config_store_load(std::string &ssid, std::string &pass, std::string &presets);
bool config_store_save(const std::string &ssid, const std::string &pass,
                       const std::string &presets);
