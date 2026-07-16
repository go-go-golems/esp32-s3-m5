// Retained widget tree (Phase 9): typed nodes in a bounded arena with
// generation-safe handles.
//
// Rules (ticket design doc §5/§6.1):
//  - Nodes are POD state, never callbacks or borrowed pointers. Text is
//    copied into the node; dynamic data binds by DependencyId; taps report
//    a hit_id — the app (or a future JS runtime) owns what those ids mean.
//  - The arena is fixed-capacity; exhaustion and stale handles are explicit
//    (CapacityExceeded / InvalidArgument), never UB.
//  - Mutation goes through setters that bump the node's content version so
//    render-state diffing can compute damage without dirty flags in nodes.
#pragma once

#include <stdint.h>

#include "s3paper/draw_ops.h"
#include "s3paper/geometry.h"
#include "s3paper/status.h"

namespace s3paper {

using DependencyId = uint32_t;  // 0 = none

enum class WidgetKind : uint8_t {
    Text = 0,
    Row,
    Col,
    Spacer,
    Divider,
    Progress,
    List,
    Book,
    Region,
    Canvas,  // freehand command list (ESP-52); content in the arena store
};

const char *WidgetKindName(WidgetKind kind);

enum class MainAlign : uint8_t { Start = 0, Center, End, SpaceBetween };
enum class CrossAlign : uint8_t { Stretch = 0, Start, Center, End };
enum class TextAlign : uint8_t { Start = 0, Center, End };

struct WidgetHandle {
    uint16_t index;
    uint16_t generation;  // 0 = null handle
};

constexpr WidgetHandle kNullWidget{0, 0};
constexpr uint16_t kNoWidgetIndex = 0xFFFF;

inline bool IsNull(const WidgetHandle &h) { return h.generation == 0; }

struct TextProps {
    static constexpr uint32_t kCapacity = 64;
    char value[kCapacity];  // NUL-terminated copy, never a borrowed pointer
    uint8_t font_id;
    Gray8 gray;
    TextAlign align;
    // Filled background in `gray`, glyphs in the inverse (Swiss chrome
    // chips). ESP-51 v2 builder prop.
    uint8_t invert;
};

struct SpacerProps {
    int32_t fixed;  // main-axis pixels when flex == 0
};

struct DividerProps {
    int32_t thickness;
    Gray8 gray;
};

struct ProgressProps {
    uint16_t permille;  // 0..1000
    int32_t height;
    Gray8 gray;
};

struct ListProps {
    uint16_t first_visible;  // child index offset for pagination (not scroll)
};

struct BookProps {
    uint32_t book_ref;  // app-defined content identity; layout only reserves
};

// Canvas: node side holds only the store slot; commands live in the
// arena's CanvasCmd slabs (a POD node cannot hold a variable list).
struct CanvasProps {
    uint16_t store;  // index into WidgetArena's canvas slabs
};

// One freehand drawing command, canvas-relative coordinates (the emitter
// adds the frame origin at render time). 12 bytes, POD.
struct CanvasCmd {
    enum Kind : uint8_t {
        kFill = 0,   // a,b,c,d = x,y,w,h
        kBox,        // a,b,c,d = x,y,w,h (outline, thickness)
        kLine,       // a,b,c,d = x0,y0,x1,y1 (thickness)
        kDisc,       // a,b,c = cx,cy,r
        kRing,       // a,b,c = cx,cy,r (thickness)
    };
    uint8_t kind;
    Gray8 gray;
    uint8_t thickness;
    uint8_t _pad;
    int16_t a, b, c, d;
};

struct RegionProps {
    uint32_t region_id;
    uint32_t interval_ms;  // 0 = event-only
    bool quiet_while_active;
};

struct WidgetNode {
    WidgetKind kind;
    uint16_t generation;
    bool in_use;
    uint16_t first_child;
    uint16_t last_child;
    uint16_t next_sibling;
    uint16_t parent;  // kNoWidgetIndex when detached; guards re-parenting
    uint32_t content_version;  // bumped by setters; diffed for damage

    // Layout props (containers use all; leaves use sizing only).
    Insets padding;
    int32_t gap;
    MainAlign main_align;
    CrossAlign cross_align;
    int32_t fixed_w;  // -1 = auto
    int32_t fixed_h;  // -1 = auto
    uint16_t flex;    // 0 = intrinsic size on the parent's main axis

