#pragma once

#include <M5GFX.h>
#include <cstdint>

namespace gnosis {

static constexpr int kGlyphW = 6;  // 5px glyph + 1px gap
static constexpr int kGlyphH = 8;  // 7px glyph + 1px gap

// Draw text using the built-in 5x7 bitmap font.
// size: pixel multiplier (1, 2, or 4).
// max_chars: if >= 0, truncate to this many characters.
void DrawBitmapText(M5GFX& display, const char* text, int x, int y,
                    int size, uint32_t color, int max_chars = -1);

// Measure text width in pixels for a given size.
int MeasureBitmapText(const char* text, int size);

}  // namespace gnosis
