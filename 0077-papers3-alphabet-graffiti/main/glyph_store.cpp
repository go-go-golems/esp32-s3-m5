#include "glyph_store.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_err.h"
#include "esp_spiffs.h"

namespace alphabet_graffiti {

namespace {

constexpr const char* kStoreVersion = "glyph-store-v1";

}  // namespace

void GlyphStore::InitializeTemplates(std::array<GlyphTemplate, kGlyphCount>& templates)
{
    for (std::size_t i = 0; i < templates.size(); ++i) {
        templates[i] = GlyphTemplate{GlyphForIndex(i), false, {}};
    }
}

char GlyphStore::GlyphForIndex(std::size_t index)
{
    if (index < 26) {
        return static_cast<char>('A' + static_cast<int>(index));
    }
    return static_cast<char>('0' + static_cast<int>(index - 26));
}

int GlyphStore::IndexForGlyph(char glyph)
{
    if (glyph >= 'A' && glyph <= 'Z') {
        return glyph - 'A';
    }
    if (glyph >= '0' && glyph <= '9') {
        return 26 + (glyph - '0');
    }
    return -1;
}

bool GlyphStore::Mount()
{
    if (mounted_) {
        last_status_ = "SPIFFS already mounted.";
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 4,
        .format_if_mount_failed = true,
    };

    const esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        last_status_ = std::string("SPIFFS mount failed: ") + esp_err_to_name(err);
        return false;
    }

    mounted_ = true;

    size_t total = 0;
    size_t used = 0;
    if (esp_spiffs_info("storage", &total, &used) == ESP_OK) {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "SPIFFS ready: %u/%u bytes used.", static_cast<unsigned>(used),
                      static_cast<unsigned>(total));
        last_status_ = buffer;
    } else {
        last_status_ = "SPIFFS mounted.";
    }
    return true;
}

bool GlyphStore::Load(std::array<GlyphTemplate, kGlyphCount>& templates)
{
    if (!mounted_ && !Mount()) {
        return false;
    }

    FILE* file = std::fopen(FilePath(), "r");
    if (!file) {
        last_status_ = std::string("No saved glyph file yet at ") + FilePath();
        return true;
    }

    InitializeTemplates(templates);

    char line[1024];
    if (!std::fgets(line, sizeof(line), file)) {
        std::fclose(file);
        last_status_ = "Glyph file exists but is empty.";
        return true;
    }

    if (std::strncmp(line, kStoreVersion, std::strlen(kStoreVersion)) != 0) {
        std::fclose(file);
        last_status_ = "Glyph file version mismatch.";
        return false;
    }

    std::size_t loaded_count = 0;
    while (std::fgets(line, sizeof(line), file)) {
        char* cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (*cursor == '\0' || *cursor == '\n' || *cursor == '#') {
            continue;
        }

        const char glyph = *cursor++;
        const int glyph_index = IndexForGlyph(glyph);
        if (glyph_index < 0) {
            continue;
        }

        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }

        char* end_ptr = nullptr;
        const long count = std::strtol(cursor, &end_ptr, 10);
        if (count <= 0 || !end_ptr) {
            continue;
        }

        std::vector<float> vector;
        vector.reserve(static_cast<std::size_t>(count));
        cursor = end_ptr;
        for (long i = 0; i < count; ++i) {
            const float value = std::strtof(cursor, &end_ptr);
            if (end_ptr == cursor) {
                break;
            }
            vector.push_back(value);
            cursor = end_ptr;
        }

        if (vector.size() != static_cast<std::size_t>(count)) {
            continue;
        }

        templates[static_cast<std::size_t>(glyph_index)].recorded = true;
        templates[static_cast<std::size_t>(glyph_index)].vector = std::move(vector);
        ++loaded_count;
    }

    std::fclose(file);

    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Loaded %u glyphs from /spiffs.", static_cast<unsigned>(loaded_count));
    last_status_ = buffer;
    return true;
}

bool GlyphStore::Save(const std::array<GlyphTemplate, kGlyphCount>& templates)
{
    if (!mounted_ && !Mount()) {
        return false;
    }

    FILE* file = std::fopen(FilePath(), "w");
    if (!file) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "Open failed for %s: errno=%d", FilePath(), errno);
        last_status_ = buffer;
        return false;
    }

    std::fprintf(file, "%s\n", kStoreVersion);

    std::size_t saved_count = 0;
    for (const auto& glyph : templates) {
        if (!glyph.recorded || glyph.vector.empty()) {
            continue;
        }

        std::fprintf(file, "%c %u", glyph.label, static_cast<unsigned>(glyph.vector.size()));
        for (float value : glyph.vector) {
            std::fprintf(file, " %.6f", static_cast<double>(value));
        }
        std::fprintf(file, "\n");
        ++saved_count;
    }

    std::fclose(file);

    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Saved %u glyphs to /spiffs.", static_cast<unsigned>(saved_count));
    last_status_ = buffer;
    return true;
}

}  // namespace alphabet_graffiti
