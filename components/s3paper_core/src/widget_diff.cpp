#include "s3paper/widget_diff.h"

#include <cstring>

namespace s3paper {

void DependencyTracker::Clear() { count_ = 0; }

Status DependencyTracker::MarkDirty(DependencyId id) {
    if (id == 0) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (IsDirty(id)) {
        return OkStatus();
    }
    if (count_ >= kCapacity) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    dirty_[count_++] = id;
    return OkStatus();
}

bool DependencyTracker::IsDirty(DependencyId id) const {
    for (uint32_t i = 0; i < count_; ++i) {
        if (dirty_[i] == id) {
            return true;
        }
    }
    return false;
}

uint32_t DependencyTracker::CollectDirtyWidgets(const WidgetArena &arena,
                                                WidgetHandle *out,
                                                uint32_t cap) const {
    uint32_t written = 0;
    for (uint16_t i = 0; i < WidgetArena::kCapacity && written < cap; ++i) {
        const WidgetNode *n = arena.At(i);
        if (n == nullptr || n->dependency == 0 || !IsDirty(n->dependency)) {
            continue;
        }
        out[written++] = WidgetHandle{i, n->generation};
    }
    return written;
}

void RenderStateDiff::Reset() { std::memset(slots_, 0, sizeof(slots_)); }

namespace {

// Builds the per-arena-slot view of a layout (last entry wins; layout never
// emits a node twice, so this is 1:1).
struct CurrentSlot {
    bool present;
    uint16_t generation;
    uint32_t version;
    WidgetKind kind;
    Rect frame;
};

void BuildCurrent(const WidgetArena &arena, const LayoutEntry *entries,
                  uint32_t count, CurrentSlot *slots) {
    std::memset(slots, 0,
                sizeof(CurrentSlot) * WidgetArena::kCapacity);
    for (uint32_t i = 0; i < count; ++i) {
        const WidgetNode *n = arena.Get(entries[i].widget);
        if (n == nullptr) {
            continue;
        }
        CurrentSlot &s = slots[entries[i].widget.index];
        s.present = true;
        s.generation = entries[i].widget.generation;
        s.version = n->content_version;
        s.kind = n->kind;
        s.frame = entries[i].frame;
    }
}

}  // namespace

void RenderStateDiff::Capture(const WidgetArena &arena,
                              const LayoutEntry *entries, uint32_t count) {
    CurrentSlot current[WidgetArena::kCapacity];
    BuildCurrent(arena, entries, count, current);
    for (uint32_t i = 0; i < WidgetArena::kCapacity; ++i) {
        slots_[i] = Slot{current[i].present, current[i].generation,
                         current[i].version, current[i].kind,
                         current[i].frame};
    }
}

Result<uint32_t> RenderStateDiff::Diff(const WidgetArena &arena,
                                       const LayoutEntry *entries,
                                       uint32_t count, Rect *out,
                                       uint32_t cap) const {
    if (entries == nullptr && count > 0) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    CurrentSlot current[WidgetArena::kCapacity];
    BuildCurrent(arena, entries, count, current);
    uint32_t written = 0;
    for (uint32_t i = 0; i < WidgetArena::kCapacity; ++i) {
        const Slot &old_slot = slots_[i];
        const CurrentSlot &now = current[i];
        Rect damage[2];
        uint32_t rects = 0;
        if (old_slot.present && now.present) {
            const bool changed = old_slot.generation != now.generation ||
                                 old_slot.kind != now.kind ||
                                 old_slot.version != now.version ||
                                 !RectEquals(old_slot.frame, now.frame);
            if (changed) {
                damage[rects++] = old_slot.frame;
                if (!RectEquals(old_slot.frame, now.frame)) {
                    damage[rects++] = now.frame;
                }
            }
        } else if (old_slot.present) {
            damage[rects++] = old_slot.frame;
        } else if (now.present) {
            damage[rects++] = now.frame;
        }
        for (uint32_t d = 0; d < rects; ++d) {
            if (IsEmpty(damage[d])) {
                continue;
            }
            if (written >= cap || out == nullptr) {
                return Result<uint32_t>::Err(StatusCode::CapacityExceeded);
            }
            out[written++] = damage[d];
        }
    }
    return Result<uint32_t>::Ok(written);
}

}  // namespace s3paper
