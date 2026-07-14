// Parses the Mac app "PPA Group Control" presets.json into scenes.
// Schema (design doc §3.2):
//   {"scenes": {"<name>": {"uid_<hex>"|"ip_<addr>": {"id":N,"sub":N,"name":"..."}}}}
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct PpaAction {
    uint32_t uid = 0;      // DeviceUniqueId; 0 = fixed IP addressing
    uint32_t fixed_ip = 0; // network byte order; only used when uid == 0
    uint8_t preset_id = 0;
    uint8_t preset_sub = 0;
    std::string preset_name;
};

struct PpaScene {
    std::string name;
    std::vector<PpaAction> actions;
};

// Returns false on JSON parse error; scenes may be empty on success.
bool scene_model_parse(const char *json, std::vector<PpaScene> &out);
