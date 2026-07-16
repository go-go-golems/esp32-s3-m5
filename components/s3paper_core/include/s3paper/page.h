// Routable pages (Phase 9, design §5): a page is a full-screen unit with
// header/content/footer/overlay slots and a bounded navigation stack.
//
// Refresh policy contract: every successful Push/Back is a screen change —
// the caller presents the next frame with Intent::ScreenChange (clean full)
// exactly like the Phase 8 reader does for library<->reading transitions.
// The router only routes; it never draws and never owns widget lifetimes.
#pragma once

#include <stdint.h>

#include "s3paper/status.h"
#include "s3paper/widget.h"
#include "s3paper/widget_layout.h"

namespace s3paper {

using PageId = uint16_t;
constexpr PageId kNoPage = 0xFFFF;

// Any slot may be null. Overlay is laid out over the full page bounds and
// drawn last (declare hit_z on overlay widgets to win hit testing).
struct PageSlots {
    WidgetHandle header;
    WidgetHandle content;
    WidgetHandle footer;
    WidgetHandle overlay;
};

class PageRouter {
  public:
    static constexpr uint32_t kMaxPages = 8;
    static constexpr uint32_t kMaxStack = 8;
    static constexpr uint32_t kNameCapacity = 16;

    void Reset();

    // Registers a named page. The name is copied and truncated to fit.
    Result<PageId> Register(const char *name, const PageSlots &slots);
    Status SetSlots(PageId id, const PageSlots &slots);

    // Push makes `id` current (stack grows; CapacityExceeded when full).
    // Pushing the current page again is Ok and does not grow the stack.
    Status Push(PageId id);
    // Pops to the previous page; InvalidArgument at the stack bottom.
    Result<PageId> Back();

    Result<PageId> Current() const;
    const PageSlots *Slots(PageId id) const;
    const char *Name(PageId id) const;  // "" for unknown ids
    uint32_t stack_depth() const { return stack_depth_; }

  private:
    struct Page {
        char name[kNameCapacity];
        PageSlots slots;
    };
    Page pages_[kMaxPages];
    uint32_t page_count_ = 0;
    PageId stack_[kMaxStack];
    uint32_t stack_depth_ = 0;
};

// Lays out all non-null slots of a page into `bounds`: header at its
// intrinsic height on top, footer at its intrinsic height at the bottom,
// content filling the space between, overlay over the full bounds (last =
// painted on top). Entries from all slots share one array; parent indices
// are adjusted to it.
Result<uint32_t> LayoutPage(const WidgetArena &arena, const PageSlots &slots,
                            const Rect &bounds, LayoutEntry *out,
                            uint32_t cap);

}  // namespace s3paper
