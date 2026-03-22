#pragma once

#include "gnosis_types.h"
#include <M5GFX.h>

namespace gnosis {

// E-ink grayscale palette
static constexpr uint32_t kColorBg    = 0xFFFFFF;
static constexpr uint32_t kColorFg    = 0x000000;
static constexpr uint32_t kColorMid   = 0x808080;
static constexpr uint32_t kColorLight = 0xC0C0C0;
static constexpr uint32_t kColorGhost = 0xAAAAAA;
static constexpr uint32_t kColorDim   = 0x666666;

uint32_t ResolveColor(int idx);

// Render the entire subtree into the display, clipped to clip_rect.
void RenderSubtree(M5GFX& display, Node* node, const Rect& clip);

// Draw a single leaf widget.
void DrawWidget(M5GFX& display, Node* node, const Rect& clip);

}  // namespace gnosis
