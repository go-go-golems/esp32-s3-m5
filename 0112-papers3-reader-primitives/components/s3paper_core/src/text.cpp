#include "s3paper/text.h"

namespace s3paper {
namespace fontdata {

// The vendored headers use the Adafruit type/macro names; map them onto the
// s3paper structs so the data files stay byte-identical to upstream.
using GFXglyph = GfxGlyph;
using GFXfont = GfxFont;
#define PROGMEM

#include "../fonts/FreeSerif12pt7b.h"
#include "../fonts/FreeSerif18pt7b.h"

#undef PROGMEM

}  // namespace fontdata

const GfxFont *GetFont(uint8_t font_id) {
    switch (font_id) {
        case kFontUi: return &fontdata::FreeSerif12pt7b;
        case kFontBody: return &fontdata::FreeSerif18pt7b;
    }
    return nullptr;
}

bool Utf8Next(const char *text, uint32_t len, uint32_t *pos, uint32_t *cp) {
    if (*pos >= len) {
        return false;
    }
    const auto *bytes = reinterpret_cast<const uint8_t *>(text);
    const uint8_t b0 = bytes[*pos];
    uint32_t need = 0;
    uint32_t value = 0;
    uint32_t min_value = 0;
    if (b0 < 0x80) {
        *cp = b0;
        (*pos)++;
        return true;
    } else if ((b0 & 0xE0) == 0xC0) {
        need = 1;
        value = b0 & 0x1F;
        min_value = 0x80;
    } else if ((b0 & 0xF0) == 0xE0) {
        need = 2;
        value = b0 & 0x0F;
        min_value = 0x800;
    } else if ((b0 & 0xF8) == 0xF0) {
        need = 3;
        value = b0 & 0x07;
        min_value = 0x10000;
    } else {
        // Stray continuation or invalid lead byte.
        *cp = kReplacementChar;
        (*pos)++;
        return true;
    }
    if (*pos + need >= len) {
        // Truncated sequence at end of buffer.
        *cp = kReplacementChar;
        (*pos)++;
        return true;
    }
    for (uint32_t i = 1; i <= need; ++i) {
        const uint8_t b = bytes[*pos + i];
        if ((b & 0xC0) != 0x80) {
            *cp = kReplacementChar;
            (*pos)++;
            return true;
        }
        value = (value << 6) | (b & 0x3F);
    }
    if (value < min_value || value > 0x10FFFF ||
        (value >= 0xD800 && value <= 0xDFFF)) {
        // Overlong, out of range, or surrogate.
        *cp = kReplacementChar;
        (*pos)++;
        return true;
    }
    *cp = value;
    *pos += need + 1;
    return true;
}

Result<GlyphMetrics> GetGlyphMetrics(uint8_t font_id, uint32_t codepoint) {
    const GfxFont *font = GetFont(font_id);
    if (font == nullptr) {
        return Result<GlyphMetrics>::Err(StatusCode::InvalidArgument);
    }
    if (codepoint >= font->first && codepoint <= font->last) {
        const GfxGlyph &g = font->glyphs[codepoint - font->first];
        GlyphMetrics m{};
        m.advance = g.x_advance;
        m.width = g.width;
        m.height = g.height;
        m.x_offset = g.x_offset;
        m.y_offset = g.y_offset;
        m.fallback = false;
        return Result<GlyphMetrics>::Ok(m);
    }
    // Fallback box: deterministic metrics derived from the font size so
    // unknown codepoints stay visible and measurable.
    const int32_t em = font->y_advance;
    GlyphMetrics m{};
    m.advance = (em * 2) / 3;
    m.width = m.advance - 2;
    m.height = (em * 2) / 3;
    m.x_offset = 1;
    m.y_offset = -m.height;
    m.fallback = true;
    return Result<GlyphMetrics>::Ok(m);
}

Result<int32_t> MeasureText(uint8_t font_id, const char *text, uint32_t len) {
    if (GetFont(font_id) == nullptr || (text == nullptr && len > 0)) {
        return Result<int32_t>::Err(StatusCode::InvalidArgument);
    }
    int64_t width = 0;
    uint32_t pos = 0;
    uint32_t cp = 0;
    while (Utf8Next(text, len, &pos, &cp)) {
        const Result<GlyphMetrics> m = GetGlyphMetrics(font_id, cp);
        if (!m.ok()) {
            return Result<int32_t>::Err(m.code);
        }
        width += m.value.advance;
        if (width > INT32_MAX) {
            return Result<int32_t>::Err(StatusCode::InvalidArgument);
        }
    }
    return Result<int32_t>::Ok(static_cast<int32_t>(width));
}

uint32_t SplitParagraphs(const char *text, uint32_t len, TextSpan *out,
                         uint32_t cap) {
    uint32_t count = 0;
    uint32_t start = 0;
    for (uint32_t i = 0; i <= len; ++i) {
        if (i == len || text[i] == '\n') {
            uint32_t end = i;
            if (end > start && text[end - 1] == '\r') {
                end--;  // tolerate CRLF
            }
            if (count < cap) {
                out[count] = TextSpan{start, end - start};
            }
            count++;
            start = i + 1;
        }
    }
    return count;
}

Result<uint32_t> BreakLines(uint8_t font_id, const char *text, uint32_t len,
                            int32_t max_width, LineSpan *out, uint32_t cap) {
    if (GetFont(font_id) == nullptr || max_width <= 0 ||
        (text == nullptr && len > 0)) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    uint32_t line_count = 0;
    uint32_t pos = 0;
    while (pos < len) {
        // Skip leading spaces of the line.
        while (pos < len && text[pos] == ' ') {
            pos++;
        }
        if (pos >= len) {
            break;
        }
        const uint32_t line_start = pos;
        int32_t width = 0;
        uint32_t last_break_end = 0;      // byte end of last fitting word
        int32_t last_break_width = 0;
        uint32_t scan = pos;
        uint32_t line_end = 0;
        int32_t line_width = 0;
        for (;;) {
            // Scan one word (run of non-spaces).
            uint32_t word_end = scan;
            int32_t word_width = 0;
            uint32_t p = scan;
            uint32_t cp = 0;
            uint32_t prev_p = p;
            while (p < len && text[p] != ' ') {
                prev_p = p;
                if (!Utf8Next(text, len, &p, &cp)) {
                    break;
                }
                const Result<GlyphMetrics> m = GetGlyphMetrics(font_id, cp);
                if (!m.ok()) {
                    return Result<uint32_t>::Err(m.code);
                }
                if (width + word_width + m.value.advance > max_width &&
                    last_break_end == 0 && word_width > 0) {
                    // Long-word hard break at a codepoint boundary.
                    word_end = prev_p;
                    goto emit_hard_break;
                }
                word_width += m.value.advance;
                word_end = p;
            }
            if (width + word_width <= max_width || width == 0) {
                // Word fits (or is the forced first word of the line).
                width += word_width;
                last_break_end = word_end;
                last_break_width = width;
                // Consume following spaces (counted for further fitting).
                scan = word_end;
                while (scan < len && text[scan] == ' ') {
                    const Result<GlyphMetrics> sp =
                        GetGlyphMetrics(font_id, ' ');
                    width += sp.value.advance;
                    scan++;
                }
                if (word_end >= len || scan >= len) {
                    line_end = last_break_end;
                    line_width = last_break_width;
                    break;
                }
                continue;
            }
            // Word does not fit: break at the last fitting word end.
            line_end = last_break_end;
            line_width = last_break_width;
            break;
        emit_hard_break:
            line_end = word_end;
            line_width = 0;  // recomputed below
            {
                const Result<int32_t> w = MeasureText(
                    font_id, text + line_start, line_end - line_start);
                if (!w.ok()) {
                    return Result<uint32_t>::Err(w.code);
                }
                line_width = w.value;
            }
            break;
        }
        if (line_end <= line_start) {
            // Guarantee progress: take one codepoint no matter what.
            uint32_t p = line_start;
            uint32_t cp = 0;
            Utf8Next(text, len, &p, &cp);
            line_end = p;
            const Result<int32_t> w = MeasureText(
                font_id, text + line_start, line_end - line_start);
            line_width = w.ok() ? w.value : 0;
        }
        if (line_count < cap) {
            out[line_count] = LineSpan{line_start, line_end - line_start,
                                       line_width};
        }
        line_count++;
        if (line_count > cap) {
            return Result<uint32_t>::Err(StatusCode::CapacityExceeded);
        }
        pos = line_end;
    }
    if (line_count > cap) {
        return Result<uint32_t>::Err(StatusCode::CapacityExceeded);
    }
    return Result<uint32_t>::Ok(line_count);
}

}  // namespace s3paper
