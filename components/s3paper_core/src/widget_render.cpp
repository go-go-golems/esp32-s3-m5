#include "s3paper/widget_render.h"

#include <cstring>

#include "s3paper/text.h"

namespace s3paper {
namespace {

// Baseline and x-position for a Text node inside its frame.
void TextPlacement(const WidgetNode &n, const Rect &frame, int32_t *out_x,
                   int32_t *out_baseline) {
    const uint8_t font = n.props.text.font_id;
    int32_t ascent = 0;
    int32_t line_height = frame.h;
    const Result<FontLineMetrics> m = GetFontLineMetrics(font);
    if (m.ok()) {
        ascent = m.value.ascent;
        line_height = m.value.line_height;
    }
    *out_baseline = frame.y + (frame.h - line_height) / 2 + ascent;
    int32_t x = frame.x;
    if (n.props.text.align != TextAlign::Start) {
        const uint32_t len =
            static_cast<uint32_t>(std::strlen(n.props.text.value));
        const Result<int32_t> w = MeasureText(font, n.props.text.value, len);
        const int32_t text_w = w.ok() ? w.value : 0;
        if (n.props.text.align == TextAlign::Center) {
            x += (frame.w - text_w) / 2;
        } else {
            x += frame.w - text_w;
        }
    }
    *out_x = x;
}

Status EmitNode(const WidgetArena &arena, const WidgetNode &n,
                const Rect &frame, FrameBuilder &fb) {
    switch (n.kind) {
        case WidgetKind::Text: {
            const uint32_t len =
                static_cast<uint32_t>(std::strlen(n.props.text.value));
            if (len == 0) {
                return OkStatus();
            }
            int32_t x = 0;
            int32_t baseline = 0;
            TextPlacement(n, frame, &x, &baseline);
            const Rect bounds{x, frame.y, frame.w - (x - frame.x), frame.h};
            if (n.props.text.invert != 0) {
                // Filled chip: background in the node's gray, glyphs in
                // the inverse so default black text becomes white-on-black.
                const Status bg = fb.FillRect(frame, n.props.text.gray);
                if (!bg.ok()) {
                    return bg;
                }
                const Gray8 ink = static_cast<Gray8>(
                    255 - static_cast<int32_t>(n.props.text.gray));
                return fb.GlyphRun(bounds, baseline, n.props.text.font_id,
                                   0, n.props.text.value, len, ink);
            }
            return fb.GlyphRun(bounds, baseline, n.props.text.font_id, 0,
                               n.props.text.value, len, n.props.text.gray);
        }
        case WidgetKind::Divider:
            return fb.FillRect(frame, n.props.divider.gray);
        case WidgetKind::Progress: {
            const Status border = fb.StrokeRect(frame, n.props.progress.gray, 1);
            if (!border.ok()) {
                return border;
            }
            const Rect inner{frame.x + 2, frame.y + 2, frame.w - 4,
                             frame.h - 4};
            if (inner.w <= 0 || inner.h <= 0) {
                return OkStatus();
            }
            const int32_t fill_w = static_cast<int32_t>(
                static_cast<int64_t>(inner.w) * n.props.progress.permille /
                1000);
            if (fill_w <= 0) {
                return OkStatus();
            }
            return fb.FillRect(Rect{inner.x, inner.y, fill_w, inner.h},
                               n.props.progress.gray);
        }
        case WidgetKind::Canvas: {
            // Commands are canvas-relative; confine them to the frame so
            // freehand geometry can never escape its widget box. A
            // disjoint frame/clip pair means nothing to draw.
            if (!fb.PushClip(frame).ok()) {
                return OkStatus();
            }
            uint32_t count = 0;
            const CanvasCmd *cmds = arena.CanvasCmds(n, &count);
            Status st = OkStatus();
            for (uint32_t i = 0; st.ok() && i < count; ++i) {
                const CanvasCmd &c = cmds[i];
                const int32_t x = frame.x + c.a;
                const int32_t y = frame.y + c.b;
                switch (c.kind) {
                    case CanvasCmd::kFill:
                        st = fb.FillRect(Rect{x, y, c.c, c.d}, c.gray);
                        break;
                    case CanvasCmd::kBox:
                        st = fb.StrokeRect(Rect{x, y, c.c, c.d}, c.gray,
                                           c.thickness);
                        break;
                    case CanvasCmd::kLine:
                        st = fb.Line(x, y, frame.x + c.c, frame.y + c.d,
                                     c.gray, c.thickness);
                        break;
                    case CanvasCmd::kDisc:
                        st = fb.Circle(x, y, c.c, c.gray);
                        break;
                    case CanvasCmd::kRing:
                        st = fb.Ring(x, y, c.c, c.gray, c.thickness);
                        break;
                    default:
                        break;
                }
            }
            const Status popped = fb.PopClip();
            return st.ok() ? popped : st;
        }
        default:
            return OkStatus();  // containers, spacers, book, region
    }
}

}  // namespace

Result<CompileResult> CompileTree(const WidgetArena &arena,
                                  const LayoutEntry *entries, uint32_t count,
                                  FrameBuilder &fb, HitRegion *hits,
                                  uint32_t hit_cap, RegionSpec *regions,
                                  uint32_t region_cap) {
    if (entries == nullptr || count > WidgetArena::kCapacity) {
        return Result<CompileResult>::Err(StatusCode::InvalidArgument);
    }
    // Effective clip per entry: intersection of all ancestor frames.
    Rect clips[WidgetArena::kCapacity];
    CompileResult result{0, 0};
    for (uint32_t i = 0; i < count; ++i) {
        const LayoutEntry &e = entries[i];
        const WidgetNode *n = arena.Get(e.widget);
        if (n == nullptr) {
            return Result<CompileResult>::Err(StatusCode::InvalidArgument);
        }
        if (e.parent_index == kNoWidgetIndex) {
            clips[i] = e.frame;
        } else {
            const Result<Rect> c =
                Intersect(clips[e.parent_index],
                          entries[e.parent_index].frame);
            clips[i] = c.ok() ? c.value : kEmptyRect;
        }
        if (IsEmpty(clips[i])) {
            continue;  // fully clipped subtree member
        }

        Status s = fb.PushClip(clips[i]);
        if (!s.ok()) {
            return Result<CompileResult>::Err(s.code);
        }
        s = EmitNode(arena, *n, e.frame, fb);
        const Status popped = fb.PopClip();
        if (!s.ok()) {
            return Result<CompileResult>::Err(s.code);
        }
        if (!popped.ok()) {
            return Result<CompileResult>::Err(popped.code);
        }

        if (n->hit_id != 0) {
            const Result<Rect> visible = Intersect(e.frame, clips[i]);
            if (visible.ok() && !IsEmpty(visible.value)) {
                if (result.hit_count >= hit_cap || hits == nullptr) {
                    return Result<CompileResult>::Err(
                        StatusCode::CapacityExceeded);
                }
                hits[result.hit_count++] =
                    HitRegion{visible.value, n->hit_id, n->hit_z};
            }
        }
        if (n->kind == WidgetKind::Region) {
            if (result.region_count >= region_cap || regions == nullptr) {
                return Result<CompileResult>::Err(
                    StatusCode::CapacityExceeded);
            }
            regions[result.region_count++] =
                RegionSpec{n->props.region.region_id, e.frame, n->dependency,
                           n->props.region.interval_ms,
                           n->props.region.quiet_while_active};
        }
    }
    return Result<CompileResult>::Ok(result);
}

Result<Rect> FindFrame(const LayoutEntry *entries, uint32_t count,
                       WidgetHandle handle) {
    for (uint32_t i = 0; i < count; ++i) {
        if (entries[i].widget.index == handle.index &&
            entries[i].widget.generation == handle.generation) {
            return Result<Rect>::Ok(entries[i].frame);
        }
    }
    return Result<Rect>::Err(StatusCode::InvalidArgument);
}

}  // namespace s3paper
