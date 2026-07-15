// Widget UI service (Phase 9). Owner-task-only.
//
// Owns the retained WidgetArena, the PageRouter, render-state diffing, and
// the region table. Screens (reader, library, fixtures) build widget trees
// here and present them through one pipeline: LayoutPage -> CompileTree ->
// optional extra ops (book body) -> refresh planner -> backend. Interval
// regions (fixture clock) update through diff-driven partial presents.
#pragma once

#include "app_events.h"
#include "s3paper/input.h"
#include "s3paper/page.h"
#include "s3paper/widget.h"
#include "s3paper/widget_render.h"

namespace reader {

// Idempotent; allocates the arena in PSRAM. All functions owner-task-only.
void UiInit();

s3paper::WidgetArena &UiArena();
s3paper::PageRouter &UiRouter();

// Ops drawn after widget compilation, into the same frame (book body lines
// use the paginator's absolute baselines). Plain function pointer: widget
// nodes never store callbacks.
using UiExtraOps = StatusCode (*)(s3paper::FrameBuilder &fb,
                                  const s3paper::LayoutEntry *entries,
                                  uint32_t entry_count);

struct UiPresentResult {
    StatusCode status;
    uint32_t hit_count;
    bool full_refresh;
};

// Full-page present of a slot set. screen_change notes a planner screen
// change (route transitions get clean fulls). Deactivates any fixture
// region ticking (the new screen owns the panel now).
UiPresentResult UiPresentPage(const s3paper::PageSlots &slots,
                              s3paper::PresentIntent intent,
                              bool screen_change, s3paper::HitRegion *hits,
                              uint32_t hit_cap, UiExtraOps extra_ops);

// Console fixtures: 1 = hello page, 2 = status page with a live clock
// region (interval updates until another screen presents).
StatusCode UiRunFixture(uint32_t which);

// Owner-loop hook: performs due interval-region updates (diff -> clipped
// re-render -> TextRegion present). Cheap when nothing is active.
void UiRegionTick(int64_t now_us);

// Monotonic count of successful full-page presents. Layers that own a
// transient screen (JS apps, fixtures) compare against the value at their
// own last present to learn another screen took the panel.
uint32_t UiPresentCount();

}  // namespace reader
