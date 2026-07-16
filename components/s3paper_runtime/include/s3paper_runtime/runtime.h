// s3paper_runtime: the present pipeline for PaperS3 firmwares (ESP-51 P3).
//
// Owns frame storage (PSRAM), the fake + M5 backends, the refresh planner,
// font registration, the retained WidgetArena/PageRouter, render-state
// diffing, and the region table. Screens build widget trees and present
// them through one pipeline: LayoutPage -> CompileTree -> optional extra
// ops -> refresh planner -> backend. Diff-driven updates blit only changed
// rects (zero visible change = zero EPD work).
//
// Threading: single-owner. Everything here must run on the UI owner task.
#pragma once

#include "s3paper/display_backend.h"
#include "s3paper/frame_builder.h"
#include "s3paper/input.h"
#include "s3paper/page.h"
#include "s3paper/refresh_planner.h"
#include "s3paper/region.h"
#include "s3paper/widget.h"
#include "s3paper/widget_render.h"

namespace s3paper_runtime {

using StatusCode = s3paper::StatusCode;

// A present that went through the refresh planner: backend result plus the
// plan that produced it.
struct PlannedPresent {
    s3paper::PresentResult present;
    s3paper::RefreshPlan plan;
};

struct RuntimeConfig {
    s3paper::Size viewport{540, 960};
    uint32_t op_capacity = 512;
    uint32_t arena_capacity = 32 * 1024;   // frame text arena (bytes)
    uint32_t trace_capacity = 16 * 1024;   // fake-backend trace (bytes)
    // Planner policy; the 64-turn budget matches the hardware-soaked 0112
    // configuration.
    uint32_t max_turns_between_full = 64;
    // Register the four standard faces (PT Serif ui/body, Liberation Sans
    // Bold display/XL) from the fonts embedded in s3paper_core.
    bool register_default_fonts = true;
};

// Allocates frame storage (PSRAM first), initializes the fake backend and
// planner, registers fonts. Idempotent; aborts on allocation failure.
void RuntimeInit(const RuntimeConfig &config);
// Idempotent default-config init (used by lazy accessors).
void RuntimeInit();

// ---- Low-level frame hooks (fixtures, book body composition) ----
s3paper::FrameBuilder &FrameBuilderRef();
s3paper::Result<s3paper::RenderFrame> FinishFrame();
PlannedPresent PresentFramePlanned(const s3paper::RenderFrame &frame,
                                   s3paper::PresentIntent intent,
                                   bool use_m5);
s3paper::RefreshPlanner &Planner();

// ---- Backends ----
s3paper::Status EnsureM5Init();
bool ReadM5Touch(s3paper::PointerSample *out);
s3paper::BackendState FakeBackendState();
s3paper::BackendState M5BackendState();
// The fake backend's normalized trace of the last fake present ("" when
// unavailable); valid until the next fake present.
const char *FakeTrace();
void PrintFakeTrace();

// ---- Widget pipeline ----
s3paper::WidgetArena &Arena();
s3paper::PageRouter &Router();

// Ops drawn after widget compilation, into the same frame (book body lines
// use the paginator's absolute baselines). Plain function pointer: widget
// nodes never store callbacks.
using ExtraOps = StatusCode (*)(s3paper::FrameBuilder &fb,
                                const s3paper::LayoutEntry *entries,
                                uint32_t entry_count);

struct PresentPageResult {
    StatusCode status;
    uint32_t hit_count;
    bool full_refresh;
};

// Full-page present of a slot set. screen_change notes a planner screen
// change (route transitions get clean fulls).
PresentPageResult PresentPage(const s3paper::PageSlots &slots,
                              s3paper::PresentIntent intent,
                              bool screen_change, s3paper::HitRegion *hits,
                              uint32_t hit_cap, ExtraOps extra_ops);

// Diff-driven update of the SAME retained tree presented last: re-layouts,
// diffs against the captured render state, and presents ONLY the changed
// rects (clipped re-render, TextRegion intent). Returns Ok with zero work
// when nothing visible changed; falls back to a full TextPage present when
// there is no valid capture or the damage overflows. Two invariants with
// scars behind them (ESP-50 diary S21): hit regions are NEVER re-collected
// under the damage clip (callers keep their previous hits), and
// CompileTree is always given a hits array (scratch, discarded).
PresentPageResult PresentPageUpdate(const s3paper::PageSlots &slots,
                                    s3paper::HitRegion *hits,
                                    uint32_t hit_cap, ExtraOps extra_ops);

// Monotonic count of successful presents. Layers that own a transient
// screen (JS apps, fixtures) compare against the value at their own last
// present to learn another screen took the panel.
uint32_t PresentCount();

// Trace mode: while enabled, presents render through the FAKE backend
// (normalized trace, no panel) instead of M5.
void SetTracePresent(bool enabled);

// Region spec captured at the last compile (interval regions), or null.
const s3paper::RegionSpec *FindRegion(uint32_t region_id);

}  // namespace s3paper_runtime
