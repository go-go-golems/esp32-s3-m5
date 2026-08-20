// ESP-55 P3: ROM app asset registry — see js_assets.h.
#include "js_assets.h"

#include <cstring>

// EMBED_TXTFILES symbols (main/CMakeLists.txt): file <id>.js becomes
// _binary_<id>_js_start/_end, NUL-terminated; end points past the NUL.
#define PULP_ASSET(sym) \
    extern const char _binary_##sym##_start[]; \
    extern const char _binary_##sym##_end[];
PULP_ASSET(2048_js)
PULP_ASSET(blitz_js)
PULP_ASSET(daily_js)
PULP_ASSET(dice_js)
PULP_ASSET(gallery_js)
PULP_ASSET(ink_js)
PULP_ASSET(library_js)
PULP_ASSET(postcard_js)
PULP_ASSET(radio_js)
PULP_ASSET(reader_js)
PULP_ASSET(settings_js)
PULP_ASSET(tea_js)
#undef PULP_ASSET

namespace pulp {
namespace {

struct Asset {
    const char *name;
    const char *start;
    const char *end;
};

#define PULP_ASSET_ROW(name, sym) \
    {name, _binary_##sym##_start, _binary_##sym##_end}
const Asset kAssets[] = {
    PULP_ASSET_ROW("2048", 2048_js),
    PULP_ASSET_ROW("blitz", blitz_js),
    PULP_ASSET_ROW("daily", daily_js),
    PULP_ASSET_ROW("dice", dice_js),
    PULP_ASSET_ROW("gallery", gallery_js),
    PULP_ASSET_ROW("ink", ink_js),
    PULP_ASSET_ROW("library", library_js),
    PULP_ASSET_ROW("postcard", postcard_js),
    PULP_ASSET_ROW("radio", radio_js),
    PULP_ASSET_ROW("reader", reader_js),
    PULP_ASSET_ROW("settings", settings_js),
    PULP_ASSET_ROW("tea", tea_js),
};
#undef PULP_ASSET_ROW

constexpr uint32_t kAssetCount = sizeof(kAssets) / sizeof(kAssets[0]);

}  // namespace

bool AssetsFind(const char *name, const char **src, uint32_t *len) {
    for (uint32_t i = 0; i < kAssetCount; ++i) {
        if (std::strcmp(kAssets[i].name, name) == 0) {
            *src = kAssets[i].start;
            // end points past the NUL terminator EMBED_TXTFILES appends.
            *len = static_cast<uint32_t>(kAssets[i].end -
                                         kAssets[i].start) - 1;
            return true;
        }
    }
    return false;
}

uint32_t AssetsCount() { return kAssetCount; }

const char *AssetsName(uint32_t i) {
    return i < kAssetCount ? kAssets[i].name : nullptr;
}

}  // namespace pulp
