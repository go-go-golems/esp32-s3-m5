#include "widget_renderer.h"
#include "bitmap_font.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace gnosis {

uint32_t ResolveColor(int idx)
{
    switch (idx) {
    case 0: return kColorFg;
    case 1: return kColorMid;
    case 2: return kColorGhost;
    case 3: return kColorDim;
    case 4: return kColorLight;
    case 5: return kColorBg;
    default: return kColorFg;
    }
}

namespace {

void DrawLabel(M5GFX& display, Node* node)
{
    const char* text = node->text;
    int size = std::max(1, static_cast<int>(node->props[0]));
    int max_chars = node->props[2];
    uint32_t color = ResolveColor(node->props[1]);

    if (node->invert) {
        int text_w = MeasureBitmapText(text, size) + 6;
        int text_h = kGlyphH * size + 4;
        display.fillRect(node->rect.x, node->rect.y, text_w, text_h, kColorFg);
        DrawBitmapText(display, text, node->rect.x + 3, node->rect.y + 2, size, kColorBg, max_chars);
    } else {
        DrawBitmapText(display, text, node->rect.x + node->pad_x, node->rect.y + node->pad_y,
                       size, color, max_chars);
    }
}

void DrawBar(M5GFX& display, Node* node)
{
    int16_t max_val = node->props[0] > 0 ? node->props[0] : 100;
    int16_t cur_val = node->props[1];
    int16_t bar_h = node->props[2] > 0 ? node->props[2] : 4;
    int16_t fill_w = static_cast<int16_t>(node->rect.w * std::min(cur_val, max_val) / max_val);

    display.fillRect(node->rect.x, node->rect.y, node->rect.w, bar_h, kColorLight);
    display.fillRect(node->rect.x, node->rect.y, fill_w, bar_h, kColorFg);
}

void DrawGauge(M5GFX& display, Node* node)
{
    int16_t max_val = node->props[0] > 0 ? node->props[0] : 360;
    int16_t cur_val = node->props[1];

    // Label char
    DrawBitmapText(display, node->text, node->rect.x, node->rect.y, 1, kColorMid);
    // Zero-padded value
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%03d", cur_val);
    DrawBitmapText(display, buf, node->rect.x + 2 * kGlyphW, node->rect.y, 1, kColorFg);
    // Bar
    int bar_x = node->rect.x + 7 * kGlyphW;
    int bar_w = node->rect.w - 7 * kGlyphW - 4;
    if (bar_w > 0) {
        display.fillRect(bar_x, node->rect.y + 3, bar_w, 3, kColorLight);
        int fill_w = bar_w * std::min(cur_val, max_val) / max_val;
        display.fillRect(bar_x, node->rect.y + 3, fill_w, 3, kColorFg);
    }
}

void DrawList(M5GFX& display, Node* node)
{
    int16_t row_h = node->props[0] > 0 ? node->props[0] : 16;
    int16_t visible = node->props[3] > 0 ? node->props[3] : node->list_data_count;
    visible = std::min<int16_t>(visible, node->list_data_count);

    for (int i = 0; i < visible; ++i) {
        int16_t ry = static_cast<int16_t>(node->rect.y + i * row_h);

        if (i == node->list_selected) {
            display.fillRect(node->rect.x, ry, node->rect.w, row_h, kColorLight);
        }

        const auto& row = node->list_data[i];
        int cx = node->rect.x + 2;
        for (int c = 0; c < row.n_cols; ++c) {
            uint32_t col_color = ResolveColor(row.cols[c].color);
            DrawBitmapText(display, row.cols[c].text, cx, ry + 2, 1, col_color);
            cx += row.cols[c].w > 0 ? row.cols[c].w : 60;
        }
    }
}

void DrawGrid(M5GFX& display, Node* node)
{
    int16_t cols = node->props[0] > 0 ? node->props[0] : 7;
    int16_t cell_w = node->props[1];
    int16_t cell_h = node->props[2];
    int16_t count = node->props[3] > 0 ? node->props[3] : cols * 5;

    for (int i = 0; i < count; ++i) {
        int col = i % cols;
        int row = i / cols;
        int gx = node->rect.x + col * cell_w;
        int gy = node->rect.y + row * cell_h;

        display.drawRect(gx, gy, cell_w, cell_h, kColorLight);

        char buf[4];
        int day = (i % 31) + 1;
        std::snprintf(buf, sizeof(buf), "%d", day);

        if (i == node->grid_today) {
            display.fillRect(gx + 1, gy + 1, cell_w - 2, cell_h - 2, kColorFg);
            DrawBitmapText(display, buf, gx + cell_w - 18, gy + 3, 1, kColorBg);
        } else {
            DrawBitmapText(display, buf, gx + cell_w - 18, gy + 3, 1, kColorFg);
        }

        if (node->grid_events & (1u << i)) {
            display.fillRect(gx + cell_w / 2 - 1, gy + cell_h - 4, 3, 3, kColorFg);
        }
    }
}

void DrawCircle(M5GFX& display, Node* node)
{
    int cx = node->rect.x + node->props[0];
    int cy = node->rect.y + node->props[1];
    int r = node->props[2];
    display.drawCircle(cx, cy, r, kColorMid);
}

void DrawCross(M5GFX& display, Node* node)
{
    int cx = node->rect.x + node->props[0];
    int cy = node->rect.y + node->props[1];
    int len = node->props[2];
    display.drawFastHLine(cx - len, cy, 2 * len, kColorFg);
    display.drawFastVLine(cx, cy - len, 2 * len, kColorFg);
}

void DrawSep(M5GFX& display, Node* node)
{
    display.drawFastHLine(node->rect.x, node->rect.y, node->rect.w, kColorMid);
}

void DrawDot(M5GFX& display, Node* node)
{
    display.fillRect(node->rect.x + node->rect.w / 2 - 2,
                     node->rect.y + node->rect.h / 2 - 2, 4, 4, kColorFg);
}

void DrawBadge(M5GFX& display, Node* node)
{
    int text_w = MeasureBitmapText(node->text, 1) + 8;
    display.fillRect(node->rect.x, node->rect.y + 2, text_w, kGlyphH + 4, kColorFg);
    DrawBitmapText(display, node->text, node->rect.x + 4, node->rect.y + 4, 1, kColorBg);
}

void DrawIcon(M5GFX& display, Node* node)
{
    int sz = 8;
    int ix = node->rect.x + node->rect.w / 2 - sz / 2;
    int iy = node->rect.y + node->rect.h / 2 - sz / 2;

    switch (node->props[0]) {
    case 0: display.drawRect(ix, iy, sz, sz, kColorMid); break;
    case 1: display.drawCircle(ix + sz / 2, iy + sz / 2, sz / 2, kColorMid); break;
    case 2: // diamond
        display.drawLine(ix + sz / 2, iy, ix + sz, iy + sz / 2, kColorMid);
        display.drawLine(ix + sz, iy + sz / 2, ix + sz / 2, iy + sz, kColorMid);
        display.drawLine(ix + sz / 2, iy + sz, ix, iy + sz / 2, kColorMid);
        display.drawLine(ix, iy + sz / 2, ix + sz / 2, iy, kColorMid);
        break;
    case 3: // triangle
        display.drawTriangle(ix + sz / 2, iy, ix + sz, iy + sz, ix, iy + sz, kColorMid);
        break;
    }
}

void DrawTextBlock(M5GFX& display, Node* node)
{
    const char* text = node->text;
    int16_t line_h = node->props[0] > 0 ? node->props[0] : 14;
    int line = 0;
    int y = node->rect.y;
    const char* p = text;

    while (*p && y < node->rect.y + node->rect.h) {
        // Find end of line
        const char* eol = std::strchr(p, '\n');
        int len = eol ? static_cast<int>(eol - p) : static_cast<int>(std::strlen(p));
        char line_buf[64];
        len = std::min(len, 63);
        std::memcpy(line_buf, p, len);
        line_buf[len] = '\0';

        DrawBitmapText(display, line_buf, node->rect.x, y, 1, kColorFg);
        y += line_h;
        ++line;

        if (eol)
            p = eol + 1;
        else
            break;
    }
}

}  // namespace

