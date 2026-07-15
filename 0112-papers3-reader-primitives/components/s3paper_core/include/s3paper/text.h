// Text primitives (Phase 5): UTF-8 decoding, font metrics, measurement,
// and measured line breaking.
//
// One metrics source: the vendored Adafruit-GFX font data is used both for
// host-side measurement/pagination and device-side glyph blitting. If
// layout measured with different metrics than rendering, pages would drift
// and stored positions would lose meaning (the 0080 lesson).
#pragma once

#include <stdint.h>

#include "s3paper/status.h"

namespace s3paper {

// ---- UTF-8 ----

constexpr uint32_t kReplacementChar = 0xFFFD;

// Decodes the codepoint at *pos, advancing *pos past it. Malformed input
// yields kReplacementChar and advances exactly one byte, so progress is
// guaranteed and positions stay byte-accurate. Returns false at end.
bool Utf8Next(const char *text, uint32_t len, uint32_t *pos, uint32_t *cp);

// ---- Fonts (Adafruit GFX data layout) ----

struct GfxGlyph {
    uint16_t bitmap_offset;
    uint8_t width;
    uint8_t height;
    uint8_t x_advance;
    int8_t x_offset;
    int8_t y_offset;
};

struct GfxFont {
    const uint8_t *bitmaps;
    const GfxGlyph *glyphs;
    uint16_t first;
    uint16_t last;
    uint8_t y_advance;  // baseline-to-baseline line height
};

// Font ids are stable across host and device.
enum : uint8_t {
    kFontUi = 0,    // FreeSerif 12pt
    kFontBody = 1,  // FreeSerif 18pt
    kFontCount = 2,
};

// Returns nullptr for unknown ids.
const GfxFont *GetFont(uint8_t font_id);

// Metrics for one codepoint. Codepoints outside the font map to a fallback
// box glyph with deterministic metrics (never a silent zero advance).
struct GlyphMetrics {
    int32_t advance;
    int32_t width;
    int32_t height;
    int32_t x_offset;
    int32_t y_offset;
    bool fallback;  // true when the codepoint is not in the font
};

Result<GlyphMetrics> GetGlyphMetrics(uint8_t font_id, uint32_t codepoint);

// Measured width of a UTF-8 string (sum of advances, fallback included).
Result<int32_t> MeasureText(uint8_t font_id, const char *text, uint32_t len);

// ---- Paragraphs and lines ----

struct TextSpan {
    uint32_t byte_start;
    uint32_t byte_len;
};

// Splits on '\n' ('\r\n' tolerated); the newline bytes belong to no span.
// Returns the paragraph count (may exceed cap; only cap spans are written,
// so callers can size and retry or stream).
uint32_t SplitParagraphs(const char *text, uint32_t len, TextSpan *out,
                         uint32_t cap);

struct LineSpan {
    uint32_t byte_start;
    uint32_t byte_len;  // trailing spaces excluded
    int32_t width;
};

// Greedy measured line breaking of one paragraph:
//  - prefers breaking at ASCII spaces;
//  - a word wider than max_width hard-breaks at a codepoint boundary
//    (never inside a UTF-8 sequence);
//  - always makes progress (at least one codepoint per line).
// Returns Err(CapacityExceeded) if the paragraph needs more than cap lines;
// out then holds the first cap lines.
Result<uint32_t> BreakLines(uint8_t font_id, const char *text, uint32_t len,
                            int32_t max_width, LineSpan *out, uint32_t cap);

}  // namespace s3paper
