// Widget UI shim (ESP-51 Phase 3): the retained-tree present pipeline is
// the shared s3paper_runtime component now; this header re-exports it
// under the Ui* names 0112 call sites use, and keeps the 0112 console
// fixtures (hello page, status page with a live clock region).
#pragma once

#include "app_events.h"
#include "s3paper_runtime/runtime.h"

namespace reader {

using UiExtraOps = s3paper_runtime::ExtraOps;
using UiPresentResult = s3paper_runtime::PresentPageResult;

// Idempotent; allocates arena + frame storage in PSRAM.
inline void UiInit() { s3paper_runtime::RuntimeInit(); }

inline s3paper::WidgetArena &UiArena() { return s3paper_runtime::Arena(); }
inline s3paper::PageRouter &UiRouter() { return s3paper_runtime::Router(); }

inline UiPresentResult UiPresentPage(const s3paper::PageSlots &slots,
                                     s3paper::PresentIntent intent,
                                     bool screen_change,
                                     s3paper::HitRegion *hits,
                                     uint32_t hit_cap,
                                     UiExtraOps extra_ops) {
    return s3paper_runtime::PresentPage(slots, intent, screen_change, hits,
                                        hit_cap, extra_ops);
}

inline UiPresentResult UiPresentPageUpdate(const s3paper::PageSlots &slots,
                                           s3paper::HitRegion *hits,
                                           uint32_t hit_cap,
                                           UiExtraOps extra_ops) {
    return s3paper_runtime::PresentPageUpdate(slots, hits, hit_cap,
                                              extra_ops);
}

inline uint32_t UiPresentCount() { return s3paper_runtime::PresentCount(); }

inline void UiSetTracePresent(bool enabled) {
    s3paper_runtime::SetTracePresent(enabled);
}

// Console fixtures: 1 = hello page, 2 = status page with a live clock
// region (interval updates until another screen presents).
StatusCode UiRunFixture(uint32_t which);

// Builds a fixture's widget tree without presenting (trace-equivalence
// harness renders it through the fake backend). Resets the arena.
StatusCode UiBuildFixtureSlots(uint32_t which, s3paper::PageSlots *out);

// Owner-loop hook: performs due interval-region updates for the status
// fixture. Cheap when nothing is active.
void UiRegionTick(int64_t now_us);

}  // namespace reader