void DrawWidget(M5GFX& display, Node* node, const Rect& /*clip*/)
{
    switch (node->type) {
    case NodeType::LABEL:      DrawLabel(display, node); break;
    case NodeType::BAR:        DrawBar(display, node); break;
    case NodeType::GAUGE:      DrawGauge(display, node); break;
    case NodeType::LIST:       DrawList(display, node); break;
    case NodeType::GRID:       DrawGrid(display, node); break;
    case NodeType::CIRCLE:     DrawCircle(display, node); break;
    case NodeType::CROSS:      DrawCross(display, node); break;
    case NodeType::SEP:        DrawSep(display, node); break;
    case NodeType::DOT:        DrawDot(display, node); break;
    case NodeType::BADGE:      DrawBadge(display, node); break;
    case NodeType::ICON:       DrawIcon(display, node); break;
    case NodeType::TEXT_BLOCK: DrawTextBlock(display, node); break;
    default: break;
    }
}

void RenderSubtree(M5GFX& display, Node* node, const Rect& clip)
{
    if (!node) return;
    if (!node->rect.Intersects(clip)) return;

    // Container decorations
    if (node->border_b) {
        display.drawFastHLine(node->rect.x, node->rect.y + node->rect.h - 1,
                              node->rect.w, kColorMid);
    }
    if (node->border_t) {
        display.drawFastHLine(node->rect.x, node->rect.y, node->rect.w, kColorMid);
    }
    // HBOX split divider
    if (node->type == NodeType::HBOX && node->props[0] > 0) {
        display.drawFastVLine(node->rect.x + node->props[0], node->rect.y,
                              node->rect.h, kColorMid);
    }

    if (node->n_children == 0) {
        DrawWidget(display, node, clip);
    } else {
        for (int i = 0; i < node->n_children; ++i) {
            RenderSubtree(display, node->children[i], clip);
        }
    }
}

}  // namespace gnosis
