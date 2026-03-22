#pragma once

#include "gnosis_types.h"

namespace gnosis {

// Run the full layout pass on a screen (960x540).
void LayoutScreen(Screen& screen, int16_t W, int16_t H);

// Recursive layout dispatch for a single node.
void LayoutNode(Node* node, int16_t x, int16_t y, int16_t w, int16_t h);

}  // namespace gnosis
