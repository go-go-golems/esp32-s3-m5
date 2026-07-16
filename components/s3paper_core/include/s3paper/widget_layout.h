// Measured widget layout (Phase 9): retained tree -> per-node frames.
//
// Flexbox-lite, matching what the reader actually needs (design doc §6.4):
//  - Row/Col place children along one axis with padding, gap, and alignment;
//  - a child's main size is fixed_w/h if set, else flex share, else its
//    measured intrinsic size; cross size stretches unless aligned;
//  - List is a Col that skips children before first_visible and stops at
//    the first child that does not fit (pagination, never scroll), exposing
//    the shown count so callers can page;
//  - overflow rule everywhere else: children are laid out at their computed
//    size and the render pass clips to the parent frame.
// Text measurement uses the same text.h metrics as pagination, so widget
// text and book text cannot drift apart.
#pragma once

#include <stdint.h>

#include "s3paper/geometry.h"
#include "s3paper/status.h"
#include "s3paper/widget.h"

namespace s3paper {

// One laid-out node, in paint order (parent before children).
struct LayoutEntry {
    WidgetHandle widget;
    uint16_t parent_index;  // index into the entry array; kNoWidgetIndex for root
    Rect frame;             // absolute, viewport coordinates
    uint16_t list_shown;    // List nodes: children actually laid out
};

constexpr uint32_t kMaxLayoutDepth = 16;

// Lays out the tree rooted at `root` into `bounds`. Returns the number of
// entries written, Err(CapacityExceeded) when out has too little room or
// the tree is deeper than kMaxLayoutDepth, Err(InvalidArgument) on a stale
// root handle.
Result<uint32_t> LayoutTree(const WidgetArena &arena, WidgetHandle root,
                            const Rect &bounds, LayoutEntry *out,
                            uint32_t cap);

// Intrinsic size of a subtree (page slots use this to size headers and
// footers before handing the rest to content). parent_horizontal selects
// how axis-dependent leaves (Spacer, Divider) are interpreted.
Result<Size> MeasureWidget(const WidgetArena &arena, WidgetHandle handle,
                           bool parent_horizontal);

}  // namespace s3paper
