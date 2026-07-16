#include "s3paper/frame_builder.h"

namespace s3paper {

FrameBuilder::FrameBuilder(DrawOp *ops, uint32_t op_capacity,
                           FrameArena *arena, Size viewport)
    : ops_(ops), op_capacity_(ops ? op_capacity : 0), arena_(arena),
      viewport_(viewport) {
    clip_stack_[0] = Rect{0, 0, viewport.w, viewport.h};
}

void FrameBuilder::Begin() {
    op_count_ = 0;
    dropped_clipped_ = 0;
    damage_ = kEmptyRect;
    clip_depth_ = 1;
    clip_stack_[0] = Rect{0, 0, viewport_.w, viewport_.h};
    if (arena_ != nullptr) {
        arena_->Reset();
    }
}

Status FrameBuilder::PushClip(const Rect &r) {
    if (clip_depth_ >= kMaxClipDepth) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    const Result<Rect> next = Intersect(CurrentClip(), r);
    if (!next.ok()) {
        return ErrStatus(next.code);
    }
    clip_stack_[clip_depth_++] = next.value;
    return OkStatus();
}

Status FrameBuilder::PopClip() {
    if (clip_depth_ <= 1) {
        // Popping the implicit viewport clip is a caller bug.
        return ErrStatus(StatusCode::InvalidArgument);
    }
    clip_depth_--;
    return OkStatus();
}

Rect FrameBuilder::CurrentClip() const { return clip_stack_[clip_depth_ - 1]; }

Status FrameBuilder::Emit(const DrawOp &op) {
    if (op_count_ >= op_capacity_) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    const Result<Rect> u = Union(damage_, op.bounds);
    if (!u.ok()) {
        return ErrStatus(u.code);
    }
    damage_ = u.value;
    ops_[op_count_++] = op;
    return OkStatus();
}

Status FrameBuilder::FillRect(const Rect &r, Gray8 gray) {
    const Result<Rect> clipped = Intersect(r, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    DrawOp op{};
    op.kind = DrawOpKind::FillRect;
    op.gray = gray;
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    return Emit(op);
}

Status FrameBuilder::StrokeRect(const Rect &r, Gray8 gray, int32_t thickness) {
    if (thickness <= 0) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const Result<Rect> clipped = Intersect(r, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    DrawOp op{};
    op.kind = DrawOpKind::StrokeRect;
    op.gray = gray;
    // Stroke geometry is the unclipped rect (the outline position matters);
    // rendering must honor op.clip. bounds still records the clipped extent
    // for damage purposes.
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    op.payload.stroke.thickness = thickness;
    return Emit(op);
}

Status FrameBuilder::HLine(int32_t x, int32_t y, int32_t w, Gray8 gray) {
    const Result<Rect> clipped = Intersect(Rect{x, y, w, 1}, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    DrawOp op{};
    op.kind = DrawOpKind::HLine;
    op.gray = gray;
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    return Emit(op);
}

Status FrameBuilder::VLine(int32_t x, int32_t y, int32_t h, Gray8 gray) {
    const Result<Rect> clipped = Intersect(Rect{x, y, 1, h}, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    DrawOp op{};
    op.kind = DrawOpKind::VLine;
    op.gray = gray;
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    return Emit(op);
}

Status FrameBuilder::GlyphRun(const Rect &bounds, int32_t baseline_y,
                              uint8_t font_id, uint8_t size_px,
                              const char *text, uint32_t text_len,
                              Gray8 gray) {
    if (text == nullptr && text_len > 0) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (arena_ == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const Result<Rect> clipped = Intersect(bounds, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    const Result<uint32_t> stored = arena_->PushBytes(text, text_len, 1);
    if (!stored.ok()) {
        return ErrStatus(stored.code);
    }
    DrawOp op{};
    op.kind = DrawOpKind::GlyphRun;
    op.gray = gray;
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    op.payload.glyph_run.text_offset = stored.value;
    op.payload.glyph_run.text_len = text_len;
    op.payload.glyph_run.baseline_y = baseline_y;
    op.payload.glyph_run.font_id = font_id;
    op.payload.glyph_run.size_px = size_px;
    return Emit(op);
}

Result<RenderFrame> FrameBuilder::Finish(FrameId id) {
    if (clip_depth_ != 1) {
        // Unbalanced push/pop means the emitting code lost track of state.
        return Result<RenderFrame>::Err(StatusCode::CorruptData);
    }
    RenderFrame frame{};
    frame.ops = ops_;
    frame.op_count = op_count_;
    frame.arena = arena_;
    frame.damage = damage_;
    frame.viewport = viewport_;
    frame.id = id;
    return Result<RenderFrame>::Ok(frame);
}

Status FrameBuilder::Line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                          Gray8 gray, int32_t thickness) {
    if (thickness <= 0) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    // Conservative bbox: endpoint extent inflated by the thickness so
    // thick diagonal edges never escape the damage rect.
    const int32_t min_x = x0 < x1 ? x0 : x1;
    const int32_t min_y = y0 < y1 ? y0 : y1;
    const int32_t max_x = x0 > x1 ? x0 : x1;
    const int32_t max_y = y0 > y1 ? y0 : y1;
    const Rect bbox{min_x - thickness / 2, min_y - thickness / 2,
                    (max_x - min_x) + thickness + 1,
                    (max_y - min_y) + thickness + 1};
    const Result<Rect> clipped = Intersect(bbox, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    DrawOp op{};
    op.kind = DrawOpKind::Line;
    op.gray = gray;
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    op.payload.line = LinePayload{x0, y0, x1, y1, thickness};
    return Emit(op);
}

Status FrameBuilder::Circle(int32_t cx, int32_t cy, int32_t r, Gray8 gray) {
    return Ring(cx, cy, r, gray, 0);
}

Status FrameBuilder::Ring(int32_t cx, int32_t cy, int32_t r, Gray8 gray,
                          int32_t thickness) {
    if (r < 0 || thickness < 0) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const int32_t t = thickness > r ? r : thickness;  // clamp: ring -> disc
    const Rect bbox{cx - r, cy - r, 2 * r + 1, 2 * r + 1};
    const Result<Rect> clipped = Intersect(bbox, CurrentClip());
    if (!clipped.ok()) {
        return ErrStatus(clipped.code);
    }
    if (IsEmpty(clipped.value)) {
        dropped_clipped_++;
        return OkStatus();
    }
    DrawOp op{};
    op.kind = DrawOpKind::Circle;
    op.gray = gray;
    op.bounds = clipped.value;
    op.clip = CurrentClip();
    op.payload.circle = CirclePayload{cx, cy, r, t};
    return Emit(op);
}

const char *DrawOpKindName(DrawOpKind kind) {
    switch (kind) {
        case DrawOpKind::FillRect: return "FillRect";
        case DrawOpKind::StrokeRect: return "StrokeRect";
        case DrawOpKind::HLine: return "HLine";
        case DrawOpKind::VLine: return "VLine";
        case DrawOpKind::GlyphRun: return "GlyphRun";
        case DrawOpKind::Bitmap: return "Bitmap";
        case DrawOpKind::Line: return "Line";
        case DrawOpKind::Circle: return "Circle";
    }
    return "Unknown";
}

}  // namespace s3paper
