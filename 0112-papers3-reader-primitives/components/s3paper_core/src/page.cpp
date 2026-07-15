#include "s3paper/page.h"

#include <cstring>

namespace s3paper {

void PageRouter::Reset() {
    page_count_ = 0;
    stack_depth_ = 0;
}

Result<PageId> PageRouter::Register(const char *name,
                                    const PageSlots &slots) {
    if (name == nullptr) {
        return Result<PageId>::Err(StatusCode::InvalidArgument);
    }
    if (page_count_ >= kMaxPages) {
        return Result<PageId>::Err(StatusCode::CapacityExceeded);
    }
    Page &p = pages_[page_count_];
    std::strncpy(p.name, name, kNameCapacity - 1);
    p.name[kNameCapacity - 1] = '\0';
    p.slots = slots;
    return Result<PageId>::Ok(static_cast<PageId>(page_count_++));
}

Status PageRouter::SetSlots(PageId id, const PageSlots &slots) {
    if (id >= page_count_) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    pages_[id].slots = slots;
    return OkStatus();
}

Status PageRouter::Push(PageId id) {
    if (id >= page_count_) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (stack_depth_ > 0 && stack_[stack_depth_ - 1] == id) {
        return OkStatus();
    }
    if (stack_depth_ >= kMaxStack) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    stack_[stack_depth_++] = id;
    return OkStatus();
}

Result<PageId> PageRouter::Back() {
    if (stack_depth_ <= 1) {
        return Result<PageId>::Err(StatusCode::InvalidArgument);
    }
    stack_depth_--;
    return Result<PageId>::Ok(stack_[stack_depth_ - 1]);
}

Result<PageId> PageRouter::Current() const {
    if (stack_depth_ == 0) {
        return Result<PageId>::Err(StatusCode::InvalidArgument);
    }
    return Result<PageId>::Ok(stack_[stack_depth_ - 1]);
}

const PageSlots *PageRouter::Slots(PageId id) const {
    return id < page_count_ ? &pages_[id].slots : nullptr;
}

const char *PageRouter::Name(PageId id) const {
    return id < page_count_ ? pages_[id].name : "";
}

namespace {

// Appends one slot's layout, fixing up parent indices to the shared array.
Status AppendSlot(const WidgetArena &arena, WidgetHandle root,
                  const Rect &bounds, LayoutEntry *out, uint32_t cap,
                  uint32_t *count) {
    if (IsNull(root)) {
        return OkStatus();
    }
    const uint32_t offset = *count;
    if (offset >= cap) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    const Result<uint32_t> laid =
        LayoutTree(arena, root, bounds, out + offset, cap - offset);
    if (!laid.ok()) {
        return ErrStatus(laid.code);
    }
    for (uint32_t i = 0; i < laid.value; ++i) {
        LayoutEntry &e = out[offset + i];
        if (e.parent_index != kNoWidgetIndex) {
            e.parent_index = static_cast<uint16_t>(e.parent_index + offset);
        }
    }
    *count += laid.value;
    return OkStatus();
}

}  // namespace

Result<uint32_t> LayoutPage(const WidgetArena &arena, const PageSlots &slots,
                            const Rect &bounds, LayoutEntry *out,
                            uint32_t cap) {
    if (out == nullptr) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    int32_t header_h = 0;
    int32_t footer_h = 0;
    if (!IsNull(slots.header)) {
        const Result<Size> m = MeasureWidget(arena, slots.header, false);
        if (!m.ok()) {
            return Result<uint32_t>::Err(m.code);
        }
        header_h = m.value.h;
    }
    if (!IsNull(slots.footer)) {
        const Result<Size> m = MeasureWidget(arena, slots.footer, false);
        if (!m.ok()) {
            return Result<uint32_t>::Err(m.code);
        }
        footer_h = m.value.h;
    }
    int32_t content_h = bounds.h - header_h - footer_h;
    if (content_h < 0) {
        content_h = 0;
    }
    uint32_t count = 0;
    Status s = AppendSlot(arena, slots.header,
                          Rect{bounds.x, bounds.y, bounds.w, header_h}, out,
                          cap, &count);
    if (!s.ok()) {
        return Result<uint32_t>::Err(s.code);
    }
    s = AppendSlot(arena, slots.content,
                   Rect{bounds.x, bounds.y + header_h, bounds.w, content_h},
                   out, cap, &count);
    if (!s.ok()) {
        return Result<uint32_t>::Err(s.code);
    }
    s = AppendSlot(arena, slots.footer,
                   Rect{bounds.x, bounds.y + bounds.h - footer_h, bounds.w,
                        footer_h},
                   out, cap, &count);
    if (!s.ok()) {
        return Result<uint32_t>::Err(s.code);
    }
    s = AppendSlot(arena, slots.overlay, bounds, out, cap, &count);
    if (!s.ok()) {
        return Result<uint32_t>::Err(s.code);
    }
    return Result<uint32_t>::Ok(count);
}

}  // namespace s3paper
