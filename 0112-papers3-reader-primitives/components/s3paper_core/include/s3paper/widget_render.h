// Widget render compilation (Phase 9): laid-out tree -> flat DrawOps plus
// immutable hit regions and region specs.
//
// The overflow rule lives here: every node is clipped to the intersection
// of its ancestors' frames (FrameBuilder drops fully-clipped ops). Hit
// regions are emitted for nodes with hit_id != 0 using the same frames the
// ops were drawn with, so what you see is exactly what you can tap.
#pragma once

#include <stdint.h>

#include "s3paper/frame_builder.h"
#include "s3paper/input.h"
#include "s3paper/region.h"
#include "s3paper/widget_layout.h"

namespace s3paper {

struct CompileResult {
    uint32_t hit_count;
    uint32_t region_count;
};

// Emits ops for all entries into fb (already Begin()-ed) and fills hit
// regions/region specs. hits/regions may be null with cap 0 when unused.
// Overflowing hit or region capacity is an explicit CapacityExceeded.
Result<CompileResult> CompileTree(const WidgetArena &arena,
                                  const LayoutEntry *entries, uint32_t count,
                                  FrameBuilder &fb, HitRegion *hits,
                                  uint32_t hit_cap, RegionSpec *regions,
                                  uint32_t region_cap);

// Frame lookup for app compositing into reserved nodes (e.g. Book body).
// Returns Err(InvalidArgument) when the handle has no layout entry.
Result<Rect> FindFrame(const LayoutEntry *entries, uint32_t count,
                       WidgetHandle handle);

}  // namespace s3paper
