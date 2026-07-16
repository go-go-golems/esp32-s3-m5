// FrameBuilder: turns widget/layout drawing intents into a frozen, clipped,
// flat DrawOp frame.
//
// Every emit clips against the current clip stack; fully-clipped ops are
// dropped and counted. Op storage and payload arena are caller-provided and
// fixed; overflow is an explicit CapacityExceeded.
#pragma once

#include <stdint.h>

#include "s3paper/draw_ops.h"
#include "s3paper/frame_arena.h"
#include "s3paper/geometry.h"
#include "s3paper/status.h"

namespace s3paper {

class FrameBuilder {
  public:
    static constexpr uint32_t kMaxClipDepth = 8;

    FrameBuilder(DrawOp *ops, uint32_t op_capacity, FrameArena *arena,
                 Size viewport);

    // Begin a new frame: clears ops, damage, counters, and the arena.
    void Begin();

    // Clip stack. Push intersects with the current clip; an empty
    // intersection is legal (ops are dropped until pop).
    Status PushClip(const Rect &r);
    Status PopClip();
    Rect CurrentClip() const;

    Status FillRect(const Rect &r, Gray8 gray);
    Status StrokeRect(const Rect &r, Gray8 gray, int32_t thickness);
    Status HLine(int32_t x, int32_t y, int32_t w, Gray8 gray);
    Status VLine(int32_t x, int32_t y, int32_t h, Gray8 gray);
    // Arbitrary-angle segment; endpoints are true geometry (rasterizers
    // honor op.clip), bounds is the clipped bbox for damage.
    Status Line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Gray8 gray,
                int32_t thickness = 1);
    // Filled disc.
    Status Circle(int32_t cx, int32_t cy, int32_t r, Gray8 gray);
    // Circle outline, `thickness` px inward from radius r.
    Status Ring(int32_t cx, int32_t cy, int32_t r, Gray8 gray,
                int32_t thickness);
    // Copies text into the arena; the DrawOp references it by offset.
    Status GlyphRun(const Rect &bounds, int32_t baseline_y, uint8_t font_id,
                    uint8_t size_px, const char *text, uint32_t text_len,
                    Gray8 gray);

    // Freezes the frame. The returned frame stays valid until Begin() or the
    // underlying storage is reset. A dangling clip push is CorruptData.
    Result<RenderFrame> Finish(FrameId id);

    uint32_t ops_used() const { return op_count_; }
    uint32_t ops_dropped_clipped() const { return dropped_clipped_; }

  private:
    Status Emit(const DrawOp &op);

    DrawOp *ops_;
    uint32_t op_capacity_;
    uint32_t op_count_ = 0;
    FrameArena *arena_;
    Size viewport_;
    Rect clip_stack_[kMaxClipDepth];
    uint32_t clip_depth_ = 1;  // slot 0 is the viewport clip
    Rect damage_ = kEmptyRect;
    uint32_t dropped_clipped_ = 0;
};

}  // namespace s3paper
