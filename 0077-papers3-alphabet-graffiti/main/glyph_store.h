#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace alphabet_graffiti {

struct GlyphTemplate {
    char label = '?';
    bool recorded = false;
    std::vector<float> vector;
};

class GlyphStore {
public:
    static constexpr std::size_t kGlyphCount = 36;

    static void InitializeTemplates(std::array<GlyphTemplate, kGlyphCount>& templates);
    static char GlyphForIndex(std::size_t index);
    static int IndexForGlyph(char glyph);

    bool Mount();
    bool Load(std::array<GlyphTemplate, kGlyphCount>& templates);
    bool Save(const std::array<GlyphTemplate, kGlyphCount>& templates);

    const std::string& LastStatus() const { return last_status_; }
    const char* FilePath() const { return "/spiffs/glyph_templates.txt"; }

private:
    bool mounted_ = false;
    std::string last_status_;
};

}  // namespace alphabet_graffiti
