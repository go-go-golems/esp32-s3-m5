#include "s3paper/widget_layout.h"

#include <cstring>

#include "s3paper/text.h"

namespace s3paper {
namespace {

struct Ctx {
    const WidgetArena *arena;
    LayoutEntry *out;
    uint32_t cap;
    uint32_t count;
};

bool IsAxisContainer(WidgetKind k) {
    return k == WidgetKind::Row || k == WidgetKind::Col ||
           k == WidgetKind::List;
}

int32_t TextIntrinsicWidth(const WidgetNode &n) {
    const uint32_t len =
        static_cast<uint32_t>(std::strlen(n.props.text.value));
    const Result<int32_t> w =
        MeasureText(n.props.text.font_id, n.props.text.value, len);
    return w.ok() ? w.value : 0;
}

int32_t TextIntrinsicHeight(const WidgetNode &n) {
    const Result<FontLineMetrics> m =
        GetFontLineMetrics(n.props.text.font_id);
    return m.ok() ? m.value.line_height : 0;
}

// Intrinsic size, interpreted through the parent's axis for the leaves
// whose meaning depends on it (Spacer occupies main, Divider spans cross).
Size Measure(const WidgetArena &arena, uint16_t index, bool parent_horizontal,
             uint32_t depth);

Size MeasureContainer(const WidgetArena &arena, const WidgetNode &n,
                      uint32_t depth) {
    const bool horizontal = n.kind == WidgetKind::Row;
    int64_t main = 0;
    int32_t cross = 0;
    uint32_t laid = 0;
    uint16_t c = n.first_child;
    while (c != kNoWidgetIndex) {
        const WidgetNode *child = arena.At(c);
        if (child == nullptr) {
            break;  // dangling child link (destroyed without RemoveChild)
        }
        const Size cs = Measure(arena, c, horizontal, depth + 1);
        const int32_t fixed_main = horizontal ? child->fixed_w
                                              : child->fixed_h;
        const int32_t fixed_cross = horizontal ? child->fixed_h
                                               : child->fixed_w;
        const int32_t cm = fixed_main >= 0
                               ? fixed_main
                               : (horizontal ? cs.w : cs.h);
        const int32_t cc = fixed_cross >= 0
                               ? fixed_cross
                               : (horizontal ? cs.h : cs.w);
        main += cm;
        if (cc > cross) {
            cross = cc;
        }
        laid++;
        c = child->next_sibling;
    }
    if (laid > 1) {
        main += static_cast<int64_t>(n.gap) * (laid - 1);
    }
    const int32_t pad_main =
        horizontal ? n.padding.left + n.padding.right
                   : n.padding.top + n.padding.bottom;
    const int32_t pad_cross =
        horizontal ? n.padding.top + n.padding.bottom
                   : n.padding.left + n.padding.right;
    const int32_t main32 = static_cast<int32_t>(main) + pad_main;
    const int32_t cross32 = cross + pad_cross;
    return horizontal ? Size{main32, cross32} : Size{cross32, main32};
}

Size Measure(const WidgetArena &arena, uint16_t index, bool parent_horizontal,
             uint32_t depth) {
    const WidgetNode *n = arena.At(index);
    if (n == nullptr || depth > kMaxLayoutDepth) {
        return Size{0, 0};
    }
    switch (n->kind) {
        case WidgetKind::Text:
            return Size{TextIntrinsicWidth(*n), TextIntrinsicHeight(*n)};
        case WidgetKind::Spacer:
            return parent_horizontal ? Size{n->props.spacer.fixed, 0}
                                     : Size{0, n->props.spacer.fixed};
        case WidgetKind::Divider:
            return parent_horizontal ? Size{n->props.divider.thickness, 0}
                                     : Size{0, n->props.divider.thickness};
        case WidgetKind::Progress:
            return Size{0, n->props.progress.height};
        case WidgetKind::Book:
            return Size{0, 0};
        case WidgetKind::Canvas:
            // No intrinsic content size: size a canvas with fixed_w/h or
            // flex, exactly like Book (an unsized canvas collapses).
            return Size{0, 0};
        case WidgetKind::Region: {
            if (n->first_child == kNoWidgetIndex) {
                return Size{0, 0};
            }
            const Size cs =
                Measure(arena, n->first_child, parent_horizontal, depth + 1);
            return Size{cs.w + n->padding.left + n->padding.right,
                        cs.h + n->padding.top + n->padding.bottom};
        }
        case WidgetKind::Row:
        case WidgetKind::Col:
        case WidgetKind::List:
            return MeasureContainer(arena, *n, depth);
    }
    return Size{0, 0};
}

Status Arrange(Ctx &ctx, uint16_t node_index, uint16_t parent_entry,
               const Rect &frame, uint32_t depth);

Status ArrangeChildren(Ctx &ctx, const WidgetNode &n, uint16_t entry_index,
                       const Rect &frame, uint32_t depth) {
    Rect content = frame;
    const Result<Rect> shrunk = Shrink(frame, n.padding);
    if (shrunk.ok()) {
        content = shrunk.value;
    }
    const bool horizontal = n.kind == WidgetKind::Row;
    const bool is_list = n.kind == WidgetKind::List;
    const int32_t content_main = horizontal ? content.w : content.h;
    const int32_t content_cross = horizontal ? content.h : content.w;

    // Pass 1: fixed/intrinsic main sizes and flex weights.
    int64_t used = 0;
    uint32_t child_count = 0;
    uint32_t flex_total = 0;
    uint16_t skip = is_list ? n.props.list.first_visible : 0;
    uint16_t c = n.first_child;
    while (c != kNoWidgetIndex) {
        const WidgetNode *child = ctx.arena->At(c);
        if (child == nullptr) {
            break;  // dangling child link
        }
        if (skip > 0) {
            skip--;
            c = child->next_sibling;
            continue;
        }
        const int32_t fixed_main = horizontal ? child->fixed_w
                                              : child->fixed_h;
        if (fixed_main >= 0) {
            used += fixed_main;
        } else if (!is_list && child->flex > 0) {
            flex_total += child->flex;
        } else {
            const Size cs = Measure(*ctx.arena, c, horizontal, depth + 1);
            used += horizontal ? cs.w : cs.h;
        }
        child_count++;
        c = child->next_sibling;
    }
    if (child_count == 0) {
        return OkStatus();
    }
    const int64_t gaps = static_cast<int64_t>(n.gap) * (child_count - 1);
    int64_t leftover = content_main - used - gaps;
    if (leftover < 0) {
        leftover = 0;
    }

    // Main-axis start offset and extra between-gap.
    int64_t cursor = horizontal ? content.x : content.y;
    int64_t extra_gap = 0;
    if (flex_total == 0) {
        switch (n.main_align) {
            case MainAlign::Start:
                break;
            case MainAlign::Center:
                cursor += leftover / 2;
                break;
            case MainAlign::End:
                cursor += leftover;
                break;
            case MainAlign::SpaceBetween:
                if (child_count > 1) {
                    extra_gap = leftover / (child_count - 1);
                }
                break;
        }
    }

    // Pass 2: place.
    const int64_t content_end =
        static_cast<int64_t>(horizontal ? content.x + content.w
                                        : content.y + content.h);
    uint32_t flex_left = flex_total;
    int64_t flex_space = leftover;
    uint16_t shown = 0;
    skip = is_list ? n.props.list.first_visible : 0;
    c = n.first_child;
    while (c != kNoWidgetIndex) {
        const WidgetNode *child = ctx.arena->At(c);
        if (child == nullptr) {
            break;  // dangling child link
        }
        if (skip > 0) {
            skip--;
            c = child->next_sibling;
            continue;
        }
        const int32_t fixed_main = horizontal ? child->fixed_w
                                              : child->fixed_h;
        const int32_t fixed_cross = horizontal ? child->fixed_h
                                               : child->fixed_w;
        int32_t main_size;
        if (fixed_main >= 0) {
            main_size = fixed_main;
        } else if (!is_list && child->flex > 0) {
            // Integer flex distribution: last flex child takes the rest.
            const int64_t share =
                flex_left == child->flex
                    ? flex_space
                    : flex_space * child->flex / flex_left;
            main_size = static_cast<int32_t>(share);
            flex_space -= share;
            flex_left -= child->flex;
        } else {
            const Size cs = Measure(*ctx.arena, c, horizontal, depth + 1);
            main_size = horizontal ? cs.w : cs.h;
        }
        int32_t cross_size;
        if (fixed_cross >= 0) {
            cross_size = fixed_cross;
        } else if (n.cross_align == CrossAlign::Stretch) {
            cross_size = content_cross;
        } else {
            const Size cs = Measure(*ctx.arena, c, horizontal, depth + 1);
            cross_size = horizontal ? cs.h : cs.w;
        }
        int64_t cross_pos = horizontal ? content.y : content.x;
        switch (n.cross_align) {
            case CrossAlign::Stretch:
            case CrossAlign::Start:
                break;
            case CrossAlign::Center:
                cross_pos += (content_cross - cross_size) / 2;
                break;
            case CrossAlign::End:
                cross_pos += content_cross - cross_size;
                break;
        }
        if (is_list && cursor + main_size > content_end) {
            break;  // pagination: this child and the rest are not shown
        }
        const Rect child_frame =
            horizontal
                ? Rect{static_cast<int32_t>(cursor),
                       static_cast<int32_t>(cross_pos), main_size, cross_size}
                : Rect{static_cast<int32_t>(cross_pos),
                       static_cast<int32_t>(cursor), cross_size, main_size};
        const Status s = Arrange(ctx, c, entry_index, child_frame, depth + 1);
        if (!s.ok()) {
            return s;
        }
        shown++;
        cursor += main_size + n.gap + extra_gap;
        c = child->next_sibling;
    }
    if (is_list) {
        ctx.out[entry_index].list_shown = shown;
    }
    return OkStatus();
}

Status Arrange(Ctx &ctx, uint16_t node_index, uint16_t parent_entry,
               const Rect &frame, uint32_t depth) {
    if (depth > kMaxLayoutDepth) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    if (ctx.count >= ctx.cap) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    const WidgetNode *n = ctx.arena->At(node_index);
    if (n == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const uint16_t entry_index = static_cast<uint16_t>(ctx.count);
    ctx.out[ctx.count++] =
        LayoutEntry{WidgetHandle{node_index, n->generation}, parent_entry,
                    frame, 0};
    if (IsAxisContainer(n->kind)) {
        return ArrangeChildren(ctx, *n, entry_index, frame, depth);
    }
    if (n->kind == WidgetKind::Region && n->first_child != kNoWidgetIndex) {
        Rect content = frame;
        const Result<Rect> shrunk = Shrink(frame, n->padding);
        if (shrunk.ok()) {
            content = shrunk.value;
        }
        return Arrange(ctx, n->first_child, entry_index, content, depth + 1);
    }
    return OkStatus();
}

}  // namespace

Result<Size> MeasureWidget(const WidgetArena &arena, WidgetHandle handle,
                           bool parent_horizontal) {
    if (arena.Get(handle) == nullptr) {
        return Result<Size>::Err(StatusCode::InvalidArgument);
    }
    return Result<Size>::Ok(
        Measure(arena, handle.index, parent_horizontal, 0));
}

Result<uint32_t> LayoutTree(const WidgetArena &arena, WidgetHandle root,
                            const Rect &bounds, LayoutEntry *out,
                            uint32_t cap) {
    if (out == nullptr || arena.Get(root) == nullptr) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    Ctx ctx{&arena, out, cap, 0};
    const Status s = Arrange(ctx, root.index, kNoWidgetIndex, bounds, 0);
    if (!s.ok()) {
        return Result<uint32_t>::Err(s.code);
    }
    return Result<uint32_t>::Ok(ctx.count);
}

}  // namespace s3paper
