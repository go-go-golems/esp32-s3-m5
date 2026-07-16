// Flat typed draw operations and frame lifetime contracts.
//
// Rules (ticket design doc §contracts):
//  - DrawOp is POD. Text/bitmap payloads live in the frame arena and are
//    referenced by offset; a DrawOp never stores a pointer into an SD buffer,
//    a temporary std::string, or a JS value.
//  - Arena payloads stay valid until the frame has been presented and the
//    arena is explicitly Reset().
#pragma once

#include <stdint.h>

#include "s3paper/geometry.h"

namespace s3paper {

// 8-bit grayscale: 0 = black, 255 = white. Backends quantize as needed.
using Gray8 = uint8_t;

enum class DrawOpKind : uint8_t {
    FillRect = 0,
    StrokeRect,
    HLine,
    VLine,
    GlyphRun,
    Bitmap,
    Line,    // arbitrary-angle segment (LinePayload)
    Circle,  // filled disc or ring (CirclePayload)
};

const char *DrawOpKindName(DrawOpKind kind);

struct StrokePayload {
    int32_t thickness;
};

// Arbitrary-angle segment (ESP-52 canvas primitives). Endpoints are the
// TRUE pre-clip geometry; `bounds` is the clipped bbox for damage, and the
// rasterizer must honor `clip` per pixel (GlyphRun pattern).
struct LinePayload {
    int32_t x0, y0, x1, y1;
    int32_t thickness;  // >= 1
};

// Circle: filled disc (thickness == 0) or ring (thickness > 0). Center and
// radius are true geometry; `bounds` is the clipped bbox.
struct CirclePayload {
    int32_t cx, cy, r;
    int32_t thickness;
};

struct GlyphRunPayload {
    uint32_t text_offset;  // arena offset of UTF-8 bytes
    uint32_t text_len;
    int32_t baseline_y;    // absolute y of the text baseline
    uint8_t font_id;
    uint8_t size_px;
};

struct BitmapPayload {
    uint32_t data_offset;  // arena offset of pixel data
    uint32_t data_len;
    int32_t stride;        // bytes per row
};

struct DrawOp {
    DrawOpKind kind;
    Gray8 gray;
    // Geometry after clipping (what the backend must render).
    Rect bounds;
    // Clip rect in force when the op was emitted; backends that draw payloads
    // larger than bounds (glyphs, bitmaps) must honor it.
    Rect clip;
    union {
        StrokePayload stroke;
        GlyphRunPayload glyph_run;
        BitmapPayload bitmap;
        LinePayload line;
        CirclePayload circle;
    } payload;
};

using FrameId = uint32_t;

class FrameArena;  // frame_arena.h

// A frozen frame: ops plus the arena that owns their payloads. Valid until
// the arena or op storage is reset by the producing FrameBuilder.
struct RenderFrame {
    const DrawOp *ops;
    uint32_t op_count;
    const FrameArena *arena;
    Rect damage;  // union of op bounds, already clamped to the viewport
    Size viewport;
    FrameId id;
};

}  // namespace s3paper
