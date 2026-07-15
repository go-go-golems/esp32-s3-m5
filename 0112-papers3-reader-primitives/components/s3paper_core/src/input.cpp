#include "s3paper/input.h"

namespace s3paper {
namespace {

int32_t AbsI32(int32_t v) { return v < 0 ? -v : v; }

}  // namespace

const char *PointerEventKindName(PointerEventKind kind) {
    switch (kind) {
        case PointerEventKind::Down: return "Down";
        case PointerEventKind::Move: return "Move";
        case PointerEventKind::Up: return "Up";
        case PointerEventKind::Cancel: return "Cancel";
    }
    return "Unknown";
}

const char *GestureKindName(GestureKind kind) {
    switch (kind) {
        case GestureKind::Tap: return "Tap";
        case GestureKind::LongPress: return "LongPress";
        case GestureKind::SwipeLeft: return "SwipeLeft";
        case GestureKind::SwipeRight: return "SwipeRight";
        case GestureKind::SwipeUp: return "SwipeUp";
        case GestureKind::SwipeDown: return "SwipeDown";
    }
    return "Unknown";
}

Result<Point> TouchToLogical(Point physical, const Size &physical_bounds,
                             uint8_t rotation) {
    if (rotation > 3 || physical_bounds.w <= 0 || physical_bounds.h <= 0) {
        return Result<Point>::Err(StatusCode::InvalidArgument);
    }
    if (physical.x < 0 || physical.y < 0 || physical.x >= physical_bounds.w ||
        physical.y >= physical_bounds.h) {
        return Result<Point>::Err(StatusCode::InvalidArgument);
    }
    // Inverse of RotateInBounds' point maps. For rotation 1 the logical
    // viewport is (H_p, W_p) in physical terms, so the logical y coordinate
    // recovers from the physical x using the physical WIDTH (and vice versa
    // for rotation 3).
    const int32_t W = physical_bounds.w;
    const int32_t H = physical_bounds.h;
    switch (rotation) {
        case 0:
            return Result<Point>::Ok(physical);
        case 1:  // logical->physical: (x,y) -> (H_l-1-y, x); H_l == W
            return Result<Point>::Ok(Point{physical.y, W - 1 - physical.x});
        case 2:
            return Result<Point>::Ok(
                Point{W - 1 - physical.x, H - 1 - physical.y});
        case 3:  // logical->physical: (x,y) -> (y, W_l-1-x); W_l == H
            return Result<Point>::Ok(Point{H - 1 - physical.y, physical.x});
    }
    return Result<Point>::Err(StatusCode::InvalidArgument);
}

uint32_t PointerTracker::Feed(const PointerSample &sample, PointerEvent *out) {
    uint32_t written = 0;
    if (touching_ && sample.t_us - last_t_us_ > stale_timeout_us_) {
        out[written++] = PointerEvent{PointerEventKind::Cancel, last_pos_,
                                      sample.t_us};
        touching_ = false;
    }
    if (sample.touching) {
        if (!touching_) {
            touching_ = true;
            last_pos_ = sample.pos;
            last_t_us_ = sample.t_us;
            out[written++] =
                PointerEvent{PointerEventKind::Down, sample.pos, sample.t_us};
        } else if (sample.pos.x != last_pos_.x ||
                   sample.pos.y != last_pos_.y) {
            last_pos_ = sample.pos;
            last_t_us_ = sample.t_us;
            out[written++] =
                PointerEvent{PointerEventKind::Move, sample.pos, sample.t_us};
        } else {
            last_t_us_ = sample.t_us;  // identical position: refresh liveness
        }
    } else if (touching_) {
        touching_ = false;
        last_t_us_ = sample.t_us;
        out[written++] =
            PointerEvent{PointerEventKind::Up, last_pos_, sample.t_us};
    }
    return written;
}

uint32_t GestureDetector::Feed(const PointerEvent &event, GestureEvent *out) {
    switch (event.kind) {
        case PointerEventKind::Down:
            active_ = true;
            long_press_fired_ = false;
            moved_beyond_tap_ = false;
            down_pos_ = event.pos;
            down_t_us_ = event.t_us;
            last_pos_ = event.pos;
            return 0;
        case PointerEventKind::Move:
            if (!active_) {
                return 0;
            }
            last_pos_ = event.pos;
            if (AbsI32(event.pos.x - down_pos_.x) > config_.tap_max_dist ||
                AbsI32(event.pos.y - down_pos_.y) > config_.tap_max_dist) {
                moved_beyond_tap_ = true;
            }
            return 0;
        case PointerEventKind::Cancel:
            active_ = false;
            return 0;
        case PointerEventKind::Up: {
            if (!active_) {
                return 0;
            }
            active_ = false;
            if (long_press_fired_) {
                return 0;  // long-press already consumed this sequence
            }
            const int32_t dx = event.pos.x - down_pos_.x;
            const int32_t dy = event.pos.y - down_pos_.y;
            const int32_t adx = AbsI32(dx);
            const int32_t ady = AbsI32(dy);
            // Cardinal swipe: main axis long enough, off-axis bounded.
            if (adx >= config_.swipe_min_dist &&
                ady * config_.swipe_axis_ratio_den <=
                    adx * config_.swipe_axis_ratio_num) {
                *out = GestureEvent{dx > 0 ? GestureKind::SwipeRight
                                           : GestureKind::SwipeLeft,
                                    down_pos_, event.t_us};
                return 1;
            }
            if (ady >= config_.swipe_min_dist &&
                adx * config_.swipe_axis_ratio_den <=
                    ady * config_.swipe_axis_ratio_num) {
                *out = GestureEvent{dy > 0 ? GestureKind::SwipeDown
                                           : GestureKind::SwipeUp,
                                    down_pos_, event.t_us};
                return 1;
            }
            // The Up position must also be within tap distance: a trace
            // with no Move samples (coarse polling) must not tap-on-drag.
            if (!moved_beyond_tap_ && adx <= config_.tap_max_dist &&
                ady <= config_.tap_max_dist &&
                event.t_us - down_t_us_ <= config_.tap_max_us) {
                *out = GestureEvent{GestureKind::Tap, down_pos_, event.t_us};
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

uint32_t GestureDetector::Update(int64_t now_us, GestureEvent *out) {
    if (!active_ || long_press_fired_ || moved_beyond_tap_) {
        return 0;
    }
    if (now_us - down_t_us_ >= config_.long_press_us) {
        long_press_fired_ = true;
        *out = GestureEvent{GestureKind::LongPress, down_pos_, now_us};
        return 1;
    }
    return 0;
}

Result<uint32_t> HitTest(const HitRegion *regions, uint32_t count,
                         const Point &p) {
    bool found = false;
    uint32_t best_id = 0;
    int16_t best_z = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (!Contains(regions[i].rect, p)) {
            continue;
        }
        // Later entries win ties, so >= keeps paint order deterministic.
        if (!found || regions[i].z >= best_z) {
            found = true;
            best_z = regions[i].z;
            best_id = regions[i].region_id;
        }
    }
    if (!found) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    return Result<uint32_t>::Ok(best_id);
}

Status Scheduler::Add(uint32_t deadline_id, int64_t due_us) {
    for (uint32_t i = 0; i < count_; ++i) {
        if (entries_[i].id == deadline_id) {
            entries_[i].due_us = due_us;  // replace = reschedule
            return OkStatus();
        }
    }
    if (count_ >= kCapacity) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    entries_[count_++] = Entry{deadline_id, due_us};
    return OkStatus();
}

Status Scheduler::Cancel(uint32_t deadline_id) {
    for (uint32_t i = 0; i < count_; ++i) {
        if (entries_[i].id == deadline_id) {
            entries_[i] = entries_[--count_];
            return OkStatus();
        }
    }
    return ErrStatus(StatusCode::InvalidArgument);
}

Result<int64_t> Scheduler::NextDue() const {
    if (count_ == 0) {
        return Result<int64_t>::Err(StatusCode::InvalidArgument);
    }
    int64_t best = entries_[0].due_us;
    for (uint32_t i = 1; i < count_; ++i) {
        if (entries_[i].due_us < best) {
            best = entries_[i].due_us;
        }
    }
    return Result<int64_t>::Ok(best);
}

Result<uint32_t> Scheduler::PopDue(int64_t now_us) {
    int32_t best_index = -1;
    for (uint32_t i = 0; i < count_; ++i) {
        if (entries_[i].due_us > now_us) {
            continue;
        }
        if (best_index < 0 ||
            entries_[i].due_us < entries_[best_index].due_us ||
            (entries_[i].due_us == entries_[best_index].due_us &&
             entries_[i].id < entries_[best_index].id)) {
            best_index = static_cast<int32_t>(i);
        }
    }
    if (best_index < 0) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    const uint32_t id = entries_[best_index].id;
    entries_[best_index] = entries_[--count_];
    return Result<uint32_t>::Ok(id);
}

}  // namespace s3paper