    // Interaction / invalidation (0 = none).
    uint32_t hit_id;
    int16_t hit_z;
    DependencyId dependency;

    union {
        TextProps text;
        SpacerProps spacer;
        DividerProps divider;
        ProgressProps progress;
        ListProps list;
        BookProps book;
        RegionProps region;
        CanvasProps canvas;
    } props;
};

// Bounded retained tree storage. All handle-taking calls validate index and
// generation; a destroyed slot's generation advances so stale handles fail.
class WidgetArena {
  public:
    static constexpr uint32_t kCapacity = 128;

    WidgetArena() { Reset(); }

    // Destroys everything and invalidates all outstanding handles.
    void Reset();

    Result<WidgetHandle> Create(WidgetKind kind);
    // Destroys a node and its whole subtree. The node must not still be
    // linked as someone's child (destroy the root of what you detached).
    Status Destroy(WidgetHandle handle);

    // Appends child to parent's child list (paint/layout order).
    Status AddChild(WidgetHandle parent, WidgetHandle child);
    // Unlinks child from parent (the subtree stays alive; Destroy it or
    // re-attach it elsewhere). InvalidArgument when not a child of parent.
    Status RemoveChild(WidgetHandle parent, WidgetHandle child);

    const WidgetNode *Get(WidgetHandle handle) const;
    // Index-based access for layout/render internals (no generation check).
    const WidgetNode *At(uint16_t index) const;

    // ---- Mutators (bump content_version) ----
    Status SetText(WidgetHandle handle, const char *text);
    Status SetProgress(WidgetHandle handle, uint16_t permille);
    Status SetListFirstVisible(WidgetHandle handle, uint16_t first_visible);

    // ---- Canvas command store (ESP-52) ----
    static constexpr uint32_t kCanvasSlots = 8;
    static constexpr uint32_t kCanvasCmds = 96;  // per slot
    // Appends one command (CapacityExceeded when the slot is full).
    Status CanvasAppend(WidgetHandle handle, const CanvasCmd &cmd);
    // Drops all commands (version bump: the diff damages the frame).
    Status CanvasClear(WidgetHandle handle);
    // Render-side access; count 0 for stale/non-canvas handles.
    const CanvasCmd *CanvasCmds(const WidgetNode &node,
                                uint32_t *out_count) const;

    // ---- Prop configuration (no version bump; call before first render) ----
    // Returns a mutable node for builder-time setup; nullptr when stale.
    WidgetNode *Configure(WidgetHandle handle);

    uint32_t live_count() const { return live_count_; }

  private:
    WidgetNode nodes_[kCapacity];
    uint32_t live_count_ = 0;
    CanvasCmd canvas_cmds_[kCanvasSlots][kCanvasCmds];
    uint16_t canvas_count_[kCanvasSlots] = {};
    bool canvas_used_[kCanvasSlots] = {};

    friend Result<WidgetHandle> NewCanvas(WidgetArena &arena);

    WidgetNode *GetMutable(WidgetHandle handle);
    void DestroyIndex(uint16_t index);
};

// ---- Builder helpers (P9.3) ----
// Each creates a configured node; composition is AddChild. All return
// Err(CapacityExceeded) when the arena is full.
Result<WidgetHandle> NewText(WidgetArena &arena, const char *text,
                             uint8_t font_id, Gray8 gray,
                             TextAlign align = TextAlign::Start);
Result<WidgetHandle> NewRow(WidgetArena &arena);
Result<WidgetHandle> NewCol(WidgetArena &arena);
Result<WidgetHandle> NewSpacer(WidgetArena &arena, int32_t fixed,
                               uint16_t flex = 0);
Result<WidgetHandle> NewDivider(WidgetArena &arena, int32_t thickness,
                                Gray8 gray);
Result<WidgetHandle> NewProgress(WidgetArena &arena, uint16_t permille,
                                 int32_t height, Gray8 gray);
Result<WidgetHandle> NewList(WidgetArena &arena);
Result<WidgetHandle> NewCanvas(WidgetArena &arena);
Result<WidgetHandle> NewBook(WidgetArena &arena, uint32_t book_ref);
Result<WidgetHandle> NewRegion(WidgetArena &arena, uint32_t region_id,
                               uint32_t interval_ms, bool quiet_while_active);

}  // namespace s3paper
