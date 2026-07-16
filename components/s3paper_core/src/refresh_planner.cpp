#include "s3paper/refresh_planner.h"

namespace s3paper {
namespace {

// Distance-aware overlap: rects whose expanded bounds intersect merge.
bool ShouldMerge(const Rect &a, const Rect &b, int32_t distance) {
    const Rect expanded{a.x - distance, a.y - distance, a.w + 2 * distance,
                        a.h + 2 * distance};
    const Result<Rect> overlap = Intersect(expanded, b);
    return overlap.ok() && !IsEmpty(overlap.value);
}

EpdWaveform WaveformForIntent(PresentIntent intent) {
    switch (intent) {
        case PresentIntent::InteractiveInk: return EpdWaveform::Fastest;
        case PresentIntent::TextRegion: return EpdWaveform::Fast;
        case PresentIntent::TextPage: return EpdWaveform::Text;
        case PresentIntent::ImageQuality: return EpdWaveform::Quality;
        case PresentIntent::CleanFull: return EpdWaveform::Quality;
    }
    return EpdWaveform::Quality;
}

}  // namespace

const char *EpdWaveformName(EpdWaveform waveform) {
    switch (waveform) {
        case EpdWaveform::Quality: return "Quality";
        case EpdWaveform::Text: return "Text";
        case EpdWaveform::Fast: return "Fast";
        case EpdWaveform::Fastest: return "Fastest";
    }
    return "Unknown";
}

const char *RefreshReasonName(RefreshReason reason) {
    switch (reason) {
        case RefreshReason::PartialDamage: return "PartialDamage";
        case RefreshReason::FirstRender: return "FirstRender";
        case RefreshReason::ScreenChange: return "ScreenChange";
        case RefreshReason::Wake: return "Wake";
        case RefreshReason::ExplicitRequest: return "ExplicitRequest";
        case RefreshReason::BudgetTurns: return "BudgetTurns";
        case RefreshReason::BudgetPartialArea: return "BudgetPartialArea";
        case RefreshReason::BudgetElapsed: return "BudgetElapsed";
    }
    return "Unknown";
}

RefreshPlanner::RefreshPlanner(Size viewport, RefreshPolicy policy)
    : viewport_(viewport), policy_(policy) {}

Status RefreshPlanner::AddDamage(const Rect &r) {
    const Result<Rect> aligned =
        AlignDamageForEpd(r, viewport_, policy_.align_x);
    if (!aligned.ok()) {
        return ErrStatus(aligned.code);
    }
    if (IsEmpty(aligned.value)) {
        return OkStatus();  // off-screen or empty damage is a no-op
    }
    Rect incoming = aligned.value;
    // Merge with any existing rect it touches; a merge can cascade, so
    // restart the scan after each absorption.
    for (uint32_t i = 0; i < damage_count_;) {
        if (ShouldMerge(damage_[i], incoming, policy_.merge_distance)) {
            const Result<Rect> merged = Union(damage_[i], incoming);
            if (!merged.ok()) {
                return ErrStatus(merged.code);
            }
            incoming = merged.value;
            damage_[i] = damage_[--damage_count_];
            i = 0;
            continue;
        }
        ++i;
    }
    if (damage_count_ < kMaxDamageRects) {
        damage_[damage_count_++] = incoming;
        return OkStatus();
    }
    // Capacity fallback: collapse everything into one bounding rect.
    Rect all = incoming;
    for (uint32_t i = 0; i < damage_count_; ++i) {
        const Result<Rect> merged = Union(all, damage_[i]);
        if (!merged.ok()) {
            return ErrStatus(merged.code);
        }
        all = merged.value;
    }
    damage_[0] = all;
    damage_count_ = 1;
    history_.merge_fallbacks++;
    return OkStatus();
}

void RefreshPlanner::ClearDamage() { damage_count_ = 0; }

void RefreshPlanner::NoteScreenChange() { screen_changed_ = true; }
void RefreshPlanner::NoteWake() { woke_ = true; }
void RefreshPlanner::RequestFull() { full_requested_ = true; }

RefreshPlan RefreshPlanner::Plan(PresentIntent intent, int64_t now_us) const {
    RefreshPlan plan{};
    plan.waveform = WaveformForIntent(intent);

    RefreshReason full_reason = RefreshReason::PartialDamage;
    bool full = false;
    if (!history_.first_render_done) {
        full = true;
        full_reason = RefreshReason::FirstRender;
    } else if (woke_) {
        full = true;
        full_reason = RefreshReason::Wake;
    } else if (screen_changed_) {
        full = true;
        full_reason = RefreshReason::ScreenChange;
    } else if (full_requested_ || intent == PresentIntent::CleanFull) {
        full = true;
        full_reason = RefreshReason::ExplicitRequest;
    } else if (history_.turns_since_full + 1 >
               policy_.max_turns_between_full) {
        full = true;
        full_reason = RefreshReason::BudgetTurns;
    } else if (history_.partial_area_since_full >=
               policy_.max_partial_area_between_full) {
        full = true;
        full_reason = RefreshReason::BudgetPartialArea;
    } else if (now_us - history_.last_full_us >=
               policy_.max_elapsed_us_between_full) {
        full = true;
        full_reason = RefreshReason::BudgetElapsed;
    }

    plan.reason = full_reason;
    if (full) {
        plan.full_refresh = true;
        plan.waveform = EpdWaveform::Quality;
        plan.regions[0] = Rect{0, 0, viewport_.w, viewport_.h};
        plan.region_count = 1;
        plan.aligned_area = static_cast<uint64_t>(viewport_.w) * viewport_.h;
        return plan;
    }
    plan.full_refresh = false;
    plan.region_count = damage_count_;
    for (uint32_t i = 0; i < damage_count_; ++i) {
        plan.regions[i] = damage_[i];
        plan.aligned_area += static_cast<uint64_t>(Area(damage_[i]));
    }
    return plan;
}

void RefreshPlanner::RecordPresent(const RefreshPlan &plan, int64_t now_us) {
    if (plan.full_refresh) {
        history_.fulls_total++;
        history_.turns_since_full = 0;
        history_.partial_area_since_full = 0;
        history_.last_full_us = now_us;
        history_.first_render_done = true;
        screen_changed_ = false;
        woke_ = false;
        full_requested_ = false;
    } else {
        history_.partials_total++;
        history_.turns_since_full++;
        history_.partial_area_since_full += plan.aligned_area;
    }
    damage_count_ = 0;
}

}  // namespace s3paper
