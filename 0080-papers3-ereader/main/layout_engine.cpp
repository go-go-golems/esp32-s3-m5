#include "layout_engine.h"
#include "bitmap_font.h"

#include <algorithm>
#include <cstring>

namespace gnosis {

namespace {

bool HasExplicitHeight(const Node* n)
{
    return n->explicit_h > 0;
}

bool HasExplicitWidth(const Node* n)
{
    return n->explicit_w > 0;
}

int16_t TextWidth(const Node* n)
{
    int size = std::max<int>(1, n->props[0]);
    int len = static_cast<int>(std::strlen(n->text));
    return static_cast<int16_t>(len * kGlyphW * size + n->pad_x * 2);
}

void LayoutVBox(Node* node, int16_t x, int16_t y, int16_t w, int16_t h)
{
    int32_t fixed_total = 0;
    int flex_count = 0;

    for (int i = 0; i < node->n_children; ++i) {
        Node* c = node->children[i];
        if (HasExplicitHeight(c))
            fixed_total += c->explicit_h;
        else
            ++flex_count;
    }

    int16_t remaining = static_cast<int16_t>(h - fixed_total);
    int16_t flex_h = flex_count > 0 ? static_cast<int16_t>(remaining / flex_count) : 0;
    int16_t cursor_y = y;

    for (int i = 0; i < node->n_children; ++i) {
        Node* c = node->children[i];
        int16_t ch = HasExplicitHeight(c) ? c->explicit_h : flex_h;
        LayoutNode(c, x, cursor_y, w, ch);
        cursor_y += ch;
    }
}

void LayoutHBox(Node* node, int16_t x, int16_t y, int16_t w, int16_t h)
{
    // Split mode
    if (node->props[0] > 0 && node->n_children >= 2) {
        int16_t split_w = node->props[0];
        int16_t divider = 1;
        int16_t right_w = static_cast<int16_t>(w - split_w - divider);
        LayoutNode(node->children[0], x, y, split_w, h);
        LayoutNode(node->children[1], static_cast<int16_t>(x + split_w + divider), y, right_w, h);
        return;
    }

    // General case
    int32_t fixed_total = 0;
    int flex_count = 0;

    for (int i = 0; i < node->n_children; ++i) {
        Node* c = node->children[i];
        if (HasExplicitWidth(c)) {
            fixed_total += c->explicit_w;
        } else if (c->type == NodeType::SPACER) {
            ++flex_count;
        } else if (c->type == NodeType::LABEL || c->type == NodeType::BADGE ||
                   c->type == NodeType::GAUGE || c->type == NodeType::DOT) {
            fixed_total += TextWidth(c);
        } else {
            ++flex_count;
        }
    }

    int16_t remaining = static_cast<int16_t>(w - fixed_total);
    int16_t flex_w = flex_count > 0 ? static_cast<int16_t>(remaining / flex_count) : 0;
    int16_t cursor_x = x;

    for (int i = 0; i < node->n_children; ++i) {
        Node* c = node->children[i];
        int16_t cw;
        if (HasExplicitWidth(c)) {
            cw = c->explicit_w;
        } else if (c->type == NodeType::SPACER) {
            cw = flex_w;
        } else if (c->type == NodeType::LABEL || c->type == NodeType::BADGE ||
                   c->type == NodeType::GAUGE || c->type == NodeType::DOT) {
            cw = TextWidth(c);
        } else {
            cw = flex_w;
        }
        LayoutNode(c, cursor_x, y, cw, h);
        cursor_x += cw;
    }
}

void LayoutFixed(Node* node, int16_t x, int16_t y, int16_t w, int16_t h)
{
    for (int i = 0; i < node->n_children; ++i) {
        Node* c = node->children[i];
        int16_t cx = static_cast<int16_t>(x + c->offset_x);
        int16_t cy = static_cast<int16_t>(y + c->offset_y);
        int16_t cw = c->explicit_w > 0 ? c->explicit_w : static_cast<int16_t>(w - c->offset_x);
        int16_t ch = c->explicit_h > 0 ? c->explicit_h : static_cast<int16_t>(h - c->offset_y);
        LayoutNode(c, cx, cy, cw, ch);
    }
}

void LayoutLeaf(Node* node, int16_t /*x*/, int16_t /*y*/, int16_t w, int16_t h)
{
    switch (node->type) {
    case NodeType::LABEL: {
        int size = std::max<int>(1, node->props[0]);
        node->props[2] = static_cast<int16_t>(w / (kGlyphW * size));
        break;
    }
    case NodeType::LIST: {
        int16_t row_h = node->props[0] > 0 ? node->props[0] : 16;
        int16_t max_items = node->props[1];
        node->props[3] = static_cast<int16_t>(std::min<int16_t>(max_items, static_cast<int16_t>(h / row_h)));
        break;
    }
    case NodeType::GRID: {
        int16_t cols = node->props[0] > 0 ? node->props[0] : 7;
        if (node->props[1] == 0)
            node->props[1] = static_cast<int16_t>(w / cols);
        if (node->props[2] == 0)
            node->props[2] = 20;
        break;
    }
    default:
        break;
    }
}

}  // namespace

void LayoutNode(Node* node, int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (!node) return;
    node->rect = {x, y, w, h};

    switch (node->type) {
    case NodeType::VBOX:
        LayoutVBox(node, x, y, w, h);
        break;
    case NodeType::HBOX:
        LayoutHBox(node, x, y, w, h);
        break;
    case NodeType::FIXED:
        LayoutFixed(node, x, y, w, h);
        break;
    default:
        LayoutLeaf(node, x, y, w, h);
        break;
    }
}

void LayoutScreen(Screen& screen, int16_t W, int16_t H)
{
    int16_t bar_h = screen.bar ? screen.bar->explicit_h : 0;
    int16_t nav_h = screen.nav ? screen.nav->explicit_h : 0;
    int16_t body_h = static_cast<int16_t>(H - bar_h - nav_h);

    if (screen.bar) LayoutNode(screen.bar, 0, 0, W, bar_h);
    if (screen.body) LayoutNode(screen.body, 0, bar_h, W, body_h);
    if (screen.nav) LayoutNode(screen.nav, 0, static_cast<int16_t>(bar_h + body_h), W, nav_h);
}

}  // namespace gnosis
