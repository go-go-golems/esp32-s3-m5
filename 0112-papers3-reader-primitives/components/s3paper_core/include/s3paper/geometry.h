// Defensive integer geometry for the s3paper reader stack.
//
// Conventions (from the ticket design doc):
//  - Rectangles are half-open: [x, x+w) x [y, y+h).
//  - Zero or negative width/height means empty; empty rects normalize to all
//    zeros so traces and comparisons are stable.
//  - All arithmetic runs in int64_t before narrowing; anything that cannot be
//    represented as int32_t is an explicit InvalidArgument, never a wrap.
//
// Pure header/impl: no ESP-IDF, FreeRTOS, or M5 includes.
#pragma once

#include <stdint.h>

#include "s3paper/status.h"

namespace s3paper {

// Plain aggregates with trivial default constructors so they stay legal in
// unions and POD message payloads. Value-initialize ({}) for zeroing.
struct Point {
    int32_t x;
    int32_t y;
};

struct Size {
    int32_t w;
    int32_t h;
};

struct Insets {
    int32_t top;
    int32_t right;
    int32_t bottom;
    int32_t left;
};

struct Rect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

constexpr Rect kEmptyRect{0, 0, 0, 0};

bool IsEmpty(const Rect &r);
bool RectEquals(const Rect &a, const Rect &b);
int64_t Area(const Rect &r);

// Empty inputs normalize to kEmptyRect; anything with negative w/h is empty.
Rect Normalized(const Rect &r);

bool Contains(const Rect &r, const Point &p);
bool ContainsRect(const Rect &outer, const Rect &inner);

// Intersection of possibly-disjoint rects; disjoint yields Ok(kEmptyRect).
// InvalidArgument only when an input edge overflows int32 range.
Result<Rect> Intersect(const Rect &a, const Rect &b);

// Bounding union; an empty side passes the other side through.
Result<Rect> Union(const Rect &a, const Rect &b);

// Clamp into [0,0,bounds.w,bounds.h].
Result<Rect> ClampTo(const Rect &r, const Size &bounds);

Result<Rect> Translate(const Rect &r, int32_t dx, int32_t dy);

// Shrink by insets; over-shrinking yields Ok(kEmptyRect).
Result<Rect> Shrink(const Rect &r, const Insets &insets);

// Map a rect from logical coordinates into physical coordinates for a
// display rotated by `rotation` quarter-turns clockwise (0..3).
// `logical_bounds` is the logical viewport the rect lives in.
Result<Rect> RotateInBounds(const Rect &r, const Size &logical_bounds,
                            uint8_t rotation);

// EPD damage alignment: expand to horizontal multiples of `align_x` and clamp
// to bounds. One authoritative implementation; widgets must not roll their
// own width rounding. align_x must be a positive power of two.
//
// NOTE: align_x=8 is the provisional project default until Phase 0 hardware
// measurements produce a driver-verified constraint (ticket task lvjt).
Result<Rect> AlignDamageForEpd(const Rect &r, const Size &bounds,
                               int32_t align_x);

}  // namespace s3paper
