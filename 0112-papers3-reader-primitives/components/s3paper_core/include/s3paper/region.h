// Overlay regions (Phase 9, design doc §9.6): the unit of partial refresh
// and scheduling for dynamic content (clock, battery, status chips).
//
// A region declares WHAT depends on WHICH data and HOW OFTEN it may update;
// the owner loop wires intervals into the shared Scheduler and presents
// invalid regions through the refresh planner. Regions never own callbacks:
// dynamic values are DependencyIds the app (or future JS layer) resolves.
#pragma once

#include <stdint.h>

#include "s3paper/geometry.h"
#include "s3paper/status.h"
#include "s3paper/widget.h"

namespace s3paper {

struct RegionSpec {
    uint32_t id;
    Rect bounds;             // absolute, from layout
    DependencyId dependency; // 0 = interval/explicit only
    uint32_t interval_ms;    // 0 = event-only
    bool quiet_while_active; // defer updates while the user is interacting
};

// Bounded region registry with explicit invalidation. Deterministic:
// TakeInvalid returns regions in registration order.
class RegionTable {
  public:
    static constexpr uint32_t kCapacity = 8;

    void Clear();
    Status Add(const RegionSpec &spec);  // duplicate id replaces
    uint32_t count() const { return count_; }
    const RegionSpec *At(uint32_t i) const;
    const RegionSpec *Find(uint32_t region_id) const;

    // Marks every region depending on `changed` invalid.
    void Invalidate(DependencyId changed);
    // Marks one region invalid (interval fired, explicit request).
    Status InvalidateRegion(uint32_t region_id);
    // Pops all invalid regions into out (registration order); clears marks.
    uint32_t TakeInvalid(uint32_t *region_ids, uint32_t cap);

  private:
    RegionSpec specs_[kCapacity];
    bool invalid_[kCapacity];
    uint32_t count_ = 0;
};

}  // namespace s3paper
