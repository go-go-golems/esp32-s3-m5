#include "s3paper/geometry.h"

namespace s3paper {
namespace {

constexpr int64_t kInt32Min = INT32_MIN;
constexpr int64_t kInt32Max = INT32_MAX;

bool FitsInt32(int64_t v) { return v >= kInt32Min && v <= kInt32Max; }

// Build a rect from int64 edges, checking representability of every field.
Result<Rect> RectFromEdges(int64_t x0, int64_t y0, int64_t x1, int64_t y1) {
    if (x1 <= x0 || y1 <= y0) {
        return Result<Rect>::Ok(kEmptyRect);
    }
    const int64_t w = x1 - x0;
    const int64_t h = y1 - y0;
    if (!FitsInt32(x0) || !FitsInt32(y0) || !FitsInt32(w) || !FitsInt32(h)) {
        return Result<Rect>::Err(StatusCode::InvalidArgument);
    }
    return Result<Rect>::Ok(Rect{static_cast<int32_t>(x0),
                                 static_cast<int32_t>(y0),
                                 static_cast<int32_t>(w),
                                 static_cast<int32_t>(h)});
}

}  // namespace

bool IsEmpty(const Rect &r) { return r.w <= 0 || r.h <= 0; }

bool RectEquals(const Rect &a, const Rect &b) {
    if (IsEmpty(a) && IsEmpty(b)) {
        return true;
    }
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

int64_t Area(const Rect &r) {
    if (IsEmpty(r)) {
        return 0;
    }
    return static_cast<int64_t>(r.w) * static_cast<int64_t>(r.h);
}

Rect Normalized(const Rect &r) { return IsEmpty(r) ? kEmptyRect : r; }

bool Contains(const Rect &r, const Point &p) {
    if (IsEmpty(r)) {
        return false;
    }
    const int64_t x1 = static_cast<int64_t>(r.x) + r.w;
    const int64_t y1 = static_cast<int64_t>(r.y) + r.h;
    return p.x >= r.x && p.y >= r.y && p.x < x1 && p.y < y1;
}

bool ContainsRect(const Rect &outer, const Rect &inner) {
    if (IsEmpty(inner)) {
        return true;
    }
    if (IsEmpty(outer)) {
        return false;
    }
    const int64_t ox1 = static_cast<int64_t>(outer.x) + outer.w;
    const int64_t oy1 = static_cast<int64_t>(outer.y) + outer.h;
    const int64_t ix1 = static_cast<int64_t>(inner.x) + inner.w;
    const int64_t iy1 = static_cast<int64_t>(inner.y) + inner.h;
    return inner.x >= outer.x && inner.y >= outer.y && ix1 <= ox1 &&
           iy1 <= oy1;
}

Result<Rect> Intersect(const Rect &a, const Rect &b) {
    if (IsEmpty(a) || IsEmpty(b)) {
        return Result<Rect>::Ok(kEmptyRect);
    }
    const int64_t x0 =
        (a.x > b.x) ? a.x : b.x;
    const int64_t y0 = (a.y > b.y) ? a.y : b.y;
    const int64_t ax1 = static_cast<int64_t>(a.x) + a.w;
    const int64_t ay1 = static_cast<int64_t>(a.y) + a.h;
    const int64_t bx1 = static_cast<int64_t>(b.x) + b.w;
    const int64_t by1 = static_cast<int64_t>(b.y) + b.h;
    const int64_t x1 = (ax1 < bx1) ? ax1 : bx1;
    const int64_t y1 = (ay1 < by1) ? ay1 : by1;
    return RectFromEdges(x0, y0, x1, y1);
}

Result<Rect> Union(const Rect &a, const Rect &b) {
    if (IsEmpty(a)) {
        return Result<Rect>::Ok(Normalized(b));
    }
    if (IsEmpty(b)) {
        return Result<Rect>::Ok(Normalized(a));
    }
    const int64_t x0 = (a.x < b.x) ? a.x : b.x;
    const int64_t y0 = (a.y < b.y) ? a.y : b.y;
    const int64_t ax1 = static_cast<int64_t>(a.x) + a.w;
    const int64_t ay1 = static_cast<int64_t>(a.y) + a.h;
    const int64_t bx1 = static_cast<int64_t>(b.x) + b.w;
    const int64_t by1 = static_cast<int64_t>(b.y) + b.h;
    const int64_t x1 = (ax1 > bx1) ? ax1 : bx1;
    const int64_t y1 = (ay1 > by1) ? ay1 : by1;
    return RectFromEdges(x0, y0, x1, y1);
}

Result<Rect> ClampTo(const Rect &r, const Size &bounds) {
    return Intersect(r, Rect{0, 0, bounds.w, bounds.h});
}

Result<Rect> Translate(const Rect &r, int32_t dx, int32_t dy) {
    if (IsEmpty(r)) {
        return Result<Rect>::Ok(kEmptyRect);
    }
    const int64_t x0 = static_cast<int64_t>(r.x) + dx;
    const int64_t y0 = static_cast<int64_t>(r.y) + dy;
    return RectFromEdges(x0, y0, x0 + r.w, y0 + r.h);
}

Result<Rect> Shrink(const Rect &r, const Insets &insets) {
    if (IsEmpty(r)) {
        return Result<Rect>::Ok(kEmptyRect);
    }
    const int64_t x0 = static_cast<int64_t>(r.x) + insets.left;
    const int64_t y0 = static_cast<int64_t>(r.y) + insets.top;
    const int64_t x1 =
        static_cast<int64_t>(r.x) + r.w - insets.right;
    const int64_t y1 = static_cast<int64_t>(r.y) + r.h - insets.bottom;
    return RectFromEdges(x0, y0, x1, y1);
}

Result<Rect> RotateInBounds(const Rect &r, const Size &logical_bounds,
                            uint8_t rotation) {
    if (rotation > 3) {
        return Result<Rect>::Err(StatusCode::InvalidArgument);
    }
    if (logical_bounds.w < 0 || logical_bounds.h < 0) {
        return Result<Rect>::Err(StatusCode::InvalidArgument);
    }
    if (IsEmpty(r)) {
        return Result<Rect>::Ok(kEmptyRect);
    }
    const int64_t W = logical_bounds.w;
    const int64_t H = logical_bounds.h;
    const int64_t x0 = r.x;
    const int64_t y0 = r.y;
    const int64_t x1 = static_cast<int64_t>(r.x) + r.w;
    const int64_t y1 = static_cast<int64_t>(r.y) + r.h;
    switch (rotation) {
        case 0:
            return RectFromEdges(x0, y0, x1, y1);
        case 1:  // 90° CW: (x,y) -> (H - y1, x)
            return RectFromEdges(H - y1, x0, H - y0, x1);
        case 2:  // 180°: (x,y) -> (W - x1, H - y1)
            return RectFromEdges(W - x1, H - y1, W - x0, H - y0);
        case 3:  // 270° CW: (x,y) -> (y, W - x1)
            return RectFromEdges(y0, W - x1, y1, W - x0);
    }
    return Result<Rect>::Err(StatusCode::InvalidArgument);
}

Result<Rect> AlignDamageForEpd(const Rect &r, const Size &bounds,
                               int32_t align_x) {
    if (align_x <= 0 || (align_x & (align_x - 1)) != 0) {
        return Result<Rect>::Err(StatusCode::InvalidArgument);
    }
    const Result<Rect> clamped = ClampTo(r, bounds);
    if (!clamped.ok() || IsEmpty(clamped.value)) {
        return clamped;
    }
    const int64_t mask = ~static_cast<int64_t>(align_x - 1);
    const int64_t x0 = static_cast<int64_t>(clamped.value.x) & mask;
    int64_t x1 = static_cast<int64_t>(clamped.value.x) + clamped.value.w;
    x1 = (x1 + align_x - 1) & mask;
    // Alignment expands only horizontally; re-clamp the right edge to bounds.
    const int64_t bx1 = bounds.w;
    return RectFromEdges(x0, clamped.value.y, (x1 < bx1) ? x1 : bx1,
                         static_cast<int64_t>(clamped.value.y) +
                             clamped.value.h);
}

}  // namespace s3paper
