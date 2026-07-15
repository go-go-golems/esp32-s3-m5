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

    // ---- Prop configuration (no version bump; call before first render) ----
    // Returns a mutable node for builder-time setup; nullptr when stale.
    WidgetNode *Configure(WidgetHandle handle);

    uint32_t live_count() const { return live_count_; }

  private:
    WidgetNode nodes_[kCapacity];
    uint32_t live_count_ = 0;

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
Result<WidgetHandle> NewBook(WidgetArena &arena, uint32_t book_ref);
Result<WidgetHandle> NewRegion(WidgetArena &arena, uint32_t region_id,
                               uint32_t interval_ms, bool quiet_while_active);

}  // namespace s3paper
