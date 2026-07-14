#include "scene_model.h"

#include <cstdlib>
#include <cstring>

#include "cJSON.h"
#include "lwip/inet.h"

bool scene_model_parse(const char *json, std::vector<PpaScene> &out) {
    out.clear();
    if (json == nullptr || json[0] == '\0') return true;
    cJSON *root = cJSON_Parse(json);
    if (root == nullptr) return false;
    cJSON *scenes = cJSON_GetObjectItem(root, "scenes");
    for (cJSON *sc = scenes ? scenes->child : nullptr; sc != nullptr; sc = sc->next) {
        PpaScene scene;
        scene.name = sc->string ? sc->string : "";
        for (cJSON *m = sc->child; m != nullptr; m = m->next) {
            if (m->string == nullptr) continue;
            PpaAction a;
            if (strncmp(m->string, "uid_", 4) == 0) {
                a.uid = static_cast<uint32_t>(strtoul(m->string + 4, nullptr, 16));
            } else if (strncmp(m->string, "ip_", 3) == 0) {
                a.uid = 0;
                a.fixed_ip = ipaddr_addr(m->string + 3);
                if (a.fixed_ip == IPADDR_NONE) continue;
            } else {
                continue;
            }
            cJSON *id = cJSON_GetObjectItem(m, "id");
            cJSON *sub = cJSON_GetObjectItem(m, "sub");
            cJSON *name = cJSON_GetObjectItem(m, "name");
            a.preset_id = static_cast<uint8_t>(cJSON_IsNumber(id) ? id->valueint : 0);
            a.preset_sub = static_cast<uint8_t>(cJSON_IsNumber(sub) ? sub->valueint : 0);
            if (cJSON_IsString(name)) a.preset_name = name->valuestring;
            scene.actions.push_back(a);
        }
        if (!scene.actions.empty()) out.push_back(std::move(scene));
    }
    cJSON_Delete(root);
    return true;
}
