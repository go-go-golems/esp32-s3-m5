// Refresh planner (Phase 3): the single owner of display refresh policy.
//
// Widgets and controllers express PresentIntent and damage; this planner
// decides whether an update is partial or a clean full refresh, which
// waveform class to use, and which aligned region to push. Backends map
// EpdWaveform to their vendor mode; nothing else in the system chooses
// display policy.
//
// Pure header/impl: host-testable with synthetic histories.
#pragma once

#include <stdint.h>

#include "s3paper/display_backend.h"
#include "s3paper/geometry.h"
#include "s3paper/status.h"

namespace s3paper {

// Waveform classes, deliberately backend-neutral. The M5 backend maps them
// onto epd_mode_t; a future qualified driver maps them onto real LUTs.
enum class EpdWaveform : uint8_t {
    Quality = 0,
    Text,
    Fast,
    Fastest,
};

const char *EpdWaveformName(EpdWaveform waveform);

enum class RefreshReason : uint8_t {
    PartialDamage = 0,   // ordinary partial update
    FirstRender,
    ScreenChange,
    Wake,
    ExplicitRequest,
    BudgetTurns,
    BudgetPartialArea,
    BudgetElapsed,
};

const char *RefreshReasonName(RefreshReason reason);

struct RefreshPolicy {
    // Clean-full budgets; any exceeded budget forces a full refresh.
    uint32_t max_turns_between_full = 16;
    uint64_t max_partial_area_between_full = 16ULL * 540 * 960;
    int64_t max_elapsed_us_between_full = 15LL * 60 * 1000 * 1000;
    // Damage rects closer than this (in aligned pixels) merge into one.
    int32_t merge_distance = 16;
    // EPD horizontal alignment (provisional 8; see AlignDamageForEpd).
    int32_t align_x = 8;
};

struct RefreshPlan {
    bool full_refresh;
    EpdWaveform waveform;
    RefreshReason reason;
    // Aligned regions to present. For a full refresh: one viewport rect.
    Rect regions[8];
    uint32_t region_count;
    uint64_t aligned_area;
};

struct RefreshHistory {
    bool first_render_done;
    uint32_t turns_since_full;
    uint64_t partial_area_since_full;
    int64_t last_full_us;       // monotonic time of last full refresh
    uint32_t fulls_total;
    uint32_t partials_total;
    uint32_t merge_fallbacks;   // damage overflow -> merged-all events
};

class RefreshPlanner {
  public:
    static constexpr uint32_t kMaxDamageRects = 8;

    explicit RefreshPlanner(Size viewport, RefreshPolicy policy = {});

    // Damage accumulation for the pending update. Rects are clamped and
    // EPD-aligned on entry; overlapping/nearby rects merge. When all slots
    // are occupied and nothing merges, everything collapses into one
    // bounding rect (counted in history as a merge fallback).
    Status AddDamage(const Rect &r);
    void ClearDamage();
    uint32_t pending_damage_count() const { return damage_count_; }

    // Events feeding clean-full triggers.
    void NoteScreenChange();
    void NoteWake();
    void RequestFull();

    // Builds the plan for the pending damage under the given intent at
    // monotonic time now_us. Does not mutate history; call RecordPresent
    // with the backend result afterwards.
    RefreshPlan Plan(PresentIntent intent, int64_t now_us) const;

    // Commits a presented plan into the history and clears pending damage
    // and one-shot triggers.
    void RecordPresent(const RefreshPlan &plan, int64_t now_us);

    const RefreshHistory &history() const { return history_; }
    const RefreshPolicy &policy() const { return policy_; }

  private:
    Size viewport_;
    RefreshPolicy policy_;
    Rect damage_[kMaxDamageRects];
    uint32_t damage_count_ = 0;
    bool screen_changed_ = false;
    bool woke_ = false;
    bool full_requested_ = false;
    RefreshHistory history_{};
};

}  // namespace s3paper
