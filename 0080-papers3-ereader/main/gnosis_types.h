#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gnosis {

// ── Geometry ──

struct Rect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;

    bool Contains(int16_t px, int16_t py) const
    {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }

    bool Intersects(const Rect& o) const
    {
        return !(o.x >= x + w || o.x + o.w <= x || o.y >= y + h || o.y + o.h <= y);
    }

    Rect Intersection(const Rect& o) const
    {
        int16_t ix = std::max(x, o.x);
        int16_t iy = std::max(y, o.y);
        int16_t ix2 = std::min<int16_t>(x + w, o.x + o.w);
        int16_t iy2 = std::min<int16_t>(y + h, o.y + o.h);
        if (ix2 <= ix || iy2 <= iy) return {};
        return {ix, iy, static_cast<int16_t>(ix2 - ix), static_cast<int16_t>(iy2 - iy)};
    }

    Rect Union(const Rect& o) const
    {
        if (w == 0 && h == 0) return o;
        if (o.w == 0 && o.h == 0) return *this;
        int16_t ux = std::min(x, o.x);
        int16_t uy = std::min(y, o.y);
        int16_t ux2 = std::max<int16_t>(x + w, o.x + o.w);
        int16_t uy2 = std::max<int16_t>(y + h, o.y + o.h);
        return {ux, uy, static_cast<int16_t>(ux2 - ux), static_cast<int16_t>(uy2 - uy)};
    }

    int32_t Area() const { return static_cast<int32_t>(w) * h; }
};

// ── Node Types ──

enum class NodeType : uint8_t {
    VBOX,
    HBOX,
    FIXED,
    LABEL,
    BAR,
    LIST,
    GRID,
    CIRCLE,
    CROSS,
    SEP,
    GAUGE,
    BADGE,
    ICON,
    DOT,
    TEXT_BLOCK,
    SPACER,
};

enum class Waveform : uint8_t {
    FAST = 0,
    PART = 1,
    FULL = 2,
};

// ── Color indices ──

enum class Color : uint8_t {
    FG = 0,
    MID = 1,
    GHOST = 2,
    DIM = 3,
    LIGHT = 4,
    BG = 5,
};

// ── Node ──

static constexpr std::size_t kMaxChildren = 16;
static constexpr std::size_t kMaxTextLen = 64;

struct Node {
    NodeType type = NodeType::VBOX;
    Waveform waveform = Waveform::PART;
    bool dirty = true;

    Rect rect{};

    Node* children[kMaxChildren]{};
    uint8_t n_children = 0;

    // Type-specific packed properties:
    //   LABEL:  [0]=size, [1]=color_idx, [2]=max_vis_chars (computed)
    //   BAR:    [0]=max, [1]=current, [2]=height_px
    //   GAUGE:  [0]=max, [1]=current
    //   LIST:   [0]=row_h, [1]=max_items, [2]=scroll, [3]=visible (computed)
    //   GRID:   [0]=cols, [1]=cell_w, [2]=cell_h, [3]=count
    //   CIRCLE: [0]=cx_off, [1]=cy_off, [2]=radius
    //   CROSS:  [0]=cx_off, [1]=cy_off, [2]=arm_len
    //   ICON:   [0]=shape (0=sq,1=circ,2=dia,3=tri)
    //   HBOX:   [0]=split_w (if non-zero)
    int16_t props[4]{};

    int16_t explicit_w = 0;
    int16_t explicit_h = 0;
    int16_t offset_x = 0;
    int16_t offset_y = 0;

    char text[kMaxTextLen]{};
    const char* ext_text = nullptr;  // External text buffer (for large content like book pages)

    int8_t pad_x = 0;
    int8_t pad_y = 0;
    bool invert = false;
    bool border_t = false;
    bool border_b = false;

    // Grid "today" highlight and event-day bitmask (first 32 days)
    int8_t grid_today = -1;
    uint32_t grid_events = 0;

    // List inline data (up to 8 rows, 2 columns each)
    static constexpr int kMaxListRows = 10;
    static constexpr int kMaxListCols = 3;
    struct ListCell {
        char text[24]{};
        uint8_t color = 0;  // Color enum
        int16_t w = 0;
    };
    struct ListRow {
        ListCell cols[kMaxListCols]{};
        uint8_t n_cols = 0;
    };
    ListRow list_data[kMaxListRows]{};
    uint8_t list_data_count = 0;
    int8_t list_selected = -1;
};

// ── Screen ──

struct Screen {
    Node* bar = nullptr;
    Node* body = nullptr;
    Node* nav = nullptr;
};

// ── Node Pool ──

static constexpr std::size_t kMaxNodes = 192;

class NodePool {
public:
    Node* Alloc()
    {
        if (count_ >= kMaxNodes) return nullptr;
        Node* n = &nodes_[count_++];
        *n = Node{};
        return n;
    }

    void Reset() { count_ = 0; }
    std::size_t Used() const { return count_; }

private:
    Node nodes_[kMaxNodes];
    std::size_t count_ = 0;
};

}  // namespace gnosis
