// Input primitives (Phase 4): touch normalization, pointer state machine,
// gesture recognition, hit regions, and a monotonic deadline scheduler.
//
// Pure header/impl: host-replayable with recorded touch traces. Hardware
// polling stays in the firmware; this layer only consumes normalized
// samples stamped with monotonic time.
#pragma once

#include <stdint.h>

#include "s3paper/geometry.h"
#include "s3paper/status.h"

namespace s3paper {

// Physical panel point -> logical viewport point for a display rotated by
// `rotation` quarter-turns clockwise (matches RotateInBounds, inverse
// direction). physical_bounds is the unrotated panel size.
Result<Point> TouchToLogical(Point physical, const Size &physical_bounds,
                             uint8_t rotation);

// One raw digitizer sample after coordinate normalization.
struct PointerSample {
    Point pos;
    bool touching;
    int64_t t_us;
};

enum class PointerEventKind : uint8_t {
    Down = 0,
    Move,
    Up,
    Cancel,
};

const char *PointerEventKindName(PointerEventKind kind);

struct PointerEvent {
    PointerEventKind kind;
    Point pos;
    int64_t t_us;
};

// Turns polled samples into deduplicated Down/Move/Up/Cancel events.
//  - repeated identical positions while touching emit nothing;
//  - a sample gap longer than stale_timeout_us while touching cancels the
//    sequence (prevents phantom drags after missed releases).
class PointerTracker {
  public:
    explicit PointerTracker(int64_t stale_timeout_us = 500000)
        : stale_timeout_us_(stale_timeout_us) {}

    // Feeds one sample; writes 0..2 events (Cancel may precede Down).
    // Returns the number written. `out` must hold at least 2 events.
    uint32_t Feed(const PointerSample &sample, PointerEvent *out);

    bool touching() const { return touching_; }

  private:
    int64_t stale_timeout_us_;
    bool touching_ = false;
    Point last_pos_{};
    int64_t last_t_us_ = 0;
};

enum class GestureKind : uint8_t {
    Tap = 0,
    LongPress,
    SwipeLeft,
    SwipeRight,
    SwipeUp,
    SwipeDown,
};

const char *GestureKindName(GestureKind kind);

struct GestureEvent {
    GestureKind kind;
    Point pos;  // Down position for taps/long-presses; Down position for swipes
    int64_t t_us;
};

struct GestureConfig {
    int64_t tap_max_us = 300000;
    int32_t tap_max_dist = 16;
    int64_t long_press_us = 600000;
    int32_t swipe_min_dist = 64;
    // Off-axis movement may be at most this fraction (numerator/denominator
    // = 1/2) of the main-axis distance for a cardinal swipe.
    int32_t swipe_axis_ratio_num = 1;
    int32_t swipe_axis_ratio_den = 2;
};

// Consumes pointer events and emits gestures. Long-press needs time even
// without events, so call Update(now) periodically.
class GestureDetector {
  public:
    explicit GestureDetector(GestureConfig config = {}) : config_(config) {}

    // Both return the number of gestures written (0 or 1). `out` must hold
    // at least 1 gesture.
    uint32_t Feed(const PointerEvent &event, GestureEvent *out);
    uint32_t Update(int64_t now_us, GestureEvent *out);

  private:
    GestureConfig config_;
    bool active_ = false;
    bool long_press_fired_ = false;
    bool moved_beyond_tap_ = false;
    Point down_pos_{};
    int64_t down_t_us_ = 0;
    Point last_pos_{};
};

// Immutable hit region emitted by layout. Higher z wins; among equal z the
// later entry (paint order) wins. Deterministic by construction.
struct HitRegion {
    Rect rect;
    uint32_t region_id;
    int16_t z;
};

// Returns the region_id of the topmost hit, or Err(InvalidArgument) when
// nothing is hit.
Result<uint32_t> HitTest(const HitRegion *regions, uint32_t count,
                         const Point &p);

// Fixed-capacity monotonic deadline scheduler: one clock for region
// deadlines, persistence deadlines, and inactivity deadlines. Re-adding an
// id replaces its deadline (used for quiet/deferred scheduling).
class Scheduler {
  public:
    static constexpr uint32_t kCapacity = 16;

    Status Add(uint32_t deadline_id, int64_t due_us);
    Status Cancel(uint32_t deadline_id);
    // Earliest pending deadline, if any.
    Result<int64_t> NextDue() const;
    // Pops the earliest deadline with due_us <= now_us. Returns
    // Err(InvalidArgument) when nothing is due.
    Result<uint32_t> PopDue(int64_t now_us);
    uint32_t pending() const { return count_; }

  private:
    struct Entry {
        uint32_t id;
        int64_t due_us;
    };
    Entry entries_[kCapacity];
    uint32_t count_ = 0;
};

}  // namespace s3paper
