// Render-state diffing and dependency invalidation (Phase 9, design §12.2).
//
// Flow per update: app data changes -> DependencyTracker.MarkDirty(dep) ->
// CollectDirtyWidgets -> app refreshes those nodes' values (SetText etc.,
// which bump content versions) -> re-layout -> RenderStateDiff.Diff yields
// exact damage rects for the refresh planner -> Capture the new state.
// Nodes carry no dirty flags; damage falls out of comparing retained state,
// so a forgotten invalidation shows up as missing damage in tests, not as
// silent stale pixels scattered through app code.
#pragma once

#include <stdint.h>

#include "s3paper/geometry.h"
#include "s3paper/status.h"
#include "s3paper/widget.h"
#include "s3paper/widget_layout.h"

namespace s3paper {

// Bounded dirty-set of DependencyIds.
class DependencyTracker {
  public:
    static constexpr uint32_t kCapacity = 32;

    void Clear();
    Status MarkDirty(DependencyId id);  // 0 is InvalidArgument
    bool IsDirty(DependencyId id) const;
    uint32_t dirty_count() const { return count_; }

    // Live widgets bound (node.dependency) to any dirty id, in arena order.
    // Returns the number written (truncates at cap; never over-reports).
    uint32_t CollectDirtyWidgets(const WidgetArena &arena, WidgetHandle *out,
                                 uint32_t cap) const;

  private:
    DependencyId dirty_[kCapacity];
    uint32_t count_ = 0;
};

// Previous-frame snapshot per arena slot: enough to detect content changes,
// moves/resizes, appearances, and disappearances.
class RenderStateDiff {
  public:
    void Reset();

    // Snapshot the state that was just presented.
    void Capture(const WidgetArena &arena, const LayoutEntry *entries,
                 uint32_t count);

    // Damage rects between the snapshot and the given current layout.
    // Changed nodes contribute old and new frames. Err(CapacityExceeded)
    // means "more damage than cap": present full-viewport instead.
    Result<uint32_t> Diff(const WidgetArena &arena,
                          const LayoutEntry *entries, uint32_t count,
                          Rect *out, uint32_t cap) const;

  private:
    struct Slot {
        bool present;
        uint16_t generation;
        uint32_t version;
        WidgetKind kind;
        Rect frame;
    };
    Slot slots_[WidgetArena::kCapacity];
};

}  // namespace s3paper
