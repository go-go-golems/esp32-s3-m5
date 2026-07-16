#include "s3paper/widget.h"

#include <cstring>

namespace s3paper {

const char *WidgetKindName(WidgetKind kind) {
    switch (kind) {
        case WidgetKind::Text: return "Text";
        case WidgetKind::Row: return "Row";
        case WidgetKind::Col: return "Col";
        case WidgetKind::Spacer: return "Spacer";
        case WidgetKind::Divider: return "Divider";
        case WidgetKind::Progress: return "Progress";
        case WidgetKind::List: return "List";
        case WidgetKind::Book: return "Book";
        case WidgetKind::Region: return "Region";
        case WidgetKind::Canvas: return "Canvas";
    }
    return "?";
}

void WidgetArena::Reset() {
    for (uint32_t i = 0; i < kCapacity; ++i) {
        // Keep generations monotonic across Reset so handles from before the
        // reset stay stale instead of aliasing new nodes.
        const uint16_t gen = nodes_[i].generation;
        std::memset(&nodes_[i], 0, sizeof(WidgetNode));
        nodes_[i].generation = gen;
        nodes_[i].first_child = kNoWidgetIndex;
        nodes_[i].last_child = kNoWidgetIndex;
        nodes_[i].next_sibling = kNoWidgetIndex;
        nodes_[i].parent = kNoWidgetIndex;
    }
    live_count_ = 0;
    for (uint32_t s = 0; s < kCanvasSlots; ++s) {
        canvas_used_[s] = false;
        canvas_count_[s] = 0;
    }
}

Result<WidgetHandle> WidgetArena::Create(WidgetKind kind) {
    for (uint32_t i = 0; i < kCapacity; ++i) {
        WidgetNode &n = nodes_[i];
        if (n.in_use) {
            continue;
        }
        const uint16_t gen = n.generation;
        std::memset(&n, 0, sizeof(WidgetNode));
        n.kind = kind;
        // Generation 0 means "null handle"; skip it on wrap.
        n.generation = static_cast<uint16_t>(gen + 1);
        if (n.generation == 0) {
            n.generation = 1;
        }
        n.in_use = true;
        n.first_child = kNoWidgetIndex;
        n.last_child = kNoWidgetIndex;
        n.next_sibling = kNoWidgetIndex;
        n.parent = kNoWidgetIndex;
        n.fixed_w = -1;
        n.fixed_h = -1;
        live_count_++;
        return Result<WidgetHandle>::Ok(
            WidgetHandle{static_cast<uint16_t>(i), n.generation});
    }
    return Result<WidgetHandle>::Err(StatusCode::CapacityExceeded);
}

WidgetNode *WidgetArena::GetMutable(WidgetHandle handle) {
    if (IsNull(handle) || handle.index >= kCapacity) {
        return nullptr;
    }
    WidgetNode &n = nodes_[handle.index];
    if (!n.in_use || n.generation != handle.generation) {
        return nullptr;
    }
    return &n;
}

const WidgetNode *WidgetArena::Get(WidgetHandle handle) const {
    return const_cast<WidgetArena *>(this)->GetMutable(handle);
}

const WidgetNode *WidgetArena::At(uint16_t index) const {
    if (index >= kCapacity || !nodes_[index].in_use) {
        return nullptr;
    }
    return &nodes_[index];
}

void WidgetArena::DestroyIndex(uint16_t index) {
    WidgetNode &n = nodes_[index];
    if (!n.in_use) {
        return;
    }
    uint16_t child = n.first_child;
    while (child != kNoWidgetIndex) {
        const uint16_t next = nodes_[child].next_sibling;
        DestroyIndex(child);
        child = next;
    }
    if (n.kind == WidgetKind::Canvas &&
        n.props.canvas.store < kCanvasSlots) {
        canvas_used_[n.props.canvas.store] = false;
        canvas_count_[n.props.canvas.store] = 0;
    }
    n.in_use = false;
    n.generation = static_cast<uint16_t>(n.generation + 1);
    if (n.generation == 0) {
        n.generation = 1;
    }
    n.first_child = kNoWidgetIndex;
    n.last_child = kNoWidgetIndex;
    n.next_sibling = kNoWidgetIndex;
    n.parent = kNoWidgetIndex;
    live_count_--;
}

Status WidgetArena::Destroy(WidgetHandle handle) {
    WidgetNode *n = GetMutable(handle);
    if (n == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    // Unlink from a live parent first. Destroying a still-linked child
    // used to leave the parent's child index dangling; once the slot was
    // reused the parent pointed at an unrelated node, and the resulting
    // shared/looped links made tree walks recurse forever (found by the
    // ESP-52 canvas fuzz extension, but latent since Phase 9).
    if (n->parent != kNoWidgetIndex && nodes_[n->parent].in_use) {
        (void)RemoveChild(
            WidgetHandle{n->parent, nodes_[n->parent].generation}, handle);
    }
    DestroyIndex(handle.index);
    return OkStatus();
}

Status WidgetArena::AddChild(WidgetHandle parent, WidgetHandle child) {
    WidgetNode *p = GetMutable(parent);
    WidgetNode *c = GetMutable(child);
    if (p == nullptr || c == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (parent.index == child.index) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    // A node has at most one parent (found by fuzzing: a trailing child has
    // no next_sibling, so the old sibling-only check allowed diamonds and
    // ancestor cycles that made every tree walk recurse forever).
    if (c->parent != kNoWidgetIndex) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    // Reject making a node a child of its own descendant.
    for (uint16_t up = parent.index; up != kNoWidgetIndex;
         up = nodes_[up].parent) {
        if (up == child.index) {
            return ErrStatus(StatusCode::InvalidArgument);
        }
    }
    if (p->first_child == kNoWidgetIndex) {
        p->first_child = child.index;
    } else {
        nodes_[p->last_child].next_sibling = child.index;
    }
    p->last_child = child.index;
    c->parent = parent.index;
    return OkStatus();
}

Status WidgetArena::RemoveChild(WidgetHandle parent, WidgetHandle child) {
    WidgetNode *p = GetMutable(parent);
    WidgetNode *c = GetMutable(child);
    if (p == nullptr || c == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    uint16_t prev = kNoWidgetIndex;
    for (uint16_t cur = p->first_child; cur != kNoWidgetIndex;
         cur = nodes_[cur].next_sibling) {
        if (cur != child.index) {
            prev = cur;
            continue;
        }
        if (prev == kNoWidgetIndex) {
            p->first_child = c->next_sibling;
        } else {
            nodes_[prev].next_sibling = c->next_sibling;
        }
        if (p->last_child == child.index) {
            p->last_child = prev;
        }
        c->next_sibling = kNoWidgetIndex;
        c->parent = kNoWidgetIndex;
        return OkStatus();
    }
    return ErrStatus(StatusCode::InvalidArgument);
}

Status WidgetArena::SetText(WidgetHandle handle, const char *text) {
    WidgetNode *n = GetMutable(handle);
    if (n == nullptr || n->kind != WidgetKind::Text || text == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (std::strncmp(n->props.text.value, text,
                     TextProps::kCapacity - 1) == 0 &&
        std::strlen(text) < TextProps::kCapacity) {
        return OkStatus();  // unchanged: no version bump, no damage
    }
    std::strncpy(n->props.text.value, text, TextProps::kCapacity - 1);
    n->props.text.value[TextProps::kCapacity - 1] = '\0';
    n->content_version++;
    return OkStatus();
}

Status WidgetArena::SetProgress(WidgetHandle handle, uint16_t permille) {
    WidgetNode *n = GetMutable(handle);
    if (n == nullptr || n->kind != WidgetKind::Progress) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (permille > 1000) {
        permille = 1000;
    }
    if (n->props.progress.permille == permille) {
        return OkStatus();
    }
    n->props.progress.permille = permille;
    n->content_version++;
    return OkStatus();
}

Status WidgetArena::SetListFirstVisible(WidgetHandle handle,
                                        uint16_t first_visible) {
    WidgetNode *n = GetMutable(handle);
    if (n == nullptr || n->kind != WidgetKind::List) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (n->props.list.first_visible == first_visible) {
        return OkStatus();
    }
    n->props.list.first_visible = first_visible;
    n->content_version++;
    return OkStatus();
}

WidgetNode *WidgetArena::Configure(WidgetHandle handle) {
    return GetMutable(handle);
}

// ---- Builders ----

Result<WidgetHandle> NewText(WidgetArena &arena, const char *text,
                             uint8_t font_id, Gray8 gray, TextAlign align) {
    if (text == nullptr) {
        return Result<WidgetHandle>::Err(StatusCode::InvalidArgument);
    }
    Result<WidgetHandle> h = arena.Create(WidgetKind::Text);
    if (!h.ok()) {
        return h;
    }
    WidgetNode *n = arena.Configure(h.value);
    std::strncpy(n->props.text.value, text, TextProps::kCapacity - 1);
    n->props.text.value[TextProps::kCapacity - 1] = '\0';
    n->props.text.font_id = font_id;
    n->props.text.gray = gray;
    n->props.text.align = align;
    return h;
}

Result<WidgetHandle> NewRow(WidgetArena &arena) {
    return arena.Create(WidgetKind::Row);
}

Result<WidgetHandle> NewCol(WidgetArena &arena) {
    return arena.Create(WidgetKind::Col);
}

Result<WidgetHandle> NewSpacer(WidgetArena &arena, int32_t fixed,
                               uint16_t flex) {
    Result<WidgetHandle> h = arena.Create(WidgetKind::Spacer);
    if (!h.ok()) {
        return h;
    }
    WidgetNode *n = arena.Configure(h.value);
    n->props.spacer.fixed = fixed;
    n->flex = flex;
    return h;
}

Result<WidgetHandle> NewDivider(WidgetArena &arena, int32_t thickness,
                                Gray8 gray) {
    Result<WidgetHandle> h = arena.Create(WidgetKind::Divider);
    if (!h.ok()) {
        return h;
    }
    WidgetNode *n = arena.Configure(h.value);
    n->props.divider.thickness = thickness;
    n->props.divider.gray = gray;
    return h;
}

Result<WidgetHandle> NewProgress(WidgetArena &arena, uint16_t permille,
                                 int32_t height, Gray8 gray) {
    Result<WidgetHandle> h = arena.Create(WidgetKind::Progress);
    if (!h.ok()) {
        return h;
    }
    WidgetNode *n = arena.Configure(h.value);
    n->props.progress.permille = permille > 1000 ? 1000 : permille;
    n->props.progress.height = height;
    n->props.progress.gray = gray;
    n->flex = 1;  // progress bars stretch by default
    return h;
}

Result<WidgetHandle> NewList(WidgetArena &arena) {
    return arena.Create(WidgetKind::List);
}

Result<WidgetHandle> NewBook(WidgetArena &arena, uint32_t book_ref) {
    Result<WidgetHandle> h = arena.Create(WidgetKind::Book);
    if (!h.ok()) {
        return h;
    }
    arena.Configure(h.value)->props.book.book_ref = book_ref;
    return h;
}

Status WidgetArena::CanvasAppend(WidgetHandle handle,
                                 const CanvasCmd &cmd) {
    WidgetNode *n = GetMutable(handle);
    if (n == nullptr || n->kind != WidgetKind::Canvas) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const uint16_t slot = n->props.canvas.store;
    if (slot >= kCanvasSlots || !canvas_used_[slot]) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (canvas_count_[slot] >= kCanvasCmds) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    canvas_cmds_[slot][canvas_count_[slot]++] = cmd;
    n->content_version++;
    return OkStatus();
}

Status WidgetArena::CanvasClear(WidgetHandle handle) {
    WidgetNode *n = GetMutable(handle);
    if (n == nullptr || n->kind != WidgetKind::Canvas) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const uint16_t slot = n->props.canvas.store;
    if (slot >= kCanvasSlots || !canvas_used_[slot]) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    if (canvas_count_[slot] != 0) {
        canvas_count_[slot] = 0;
        n->content_version++;
    }
    return OkStatus();
}

const CanvasCmd *WidgetArena::CanvasCmds(const WidgetNode &node,
                                         uint32_t *out_count) const {
    if (node.kind != WidgetKind::Canvas ||
        node.props.canvas.store >= kCanvasSlots ||
        !canvas_used_[node.props.canvas.store]) {
        *out_count = 0;
        return nullptr;
    }
    *out_count = canvas_count_[node.props.canvas.store];
    return canvas_cmds_[node.props.canvas.store];
}

Result<WidgetHandle> NewCanvas(WidgetArena &arena) {
    uint16_t slot = WidgetArena::kCanvasSlots;
    for (uint16_t s = 0; s < WidgetArena::kCanvasSlots; ++s) {
        if (!arena.canvas_used_[s]) {
            slot = s;
            break;
        }
    }
    if (slot >= WidgetArena::kCanvasSlots) {
        return Result<WidgetHandle>::Err(StatusCode::CapacityExceeded);
    }
    Result<WidgetHandle> h = arena.Create(WidgetKind::Canvas);
    if (!h.ok()) {
        return h;
    }
    arena.canvas_used_[slot] = true;
    arena.canvas_count_[slot] = 0;
    arena.Configure(h.value)->props.canvas.store = slot;
    return h;
}

Result<WidgetHandle> NewRegion(WidgetArena &arena, uint32_t region_id,
                               uint32_t interval_ms,
                               bool quiet_while_active) {
    Result<WidgetHandle> h = arena.Create(WidgetKind::Region);
    if (!h.ok()) {
        return h;
    }
    WidgetNode *n = arena.Configure(h.value);
    n->props.region.region_id = region_id;
    n->props.region.interval_ms = interval_ms;
    n->props.region.quiet_while_active = quiet_while_active;
    return h;
}

}  // namespace s3paper
