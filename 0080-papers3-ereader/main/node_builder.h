#pragma once

#include "gnosis_types.h"
#include <cstring>
#include <initializer_list>

namespace gnosis {

// ── Builder helpers ──
// These create nodes from the static pool. Use initializer_list so presets
// read like the Gnosis JSON DSL.

inline Node* VBox(NodePool& p, std::initializer_list<Node*> kids, int16_t h = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::VBOX;
    n->explicit_h = h;
    for (auto* c : kids) {
        if (c && n->n_children < kMaxChildren)
            n->children[n->n_children++] = c;
    }
    return n;
}

inline Node* HBox(NodePool& p, std::initializer_list<Node*> kids, int16_t h = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::HBOX;
    n->explicit_h = h;
    for (auto* c : kids) {
        if (c && n->n_children < kMaxChildren)
            n->children[n->n_children++] = c;
    }
    return n;
}

inline Node* HBoxSplit(NodePool& p, int16_t split_w, Node* left, Node* right)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::HBOX;
    n->props[0] = split_w;
    if (left) n->children[n->n_children++] = left;
    if (right) n->children[n->n_children++] = right;
    return n;
}

inline Node* Fixed(NodePool& p, std::initializer_list<Node*> kids)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::FIXED;
    for (auto* c : kids) {
        if (c && n->n_children < kMaxChildren)
            n->children[n->n_children++] = c;
    }
    return n;
}

inline Node* Label(NodePool& p, const char* text, int size = 1, int color = 0,
                   int16_t ox = 0, int16_t oy = 0, int16_t w = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::LABEL;
    n->props[0] = static_cast<int16_t>(size);
    n->props[1] = static_cast<int16_t>(color);
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    std::strncpy(n->text, text, kMaxTextLen - 1);
    return n;
}

inline Node* Spacer(NodePool& p)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::SPACER;
    return n;
}

inline Node* Sep(NodePool& p, int16_t ox = 0, int16_t oy = 0, int16_t w = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::SEP;
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    n->explicit_h = 1;
    return n;
}

inline Node* Dot(NodePool& p, int16_t w = 12)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::DOT;
    n->explicit_w = w;
    return n;
}

inline Node* Circle(NodePool& p, int16_t cx, int16_t cy, int16_t r)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::CIRCLE;
    n->props[0] = cx;
    n->props[1] = cy;
    n->props[2] = r;
    n->explicit_w = 1;
    n->explicit_h = 1;
    return n;
}

inline Node* Cross(NodePool& p, int16_t cx, int16_t cy, int16_t arm)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::CROSS;
    n->props[0] = cx;
    n->props[1] = cy;
    n->props[2] = arm;
    n->explicit_w = 1;
    n->explicit_h = 1;
    return n;
}

inline Node* Bar(NodePool& p, int16_t max_val, int16_t cur_val, int16_t bar_h = 4,
                 int16_t ox = 0, int16_t oy = 0, int16_t w = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::BAR;
    n->props[0] = max_val;
    n->props[1] = cur_val;
    n->props[2] = bar_h;
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    n->explicit_h = bar_h;
    return n;
}

inline Node* Gauge(NodePool& p, const char* label, int16_t cur, int16_t max_val,
                   int16_t ox, int16_t oy, int16_t w)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::GAUGE;
    n->props[0] = max_val;
    n->props[1] = cur;
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    n->explicit_h = 10;
    std::strncpy(n->text, label, kMaxTextLen - 1);
    return n;
}

inline Node* Badge(NodePool& p, const char* text, int16_t w = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::BADGE;
    n->explicit_w = w;
    std::strncpy(n->text, text, kMaxTextLen - 1);
    return n;
}

inline Node* Icon(NodePool& p, int shape, int16_t w = 24)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::ICON;
    n->props[0] = static_cast<int16_t>(shape);
    n->explicit_w = w;
    return n;
}

inline Node* TextBlock(NodePool& p, const char* text, int16_t line_h = 14,
                       int16_t ox = 0, int16_t oy = 0, int16_t w = 0, int16_t h = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::TEXT_BLOCK;
    n->props[0] = line_h;
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    n->explicit_h = h;
    std::strncpy(n->text, text, kMaxTextLen - 1);
    return n;
}

inline Node* Grid(NodePool& p, int16_t cols, int16_t cell_w, int16_t cell_h,
                  int16_t count, int8_t today = -1, uint32_t events = 0,
                  int16_t ox = 0, int16_t oy = 0, int16_t w = 0, int16_t h = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::GRID;
    n->props[0] = cols;
    n->props[1] = cell_w;
    n->props[2] = cell_h;
    n->props[3] = count;
    n->grid_today = today;
    n->grid_events = events;
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    n->explicit_h = h;
    return n;
}

inline Node* List(NodePool& p, int16_t row_h, int16_t max_items,
                  int16_t ox = 0, int16_t oy = 0, int16_t w = 0, int16_t h = 0)
{
    Node* n = p.Alloc();
    if (!n) return nullptr;
    n->type = NodeType::LIST;
    n->props[0] = row_h;
    n->props[1] = max_items;
    n->offset_x = ox;
    n->offset_y = oy;
    n->explicit_w = w;
    n->explicit_h = h;
    return n;
}

// Helper to set list row data
inline void ListAddRow(Node* list, const char* c0, int c0w = 60, int c0color = 0,
                       const char* c1 = nullptr, int c1w = 60, int c1color = 0)
{
    if (!list || list->list_data_count >= Node::kMaxListRows) return;
    auto& row = list->list_data[list->list_data_count++];
    std::strncpy(row.cols[0].text, c0, sizeof(row.cols[0].text) - 1);
    row.cols[0].w = static_cast<int16_t>(c0w);
    row.cols[0].color = static_cast<uint8_t>(c0color);
    row.n_cols = 1;
    if (c1) {
        std::strncpy(row.cols[1].text, c1, sizeof(row.cols[1].text) - 1);
        row.cols[1].w = static_cast<int16_t>(c1w);
        row.cols[1].color = static_cast<uint8_t>(c1color);
        row.n_cols = 2;
    }
}

}  // namespace gnosis
