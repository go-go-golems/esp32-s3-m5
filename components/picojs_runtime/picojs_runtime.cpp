#include "picojs_runtime.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "esp_log.h"

namespace {
constexpr const char *kTag = "picojs_runtime";
constexpr uint32_t kDefaultFrameIntervalMs = 100;
constexpr size_t kMaxCells = PICOJS_RUNTIME_DEFAULT_COLS * PICOJS_RUNTIME_DEFAULT_ROWS;

JSClassID g_os_class = 0;
JSClassID g_app_class = 0;
JSClassID g_panel_class = 0;
JSClassID g_text_class = 0;
JSClassID g_gauge_class = 0;
JSClassID g_layout_class = 0;
JSClassID g_widget_class = 0;

struct ScreenCell {
    char ch = ' ';
};

struct StoredValue {
    JSContext *ctx = nullptr;
    JSValue value = JS_UNDEFINED;

    StoredValue() = default;
    StoredValue(JSContext *c, JSValueConst v) : ctx(c), value(JS_DupValue(c, v)) {}
    StoredValue(const StoredValue &) = delete;
    StoredValue &operator=(const StoredValue &) = delete;
    StoredValue(StoredValue &&other) noexcept : ctx(other.ctx), value(other.value) {
        other.ctx = nullptr;
        other.value = JS_UNDEFINED;
    }
    StoredValue &operator=(StoredValue &&other) noexcept {
        if (this != &other) {
            reset();
            ctx = other.ctx;
            value = other.value;
            other.ctx = nullptr;
            other.value = JS_UNDEFINED;
        }
        return *this;
    }
    ~StoredValue() { reset(); }

    void reset() {
        if (ctx && !JS_IsUndefined(value)) JS_FreeValue(ctx, value);
        ctx = nullptr;
        value = JS_UNDEFINED;
    }
    void set(JSContext *c, JSValueConst v) {
        reset();
        ctx = c;
        value = JS_DupValue(c, v);
    }
    bool has() const { return ctx && !JS_IsUndefined(value); }
};

struct TextWidget;
struct GaugeWidget;
struct Panel;
struct App;

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct LayoutSegment {
    bool star = false;
    int size = 0;
    std::string id;
    Rect rect;
};

struct TextWidget {
    Panel *panel = nullptr;
    StoredValue value;
    int x = 0;
    int y = 0;
    std::string x_align;
    std::string fg;
    bool bold = false;
    bool dim = false;
};

struct GaugeWidget {
    Panel *panel = nullptr;
    int x = 0;
    int y = 0;
    int width = 16;
    std::string label;
    StoredValue value;
    StoredValue max;
    std::string source;
    int literal_value = 0;
    int literal_max = 100;
    std::string style;
    bool show_pct = false;
};

enum class WidgetKind {
    Spark,
    Table,
    Menu,
    List,
    Grid,
};

struct GridLayer {
    std::string name;
    StoredValue fn;
    std::string glyph;
};

struct GenericWidget {
    Panel *panel = nullptr;
    WidgetKind kind = WidgetKind::Spark;
    int x = 0;
    int y = 0;
    int width = 20;
    int height = 6;
    int selected = 0;
    int grid_cols = 1;
    std::string label;
    std::string marker = ">";
    std::string accent;
    std::string frame;
    std::string title;
    std::string cell = ". ";
    StoredValue data;
    StoredValue items;
    StoredValue rows;
    std::vector<std::string> columns;
    std::vector<GridLayer> layers;
};

struct Panel {
    App *app = nullptr;
    std::string id;
    std::string frame;
    StoredValue title;
    StoredValue title_right;
    StoredValue footer;
    std::vector<std::unique_ptr<TextWidget>> texts;
    std::vector<std::unique_ptr<GaugeWidget>> gauges;
    std::vector<std::unique_ptr<GenericWidget>> widgets;
};

struct TimerCallback {
    uint32_t interval_ms = 0;
    uint32_t acc_ms = 0;
    StoredValue fn;
};

struct LoopCallback {
    uint32_t step_ms = 0;
    uint32_t acc_ms = 0;
    StoredValue fn;
};

struct KeyCallback {
    std::string token;
    StoredValue fn;
};

struct App {
    struct picojs_runtime *rt = nullptr;
    std::string name;
    bool mounted = false;
    StoredValue statusbar;
    char layout_axis = 0;
    std::vector<LayoutSegment> layout_segments;
    std::vector<std::unique_ptr<Panel>> panels;
    std::vector<TimerCallback> timers;
    std::vector<LoopCallback> loops;
    std::vector<StoredValue> computes;
    std::vector<KeyCallback> keys;
};

struct LayoutBuilder {
    App *app = nullptr;
};
} // namespace

struct picojs_runtime {
    bool initialized = false;
    uint16_t cols = PICOJS_RUNTIME_DEFAULT_COLS;
    uint16_t rows = PICOJS_RUNTIME_DEFAULT_ROWS;
    uint32_t frame_interval_ms = kDefaultFrameIntervalMs;
    uint32_t frame_count = 0;
    uint32_t app_count = 0;
    uint32_t mounted_app_count = 0;
    uint32_t last_frame_ms = 0;
    uint32_t last_error_count = 0;
    bool js_installed = false;
    bool app_mode = false;
    ScreenCell cells[kMaxCells];
    char last_key[16] = {};
    std::unique_ptr<App> app;
};

namespace {
void clear_screen(picojs_runtime *rt)
{
    if (!rt) return;
    for (auto &cell : rt->cells) cell.ch = ' ';
}

void put_char(picojs_runtime *rt, int x, int y, char ch)
{
    if (!rt || x < 0 || y < 0 || x >= rt->cols || y >= rt->rows) return;
    rt->cells[y * rt->cols + x].ch = (ch >= 0x20 && ch <= 0x7e) ? ch : '?';
}

void put_text(picojs_runtime *rt, int x, int y, const char *text)
{
    if (!rt || !text || y < 0 || y >= rt->rows) return;
    for (int col = x; col < rt->cols && *text; ++col, ++text) put_char(rt, col, y, *text);
}

void draw_box(picojs_runtime *rt, int x, int y, int w, int h)
{
    if (!rt || w < 2 || h < 2) return;
    for (int col = x + 1; col < x + w - 1; ++col) {
        put_char(rt, col, y, '-');
        put_char(rt, col, y + h - 1, '-');
    }
    for (int row = y + 1; row < y + h - 1; ++row) {
        put_char(rt, x, row, '|');
        put_char(rt, x + w - 1, row, '|');
    }
    put_char(rt, x, y, '+');
    put_char(rt, x + w - 1, y, '+');
    put_char(rt, x, y + h - 1, '+');
    put_char(rt, x + w - 1, y + h - 1, '+');
}

std::string js_to_string(JSContext *ctx, JSValueConst v)
{
    const char *s = JS_ToCString(ctx, v);
    if (!s) return {};
    std::string out(s);
    JS_FreeCString(ctx, s);
    return out;
}

std::string stored_to_string(JSContext *ctx, picojs_runtime *rt, const StoredValue &stored)
{
    if (!stored.has()) return {};
    JSValue v = JS_UNDEFINED;
    bool owned = false;
    if (ctx && JS_IsFunction(ctx, stored.value)) {
        v = JS_Call(ctx, stored.value, JS_UNDEFINED, 0, nullptr);
        owned = true;
        if (JS_IsException(v)) {
            if (rt) ++rt->last_error_count;
            JS_FreeValue(ctx, v);
            return "<error>";
        }
    } else {
        v = stored.value;
    }
    const char *s = JS_ToCString(stored.ctx, v);
    std::string out = s ? s : "";
    if (s) JS_FreeCString(stored.ctx, s);
    if (owned) JS_FreeValue(ctx, v);
    return out;
}

int stored_to_int(JSContext *ctx, picojs_runtime *rt, const StoredValue &stored, int fallback)
{
    if (!stored.has()) return fallback;
    JSValue v = JS_UNDEFINED;
    bool owned = false;
    if (ctx && JS_IsFunction(ctx, stored.value)) {
        v = JS_Call(ctx, stored.value, JS_UNDEFINED, 0, nullptr);
        owned = true;
        if (JS_IsException(v)) {
            if (rt) ++rt->last_error_count;
            JS_FreeValue(ctx, v);
            return fallback;
        }
    } else {
        v = stored.value;
    }
    int32_t out = fallback;
    JS_ToInt32(stored.ctx, &out, v);
    if (owned) JS_FreeValue(ctx, v);
    return (int)out;
}

void render_banner(picojs_runtime *rt)
{
    if (!rt) return;
    clear_screen(rt);
    put_text(rt, 0, 0, "PicoJS runtime ready");
    char line[64] = {};
    std::snprintf(line, sizeof(line), "grid=%ux%u frame=%u", rt->cols, rt->rows, (unsigned)rt->frame_count);
    put_text(rt, 0, 1, line);
    if (rt->last_key[0]) {
        std::snprintf(line, sizeof(line), "last_key=%s", rt->last_key);
        put_text(rt, 0, 2, line);
    }
}

void recompute_layout(App *app)
{
    if (!app || !app->rt || app->layout_segments.empty()) return;
    const int total = app->layout_axis == 'c' ? app->rt->cols : std::max<int>(1, app->rt->rows - 1);
    int fixed = 0;
    int stars = 0;
    for (const auto &segment : app->layout_segments) {
        if (segment.star) ++stars;
        else fixed += std::max(0, segment.size);
    }
    const int star_size = stars > 0 ? std::max(1, (total - fixed) / stars) : 0;
    int pos = 0;
    for (size_t i = 0; i < app->layout_segments.size(); ++i) {
        auto &segment = app->layout_segments[i];
        int sz = segment.star ? star_size : std::max(0, segment.size);
        if (i == app->layout_segments.size() - 1) sz = std::max(0, total - pos);
        if (app->layout_axis == 'c') segment.rect = {pos, 0, sz, std::max<int>(1, app->rt->rows - 1)};
        else segment.rect = {0, pos, app->rt->cols, sz};
        pos += sz;
    }
}

Rect panel_rect(const App *app, const Panel *panel)
{
    const picojs_runtime *rt = app ? app->rt : nullptr;
    Rect fallback{0, 0, rt ? rt->cols : PICOJS_RUNTIME_DEFAULT_COLS, rt ? std::max<int>(1, rt->rows - 1) : PICOJS_RUNTIME_DEFAULT_ROWS - 1};
    if (!app || !panel) return fallback;
    for (const auto &segment : app->layout_segments) {
        if (segment.id == panel->id) return segment.rect;
    }
    return fallback;
}

int gauge_value(JSContext *ctx, picojs_runtime *rt, const GaugeWidget *gauge)
{
    if (!gauge) return 0;
    if (gauge->source == "battery") return 72 + (int)((rt ? rt->frame_count : 0) % 7);
    return stored_to_int(ctx, rt, gauge->value, gauge->literal_value);
}

void draw_gauge(JSContext *ctx, picojs_runtime *rt, const GaugeWidget *gauge, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!rt || !gauge) return;
    const int y = inner_y + gauge->y;
    if (y < inner_y || y >= inner_y + inner_h) return;
    const int x = inner_x + gauge->x;
    const int bar_w = std::max(4, std::min(gauge->width, inner_w - gauge->x - 8));
    const int max_value = std::max(1, stored_to_int(ctx, rt, gauge->max, gauge->literal_max));
    const int value = std::max(0, std::min(max_value, gauge_value(ctx, rt, gauge)));
    const int pct = (value * 100) / max_value;
    char line[96] = {};
    const int filled = (bar_w * value) / max_value;
    char bar[48] = {};
    const int capped_w = std::min<int>(bar_w, sizeof(bar) - 1);
    for (int i = 0; i < capped_w; ++i) bar[i] = i < filled ? '#' : '-';
    bar[capped_w] = 0;
    if (gauge->show_pct) std::snprintf(line, sizeof(line), "%s[%s] %d%%", gauge->label.c_str(), bar, pct);
    else std::snprintf(line, sizeof(line), "%s[%s]", gauge->label.c_str(), bar);
    put_text(rt, x, y, line);
}

int js_array_len(JSContext *ctx, JSValueConst v)
{
    if (!ctx || !JS_IsArray(ctx, v)) return 0;
    JSValue lenv = JS_GetPropertyStr(ctx, v, "length");
    int32_t len = 0;
    JS_ToInt32(ctx, &len, lenv);
    JS_FreeValue(ctx, lenv);
    return std::max<int>(0, len);
}

std::string js_object_prop_string(JSContext *ctx, JSValueConst obj, const std::string &key)
{
    if (!ctx || key.empty()) return {};
    JSValue v = JS_GetPropertyStr(ctx, obj, key.c_str());
    std::string out = js_to_string(ctx, v);
    JS_FreeValue(ctx, v);
    return out;
}

void draw_spark(JSContext *ctx, picojs_runtime *rt, const GenericWidget *w, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!ctx || !rt || !w || !w->data.has()) return;
    const int y = inner_y + w->y;
    if (y < inner_y || y >= inner_y + inner_h) return;
    JSValue arr = JS_UNDEFINED;
    bool owned = false;
    if (JS_IsFunction(ctx, w->data.value)) {
        arr = JS_Call(ctx, w->data.value, JS_UNDEFINED, 0, nullptr);
        owned = true;
    } else {
        arr = w->data.value;
    }
    if (JS_IsException(arr)) {
        ++rt->last_error_count;
        JS_FreeValue(ctx, arr);
        return;
    }
    char line[96] = {};
    std::string prefix = w->label.empty() ? "" : w->label + " ";
    std::snprintf(line, sizeof(line), "%s", prefix.c_str());
    size_t n = std::strlen(line);
    const char glyphs[] = "._-~=+#";
    const int len = std::min<int>(js_array_len(ctx, arr), std::min<int>(inner_w - w->x - (int)n, 30));
    for (int i = 0; i < len && n + 1 < sizeof(line); ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, arr, i);
        int32_t value = 0;
        JS_ToInt32(ctx, &value, item);
        JS_FreeValue(ctx, item);
        const int idx = std::max(0, std::min<int>(6, (value * 6) / 100));
        line[n++] = glyphs[idx];
    }
    line[n] = 0;
    put_text(rt, inner_x + w->x, y, line);
    if (owned) JS_FreeValue(ctx, arr);
}

void draw_menu(JSContext *ctx, picojs_runtime *rt, const GenericWidget *w, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!ctx || !rt || !w || !w->items.has()) return;
    JSValue arr = JS_IsFunction(ctx, w->items.value) ? JS_Call(ctx, w->items.value, JS_UNDEFINED, 0, nullptr) : JS_DupValue(ctx, w->items.value);
    if (JS_IsException(arr)) { ++rt->last_error_count; JS_FreeValue(ctx, arr); return; }
    int ox = inner_x + w->x;
    int oy = inner_y + w->y;
    int width = std::max(8, std::min(inner_w - w->x, w->width > 0 ? w->width : inner_w - w->x));
    if (!w->frame.empty() && oy + 2 < inner_y + inner_h) {
        draw_box(rt, ox, oy, width, std::min(inner_h - w->y, 8));
        if (!w->title.empty()) put_text(rt, ox + 2, oy, w->title.c_str());
        ++ox; ++oy; width -= 2;
    }
    const int count = js_array_len(ctx, arr);
    const int grid = std::max(1, w->grid_cols);
    const int col_w = std::max(1, width / grid);
    for (int i = 0; i < count && oy < inner_y + inner_h; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, arr, i);
        std::string label = js_to_string(ctx, item);
        JS_FreeValue(ctx, item);
        int row = (w->kind == WidgetKind::Menu) ? i / grid : i;
        int col = (w->kind == WidgetKind::Menu) ? i % grid : 0;
        int x = ox + col * col_w;
        int y = oy + row;
        if (y >= inner_y + inner_h) break;
        char line[80] = {};
        std::snprintf(line, sizeof(line), "%s %s", i == w->selected ? w->marker.c_str() : " ", label.c_str());
        put_text(rt, x, y, line);
    }
    JS_FreeValue(ctx, arr);
}

void draw_table(JSContext *ctx, picojs_runtime *rt, const GenericWidget *w, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!ctx || !rt || !w || !w->rows.has()) return;
    JSValue arr = JS_IsFunction(ctx, w->rows.value) ? JS_Call(ctx, w->rows.value, JS_UNDEFINED, 0, nullptr) : JS_DupValue(ctx, w->rows.value);
    if (JS_IsException(arr)) { ++rt->last_error_count; JS_FreeValue(ctx, arr); return; }
    int x = inner_x + w->x;
    int y = inner_y + w->y;
    std::string header;
    for (const auto &c : w->columns) { header += c; header += " "; }
    put_text(rt, x, y++, header.c_str());
    const int count = std::min<int>(js_array_len(ctx, arr), inner_y + inner_h - y);
    for (int i = 0; i < count; ++i) {
        JSValue row = JS_GetPropertyUint32(ctx, arr, i);
        std::string line = (i == w->selected ? w->marker : " ");
        line += " ";
        for (const auto &c : w->columns) {
            std::string v = js_object_prop_string(ctx, row, c);
            if (v.size() > 8) v.resize(8);
            line += v;
            line += " ";
        }
        put_text(rt, x, y + i, line.c_str());
        JS_FreeValue(ctx, row);
    }
    JS_FreeValue(ctx, arr);
}

void draw_grid(JSContext *ctx, picojs_runtime *rt, const GenericWidget *w, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!rt || !w) return;
    const int ox = inner_x + w->x;
    const int oy = inner_y + w->y;
    const int cw = std::max<int>(1, (int)w->cell.size());
    for (int yy = 0; yy < w->height && oy + yy < inner_y + inner_h; ++yy) {
        std::string line;
        for (int xx = 0; xx < w->width && (int)line.size() < inner_w - w->x; ++xx) line += w->cell;
        put_text(rt, ox, oy + yy, line.c_str());
    }
    if (!ctx) return;
    for (const auto &layer : w->layers) {
        if (!layer.fn.has()) continue;
        JSValue arr = JS_IsFunction(ctx, layer.fn.value) ? JS_Call(ctx, layer.fn.value, JS_UNDEFINED, 0, nullptr) : JS_DupValue(ctx, layer.fn.value);
        if (JS_IsException(arr)) { ++rt->last_error_count; JS_FreeValue(ctx, arr); continue; }
        const int count = js_array_len(ctx, arr);
        for (int i = 0; i < count; ++i) {
            JSValue cell = JS_GetPropertyUint32(ctx, arr, i);
            JSValue xv = JS_GetPropertyStr(ctx, cell, "x");
            JSValue yv = JS_GetPropertyStr(ctx, cell, "y");
            int32_t gx = 0, gy = 0;
            JS_ToInt32(ctx, &gx, xv); JS_ToInt32(ctx, &gy, yv);
            const char ch = layer.glyph.empty() ? '*' : layer.glyph[0];
            put_char(rt, ox + gx * cw, oy + gy, ch);
            JS_FreeValue(ctx, xv); JS_FreeValue(ctx, yv); JS_FreeValue(ctx, cell);
        }
        JS_FreeValue(ctx, arr);
    }
}

void draw_generic_widget(JSContext *ctx, picojs_runtime *rt, const GenericWidget *w, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!w) return;
    switch (w->kind) {
        case WidgetKind::Spark: draw_spark(ctx, rt, w, inner_x, inner_y, inner_w, inner_h); break;
        case WidgetKind::Table: draw_table(ctx, rt, w, inner_x, inner_y, inner_w, inner_h); break;
        case WidgetKind::Menu:
        case WidgetKind::List: draw_menu(ctx, rt, w, inner_x, inner_y, inner_w, inner_h); break;
        case WidgetKind::Grid: draw_grid(ctx, rt, w, inner_x, inner_y, inner_w, inner_h); break;
    }
}

void render_app(JSContext *ctx, picojs_runtime *rt)
{
    if (!rt || !rt->app || !rt->app->mounted) {
        render_banner(rt);
        return;
    }
    recompute_layout(rt->app.get());
    clear_screen(rt);
    for (const auto &panel_ptr : rt->app->panels) {
        const Panel *panel = panel_ptr.get();
        const Rect rect = panel_rect(rt->app.get(), panel);
        if (rect.w <= 0 || rect.h <= 0) continue;
        int inner_x = rect.x;
        int inner_y = rect.y;
        int inner_w = rect.w;
        int inner_h = rect.h;
        if (!panel->frame.empty() && rect.w >= 2 && rect.h >= 2) {
            draw_box(rt, rect.x, rect.y, rect.w, rect.h);
            inner_x = rect.x + 1;
            inner_y = rect.y + 1;
            inner_w = std::max(0, rect.w - 2);
            inner_h = std::max(0, rect.h - 2);
            const std::string title = stored_to_string(ctx, rt, panel->title);
            if (!title.empty()) put_text(rt, rect.x + 2, rect.y, title.c_str());
            const std::string title_right = stored_to_string(ctx, rt, panel->title_right);
            if (!title_right.empty()) put_text(rt, rect.x + std::max(1, rect.w - (int)title_right.size() - 2), rect.y, title_right.c_str());
            const std::string footer = stored_to_string(ctx, rt, panel->footer);
            if (!footer.empty()) put_text(rt, rect.x + 2, rect.y + rect.h - 1, footer.c_str());
        } else {
            const std::string title = stored_to_string(ctx, rt, panel->title);
            if (!title.empty()) put_text(rt, rect.x + 1, rect.y, title.c_str());
            const std::string title_right = stored_to_string(ctx, rt, panel->title_right);
            if (!title_right.empty()) put_text(rt, rect.x + std::max(1, rect.w - (int)title_right.size() - 1), rect.y, title_right.c_str());
        }
        for (const auto &text_ptr : panel->texts) {
            const TextWidget *text = text_ptr.get();
            const int y = inner_y + text->y;
            if (y < inner_y || y >= inner_y + inner_h) continue;
            const std::string value = stored_to_string(ctx, rt, text->value);
            if (text->x_align == "center") {
                const int len = (int)std::min<size_t>(value.size(), inner_w);
                const int x = inner_x + std::max(0, (inner_w - len) / 2);
                put_text(rt, x, y, value.c_str());
            } else {
                put_text(rt, inner_x + text->x, y, value.c_str());
            }
        }
        for (const auto &gauge_ptr : panel->gauges) draw_gauge(ctx, rt, gauge_ptr.get(), inner_x, inner_y, inner_w, inner_h);
        for (const auto &widget_ptr : panel->widgets) draw_generic_widget(ctx, rt, widget_ptr.get(), inner_x, inner_y, inner_w, inner_h);
    }
    const std::string status = stored_to_string(ctx, rt, rt->app->statusbar);
    if (!status.empty()) put_text(rt, 0, rt->rows - 1, status.c_str());
}

bool set_func(JSContext *ctx, JSValue obj, const char *name, JSCFunction *fn, int length)
{
    JSValue f = JS_NewCFunction(ctx, fn, name, length);
    if (JS_IsException(f)) return false;
    if (JS_SetPropertyStr(ctx, obj, name, f) < 0) {
        JS_FreeValue(ctx, f);
        return false;
    }
    return true;
}

JSValue make_app_object(JSContext *ctx, App *app);
JSValue make_panel_object(JSContext *ctx, Panel *panel);
JSValue make_text_object(JSContext *ctx, TextWidget *text);
JSValue make_gauge_object(JSContext *ctx, GaugeWidget *gauge);
JSValue make_widget_object(JSContext *ctx, GenericWidget *widget);
JSValue make_layout_object(JSContext *ctx, LayoutBuilder *layout);

JSValue js_os_app(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *rt = static_cast<picojs_runtime *>(JS_GetOpaque(this_val, g_os_class));
    if (!rt) return JS_ThrowTypeError(ctx, "bad OS object");
    auto app = std::make_unique<App>();
    app->rt = rt;
    app->name = argc > 0 ? js_to_string(ctx, argv[0]) : "app";
    rt->app = std::move(app);
    rt->app_count = 1;
    rt->mounted_app_count = 0;
    rt->frame_count = 0;
    rt->last_frame_ms = 0;
    render_banner(rt);
    return make_app_object(ctx, rt->app.get());
}

JSValue js_app_panel(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (!app) return JS_ThrowTypeError(ctx, "bad app object");
    auto panel = std::make_unique<Panel>();
    panel->app = app;
    panel->id = argc > 0 ? js_to_string(ctx, argv[0]) : "main";
    Panel *ptr = panel.get();
    app->panels.push_back(std::move(panel));
    return make_panel_object(ctx, ptr);
}

JSValue js_layout_add(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic)
{
    auto *layout = static_cast<LayoutBuilder *>(JS_GetOpaque(this_val, g_layout_class));
    if (!layout || !layout->app || argc < 2) return JS_DupValue(ctx, this_val);
    const char axis = magic == 1 ? 'c' : 'r';
    if (layout->app->layout_axis == 0) layout->app->layout_axis = axis;
    LayoutSegment segment;
    if (JS_IsString(argv[0])) {
        std::string size = js_to_string(ctx, argv[0]);
        if (size == "*") segment.star = true;
        else segment.size = std::atoi(size.c_str());
    } else {
        int32_t size = 0;
        JS_ToInt32(ctx, &size, argv[0]);
        segment.size = (int)size;
    }
    segment.id = js_to_string(ctx, argv[1]);
    layout->app->layout_segments.push_back(segment);
    recompute_layout(layout->app);
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_layout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (!app || argc < 1 || !JS_IsFunction(ctx, argv[0])) return JS_DupValue(ctx, this_val);
    app->layout_axis = 0;
    app->layout_segments.clear();
    LayoutBuilder layout{app};
    JSValue layout_obj = make_layout_object(ctx, &layout);
    JSValue ret = JS_Call(ctx, argv[0], JS_UNDEFINED, 1, &layout_obj);
    if (JS_IsException(ret)) ++app->rt->last_error_count;
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, layout_obj);
    recompute_layout(app);
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_state(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    return argc > 0 ? JS_DupValue(ctx, argv[0]) : JS_NewObject(ctx);
}

JSValue js_app_statusbar(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (app && argc > 0) app->statusbar.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_noop_this(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_mount(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (app && app->rt) {
        app->mounted = true;
        app->rt->mounted_app_count = 1;
        render_app(ctx, app->rt);
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_on(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (!app || argc < 3 || !JS_IsFunction(ctx, argv[2])) return JS_DupValue(ctx, this_val);
    std::string kind = js_to_string(ctx, argv[0]);
    if (kind != "tick") return JS_DupValue(ctx, this_val);
    int32_t interval = 0;
    JS_ToInt32(ctx, &interval, argv[1]);
    TimerCallback timer;
    timer.interval_ms = std::max<int32_t>(1, interval);
    timer.fn.set(ctx, argv[2]);
    app->timers.push_back(std::move(timer));
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_loop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (!app || argc < 2 || !JS_IsFunction(ctx, argv[1])) return JS_DupValue(ctx, this_val);
    int32_t fps = 0;
    JS_ToInt32(ctx, &fps, argv[0]);
    LoopCallback loop;
    loop.step_ms = fps > 0 ? std::max<uint32_t>(1, 1000 / (uint32_t)fps) : 1000;
    loop.fn.set(ctx, argv[1]);
    app->loops.push_back(std::move(loop));
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_compute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (app && argc > 0 && JS_IsFunction(ctx, argv[0])) app->computes.emplace_back(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_key(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (!app || argc < 2 || !JS_IsFunction(ctx, argv[1])) return JS_DupValue(ctx, this_val);
    std::string spec = js_to_string(ctx, argv[0]);
    auto add_key = [&](const std::string &token) {
        KeyCallback key;
        key.token = token;
        key.fn.set(ctx, argv[1]);
        app->keys.push_back(std::move(key));
    };
    if (spec.find("↑") != std::string::npos) { add_key("↑"); add_key("up"); }
    if (spec.find("↓") != std::string::npos) { add_key("↓"); add_key("down"); }
    if (spec.find("←") != std::string::npos) { add_key("←"); add_key("left"); }
    if (spec.find("→") != std::string::npos) { add_key("→"); add_key("right"); }
    if (spec.find("↑") == std::string::npos && spec.find("↓") == std::string::npos &&
        spec.find("←") == std::string::npos && spec.find("→") == std::string::npos) {
        add_key(spec);
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_panel_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (panel) panel->frame = argc > 0 ? js_to_string(ctx, argv[0]) : "single";
    return JS_DupValue(ctx, this_val);
}

JSValue js_panel_title(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (panel && argc > 0) panel->title.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_panel_title_right(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (panel && argc > 0) panel->title_right.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_panel_footer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (panel && argc > 0) panel->footer.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

GenericWidget *add_generic_widget(Panel *panel, WidgetKind kind)
{
    if (!panel) return nullptr;
    auto widget = std::make_unique<GenericWidget>();
    widget->panel = panel;
    widget->kind = kind;
    GenericWidget *ptr = widget.get();
    panel->widgets.push_back(std::move(widget));
    return ptr;
}

JSValue js_panel_widget(JSContext *ctx, JSValueConst this_val, int, JSValueConst *, int magic)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (!panel) return JS_ThrowTypeError(ctx, "bad panel object");
    WidgetKind kind = WidgetKind::Spark;
    switch (magic) {
        case 1: kind = WidgetKind::Table; break;
        case 2: kind = WidgetKind::Menu; break;
        case 3: kind = WidgetKind::List; break;
        case 4: kind = WidgetKind::Grid; break;
        default: kind = WidgetKind::Spark; break;
    }
    return make_widget_object(ctx, add_generic_widget(panel, kind));
}

JSValue js_panel_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (!panel) return JS_ThrowTypeError(ctx, "bad panel object");
    auto text = std::make_unique<TextWidget>();
    text->panel = panel;
    if (argc > 0) text->value.set(ctx, argv[0]);
    TextWidget *ptr = text.get();
    panel->texts.push_back(std::move(text));
    return make_text_object(ctx, ptr);
}

JSValue js_panel_gauge(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (!panel) return JS_ThrowTypeError(ctx, "bad panel object");
    auto gauge = std::make_unique<GaugeWidget>();
    gauge->panel = panel;
    GaugeWidget *ptr = gauge.get();
    panel->gauges.push_back(std::move(gauge));
    return make_gauge_object(ctx, ptr);
}

JSValue js_text_at(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *text = static_cast<TextWidget *>(JS_GetOpaque(this_val, g_text_class));
    if (text && argc > 0) {
        if (JS_IsString(argv[0])) text->x_align = js_to_string(ctx, argv[0]);
        else {
            int32_t x = 0;
            JS_ToInt32(ctx, &x, argv[0]);
            text->x = (int)x;
            text->x_align.clear();
        }
        if (argc > 1) {
            int32_t y = 0;
            JS_ToInt32(ctx, &y, argv[1]);
            text->y = (int)y;
        }
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_text_fg(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *text = static_cast<TextWidget *>(JS_GetOpaque(this_val, g_text_class));
    if (text && argc > 0) text->fg = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_text_bold(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *text = static_cast<TextWidget *>(JS_GetOpaque(this_val, g_text_class));
    if (text) text->bold = true;
    return JS_DupValue(ctx, this_val);
}

JSValue js_text_dim(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *text = static_cast<TextWidget *>(JS_GetOpaque(this_val, g_text_class));
    if (text) text->dim = true;
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_at(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge && argc > 0) {
        int32_t x = 0;
        JS_ToInt32(ctx, &x, argv[0]);
        gauge->x = (int)x;
        if (argc > 1) {
            int32_t y = 0;
            JS_ToInt32(ctx, &y, argv[1]);
            gauge->y = (int)y;
        }
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge && argc > 0) {
        gauge->label = js_to_string(ctx, argv[0]);
        if (!gauge->label.empty() && gauge->label.back() != ' ') gauge->label.push_back(' ');
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge && argc > 0) {
        if (JS_IsString(argv[0])) gauge->source = js_to_string(ctx, argv[0]);
        else {
            gauge->source.clear();
            gauge->value.set(ctx, argv[0]);
            int32_t literal = 0;
            if (JS_ToInt32(ctx, &literal, argv[0]) == 0) gauge->literal_value = (int)literal;
        }
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_width(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge && argc > 0) {
        int32_t width = 0;
        JS_ToInt32(ctx, &width, argv[0]);
        gauge->width = std::max<int>(4, (int)width);
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_max(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge && argc > 0) {
        gauge->max.set(ctx, argv[0]);
        int32_t literal = 100;
        if (JS_ToInt32(ctx, &literal, argv[0]) == 0) gauge->literal_max = std::max<int32_t>(1, literal);
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_style(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge && argc > 0) gauge->style = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_gauge_show_pct(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge) gauge->show_pct = true;
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_at(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) {
        int32_t x = 0, y = 0;
        JS_ToInt32(ctx, &x, argv[0]);
        if (argc > 1) JS_ToInt32(ctx, &y, argv[1]);
        w->x = (int)x; w->y = (int)y;
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->label = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_data(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->data.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_items(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->items.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_rows(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->rows.set(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_columns(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0 && JS_IsArray(ctx, argv[0])) {
        w->columns.clear();
        int len = js_array_len(ctx, argv[0]);
        for (int i = 0; i < len; ++i) {
            JSValue item = JS_GetPropertyUint32(ctx, argv[0], i);
            w->columns.push_back(js_to_string(ctx, item));
            JS_FreeValue(ctx, item);
        }
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_select(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) { int32_t v = 0; JS_ToInt32(ctx, &v, argv[0]); w->selected = std::max<int32_t>(0, v); }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_marker(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->marker = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_accent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->accent = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->frame = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_title(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->title = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_grid_cols(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) { int32_t v = 1; JS_ToInt32(ctx, &v, argv[0]); w->grid_cols = std::max<int32_t>(1, v); }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_size(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 1) { int32_t x = 0, y = 0; JS_ToInt32(ctx, &x, argv[0]); JS_ToInt32(ctx, &y, argv[1]); w->width = std::max<int32_t>(1, x); w->height = std::max<int32_t>(1, y); }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_width(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) { int32_t v = 0; JS_ToInt32(ctx, &v, argv[0]); w->width = std::max<int32_t>(1, v); }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_cell(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 0) w->cell = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_layer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *w = static_cast<GenericWidget *>(JS_GetOpaque(this_val, g_widget_class));
    if (w && argc > 2) {
        GridLayer layer;
        layer.name = js_to_string(ctx, argv[0]);
        layer.fn.set(ctx, argv[1]);
        layer.glyph = js_to_string(ctx, argv[2]);
        w->layers.push_back(std::move(layer));
    }
    return JS_DupValue(ctx, this_val);
}

JSValue js_widget_noop(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    return JS_DupValue(ctx, this_val);
}

void call_app_callback(JSContext *ctx, picojs_runtime *rt, StoredValue &fn)
{
    if (!ctx || !fn.has() || !JS_IsFunction(ctx, fn.value)) return;
    JSValue app_obj = (rt && rt->app) ? make_app_object(ctx, rt->app.get()) : JS_UNDEFINED;
    JSValue ret = JS_IsUndefined(app_obj)
        ? JS_Call(ctx, fn.value, JS_UNDEFINED, 0, nullptr)
        : JS_Call(ctx, fn.value, JS_UNDEFINED, 1, &app_obj);
    if (JS_IsException(ret) && rt) ++rt->last_error_count;
    JS_FreeValue(ctx, ret);
    if (!JS_IsUndefined(app_obj)) JS_FreeValue(ctx, app_obj);
}

void run_callbacks(JSContext *ctx, picojs_runtime *rt, uint32_t dt_ms)
{
    if (!ctx || !rt || !rt->app) return;
    App *app = rt->app.get();
    for (auto &timer : app->timers) {
        timer.acc_ms += dt_ms;
        if (timer.acc_ms >= timer.interval_ms) {
            timer.acc_ms %= timer.interval_ms;
            call_app_callback(ctx, rt, timer.fn);
        }
    }
    for (auto &loop : app->loops) {
        loop.acc_ms += dt_ms;
        uint32_t guard = 0;
        while (loop.acc_ms >= loop.step_ms && guard++ < 16) {
            loop.acc_ms -= loop.step_ms;
            call_app_callback(ctx, rt, loop.fn);
        }
    }
    for (auto &compute : app->computes) call_app_callback(ctx, rt, compute);
}

JSValue make_app_object(JSContext *ctx, App *app)
{
    JSValue obj = JS_NewObjectClass(ctx, g_app_class);
    JS_SetOpaque(obj, app);
    set_func(ctx, obj, "state", js_app_state, 1);
    set_func(ctx, obj, "layout", js_app_layout, 1);
    set_func(ctx, obj, "panel", js_app_panel, 1);
    set_func(ctx, obj, "statusbar", js_app_statusbar, 1);
    set_func(ctx, obj, "mount", js_app_mount, 0);
    set_func(ctx, obj, "on", js_app_on, 3);
    set_func(ctx, obj, "loop", js_app_loop, 2);
    set_func(ctx, obj, "compute", js_app_compute, 1);
    set_func(ctx, obj, "key", js_app_key, 2);
    set_func(ctx, obj, "refresh", js_app_noop_this, 0);
    set_func(ctx, obj, "dispatch", js_app_noop_this, 0);
    set_func(ctx, obj, "exit", js_app_noop_this, 0);
    return obj;
}

JSValue make_panel_object(JSContext *ctx, Panel *panel)
{
    JSValue obj = JS_NewObjectClass(ctx, g_panel_class);
    JS_SetOpaque(obj, panel);
    set_func(ctx, obj, "frame", js_panel_frame, 1);
    set_func(ctx, obj, "title", js_panel_title, 1);
    set_func(ctx, obj, "titleRight", js_panel_title_right, 1);
    set_func(ctx, obj, "footer", js_panel_footer, 1);
    set_func(ctx, obj, "text", js_panel_text, 1);
    set_func(ctx, obj, "gauge", js_panel_gauge, 0);
    JS_SetPropertyStr(ctx, obj, "spark", JS_NewCFunctionMagic(ctx, js_panel_widget, "spark", 0, JS_CFUNC_generic_magic, 0));
    JS_SetPropertyStr(ctx, obj, "table", JS_NewCFunctionMagic(ctx, js_panel_widget, "table", 0, JS_CFUNC_generic_magic, 1));
    JS_SetPropertyStr(ctx, obj, "menu", JS_NewCFunctionMagic(ctx, js_panel_widget, "menu", 0, JS_CFUNC_generic_magic, 2));
    JS_SetPropertyStr(ctx, obj, "list", JS_NewCFunctionMagic(ctx, js_panel_widget, "list", 0, JS_CFUNC_generic_magic, 3));
    JS_SetPropertyStr(ctx, obj, "grid", JS_NewCFunctionMagic(ctx, js_panel_widget, "grid", 0, JS_CFUNC_generic_magic, 4));
    return obj;
}

JSValue make_text_object(JSContext *ctx, TextWidget *text)
{
    JSValue obj = JS_NewObjectClass(ctx, g_text_class);
    JS_SetOpaque(obj, text);
    set_func(ctx, obj, "at", js_text_at, 2);
    set_func(ctx, obj, "fg", js_text_fg, 1);
    set_func(ctx, obj, "bold", js_text_bold, 0);
    set_func(ctx, obj, "dim", js_text_dim, 0);
    return obj;
}

JSValue make_gauge_object(JSContext *ctx, GaugeWidget *gauge)
{
    JSValue obj = JS_NewObjectClass(ctx, g_gauge_class);
    JS_SetOpaque(obj, gauge);
    set_func(ctx, obj, "at", js_gauge_at, 2);
    set_func(ctx, obj, "label", js_gauge_label, 1);
    set_func(ctx, obj, "value", js_gauge_value, 1);
    set_func(ctx, obj, "width", js_gauge_width, 1);
    set_func(ctx, obj, "max", js_gauge_max, 1);
    set_func(ctx, obj, "style", js_gauge_style, 1);
    set_func(ctx, obj, "showPct", js_gauge_show_pct, 0);
    return obj;
}

JSValue make_widget_object(JSContext *ctx, GenericWidget *widget)
{
    JSValue obj = JS_NewObjectClass(ctx, g_widget_class);
    JS_SetOpaque(obj, widget);
    set_func(ctx, obj, "at", js_widget_at, 2);
    set_func(ctx, obj, "label", js_widget_label, 1);
    set_func(ctx, obj, "data", js_widget_data, 1);
    set_func(ctx, obj, "items", js_widget_items, 1);
    set_func(ctx, obj, "rows", js_widget_rows, 1);
    set_func(ctx, obj, "columns", js_widget_columns, 1);
    set_func(ctx, obj, "select", js_widget_select, 1);
    set_func(ctx, obj, "marker", js_widget_marker, 1);
    set_func(ctx, obj, "accent", js_widget_accent, 1);
    set_func(ctx, obj, "frame", js_widget_frame, 1);
    set_func(ctx, obj, "title", js_widget_title, 1);
    set_func(ctx, obj, "grid", js_widget_grid_cols, 1);
    set_func(ctx, obj, "size", js_widget_size, 2);
    set_func(ctx, obj, "width", js_widget_width, 1);
    set_func(ctx, obj, "range", js_widget_noop, 2);
    set_func(ctx, obj, "glyphs", js_widget_noop, 1);
    set_func(ctx, obj, "sortBy", js_widget_noop, 2);
    set_func(ctx, obj, "onPick", js_widget_noop, 1);
    set_func(ctx, obj, "cell", js_widget_cell, 1);
    set_func(ctx, obj, "layer", js_widget_layer, 3);
    set_func(ctx, obj, "render", js_widget_noop, 1);
    return obj;
}

JSValue make_layout_object(JSContext *ctx, LayoutBuilder *layout)
{
    JSValue obj = JS_NewObjectClass(ctx, g_layout_class);
    JS_SetOpaque(obj, layout);
    JSValue row = JS_NewCFunctionMagic(ctx, js_layout_add, "row", 2, JS_CFUNC_generic_magic, 0);
    JS_SetPropertyStr(ctx, obj, "row", row);
    JSValue col = JS_NewCFunctionMagic(ctx, js_layout_add, "col", 2, JS_CFUNC_generic_magic, 1);
    JS_SetPropertyStr(ctx, obj, "col", col);
    return obj;
}

void ensure_class_ids()
{
    if (!g_os_class) JS_NewClassID(&g_os_class);
    if (!g_app_class) JS_NewClassID(&g_app_class);
    if (!g_panel_class) JS_NewClassID(&g_panel_class);
    if (!g_text_class) JS_NewClassID(&g_text_class);
    if (!g_gauge_class) JS_NewClassID(&g_gauge_class);
    if (!g_layout_class) JS_NewClassID(&g_layout_class);
    if (!g_widget_class) JS_NewClassID(&g_widget_class);
}

JSValue js_os_clock(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    std::string fmt = argc > 0 ? js_to_string(ctx, argv[0]) : "HH:mm";
    uint32_t sec = 12 * 3600;
    const int64_t now_us = esp_log_timestamp();
    sec += (uint32_t)(now_us / 1000);
    const uint32_t h = (sec / 3600) % 24;
    const uint32_t m = (sec / 60) % 60;
    const uint32_t s = sec % 60;
    char hh[3], mm[3], ss[3];
    std::snprintf(hh, sizeof(hh), "%02u", (unsigned)h);
    std::snprintf(mm, sizeof(mm), "%02u", (unsigned)m);
    std::snprintf(ss, sizeof(ss), "%02u", (unsigned)s);
    size_t pos = 0;
    while ((pos = fmt.find("HH", pos)) != std::string::npos) { fmt.replace(pos, 2, hh); pos += 2; }
    pos = 0;
    while ((pos = fmt.find("mm", pos)) != std::string::npos) { fmt.replace(pos, 2, mm); pos += 2; }
    pos = 0;
    while ((pos = fmt.find("ss", pos)) != std::string::npos) { fmt.replace(pos, 2, ss); pos += 2; }
    return JS_NewString(ctx, fmt.c_str());
}

JSValue js_os_history(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    int n = 26;
    if (argc > 1) { int32_t parsed = 26; JS_ToInt32(ctx, &parsed, argv[1]); n = std::max<int32_t>(1, parsed); }
    n = std::min(n, 64);
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0; i < n; ++i) JS_SetPropertyUint32(ctx, arr, i, JS_NewInt32(ctx, 30 + ((i * 17) % 60)));
    return arr;
}

JSValue js_os_processes(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    struct Proc { int pid; const char *name; int cpu; int mem; } procs[] = {
        {1, "kernel", 2, 18}, {7, "ui", 11, 42}, {12, "music", 48, 31}, {19, "netd", 4, 9}, {23, "shell", 1, 6},
    };
    JSValue arr = JS_NewArray(ctx);
    for (uint32_t i = 0; i < sizeof(procs) / sizeof(procs[0]); ++i) {
        JSValue p = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, p, "pid", JS_NewInt32(ctx, procs[i].pid));
        JS_SetPropertyStr(ctx, p, "name", JS_NewString(ctx, procs[i].name));
        JS_SetPropertyStr(ctx, p, "cpu", JS_NewInt32(ctx, procs[i].cpu));
        JS_SetPropertyStr(ctx, p, "mem", JS_NewInt32(ctx, procs[i].mem));
        JS_SetPropertyUint32(ctx, arr, i, p);
    }
    return arr;
}

JSValue js_os_launch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    std::string name = argc > 0 ? js_to_string(ctx, argv[0]) : "";
    std::string msg = "launch -> " + name;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue print = JS_GetPropertyStr(ctx, global, "print");
    if (JS_IsFunction(ctx, print)) {
        JSValue arg = JS_NewString(ctx, msg.c_str());
        JSValue ret = JS_Call(ctx, print, JS_UNDEFINED, 1, &arg);
        JS_FreeValue(ctx, ret);
        JS_FreeValue(ctx, arg);
    }
    JS_FreeValue(ctx, print);
    JS_FreeValue(ctx, global);
    return JS_UNDEFINED;
}

JSValue js_os_eval_expr(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_NewFloat64(ctx, 0);
    std::string expr = js_to_string(ctx, argv[0]);
    JSValue ret = JS_Eval(ctx, expr.c_str(), expr.size(), "<picojs-os-eval>", JS_EVAL_TYPE_GLOBAL);
    return ret;
}

JSValue js_os_noop(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    return JS_UNDEFINED;
}

void install_os_compat(JSContext *ctx, JSValue os)
{
    JS_SetPropertyStr(ctx, os, "battery", JS_NewInt32(ctx, 73));
    JSValue metrics = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, metrics, "cpu", JS_NewInt32(ctx, 62));
    JS_SetPropertyStr(ctx, metrics, "mem", JS_NewInt32(ctx, 41));
    JS_SetPropertyStr(ctx, metrics, "tmp", JS_NewInt32(ctx, 48));
    JS_SetPropertyStr(ctx, os, "metrics", metrics);
    JSValue cfg = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, cfg, "bright", JS_NewInt32(ctx, 80));
    JS_SetPropertyStr(ctx, cfg, "theme", JS_NewString(ctx, "amber"));
    JS_SetPropertyStr(ctx, cfg, "font", JS_NewString(ctx, "6x8"));
    JS_SetPropertyStr(ctx, cfg, "haptics", JS_NewBool(ctx, true));
    JS_SetPropertyStr(ctx, cfg, "echo", JS_NewBool(ctx, false));
    JS_SetPropertyStr(ctx, cfg, "sleep", JS_NewString(ctx, "2 min"));
    JS_SetPropertyStr(ctx, os, "cfg", cfg);
    JSValue food = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, food, "x", JS_NewInt32(ctx, 12));
    JS_SetPropertyStr(ctx, food, "y", JS_NewInt32(ctx, 3));
    JS_SetPropertyStr(ctx, os, "food", food);
    set_func(ctx, os, "clock", js_os_clock, 1);
    set_func(ctx, os, "history", js_os_history, 2);
    set_func(ctx, os, "processes", js_os_processes, 0);
    set_func(ctx, os, "launch", js_os_launch, 1);
    set_func(ctx, os, "eval", js_os_eval_expr, 1);
    set_func(ctx, os, "reset", js_os_noop, 0);
    set_func(ctx, os, "step", js_os_noop, 0);
    set_func(ctx, os, "turn", js_os_noop, 1);
}

esp_err_t register_classes(JSRuntime *js_rt)
{
    ensure_class_ids();
    JSClassDef os_def = {}; os_def.class_name = "PicoJSOS";
    JSClassDef app_def = {}; app_def.class_name = "PicoJSApp";
    JSClassDef panel_def = {}; panel_def.class_name = "PicoJSPanel";
    JSClassDef text_def = {}; text_def.class_name = "PicoJSText";
    JSClassDef gauge_def = {}; gauge_def.class_name = "PicoJSGauge";
    JSClassDef layout_def = {}; layout_def.class_name = "PicoJSLayout";
    JSClassDef widget_def = {}; widget_def.class_name = "PicoJSWidget";
    if (!JS_IsRegisteredClass(js_rt, g_os_class) && JS_NewClass(js_rt, g_os_class, &os_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_app_class) && JS_NewClass(js_rt, g_app_class, &app_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_panel_class) && JS_NewClass(js_rt, g_panel_class, &panel_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_text_class) && JS_NewClass(js_rt, g_text_class, &text_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_gauge_class) && JS_NewClass(js_rt, g_gauge_class, &gauge_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_layout_class) && JS_NewClass(js_rt, g_layout_class, &layout_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_widget_class) && JS_NewClass(js_rt, g_widget_class, &widget_def) < 0) return ESP_FAIL;
    return ESP_OK;
}
} // namespace

esp_err_t picojs_runtime_create(const picojs_runtime_config_t *cfg, picojs_runtime_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = nullptr;
    auto rt = std::make_unique<picojs_runtime>();
    if (cfg) {
        rt->cols = cfg->cols ? cfg->cols : PICOJS_RUNTIME_DEFAULT_COLS;
        rt->rows = cfg->rows ? cfg->rows : PICOJS_RUNTIME_DEFAULT_ROWS;
        rt->frame_interval_ms = cfg->frame_interval_ms ? cfg->frame_interval_ms : kDefaultFrameIntervalMs;
    }
    if (rt->cols == 0 || rt->rows == 0 || static_cast<size_t>(rt->cols) * rt->rows > kMaxCells) return ESP_ERR_INVALID_ARG;
    rt->initialized = true;
    render_banner(rt.get());
    ESP_LOGI(kTag, "runtime initialized: %ux%u cells frame_interval=%ums", rt->cols, rt->rows, (unsigned)rt->frame_interval_ms);
    *out = rt.release();
    return ESP_OK;
}

void picojs_runtime_destroy(picojs_runtime_t *rt) { delete rt; }

esp_err_t picojs_runtime_install(JSContext *ctx, picojs_runtime_t *rt)
{
    if (!ctx || !rt || !rt->initialized) return ESP_ERR_INVALID_ARG;
    esp_err_t class_err = register_classes(JS_GetRuntime(ctx));
    if (class_err != ESP_OK) return class_err;
    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) return ESP_FAIL;
    JSValue os = JS_NewObjectClass(ctx, g_os_class);
    if (JS_IsException(os)) { JS_FreeValue(ctx, global); return ESP_FAIL; }
    JS_SetOpaque(os, rt);
    set_func(ctx, os, "app", js_os_app, 1);
    install_os_compat(ctx, os);
    if (JS_SetPropertyStr(ctx, global, "OS", os) < 0) {
        JS_FreeValue(ctx, os);
        JS_FreeValue(ctx, global);
        return ESP_FAIL;
    }
    JS_FreeValue(ctx, global);
    rt->js_installed = true;
    ESP_LOGI(kTag, "installed OS native object into QuickJS context");
    return ESP_OK;
}

esp_err_t picojs_runtime_reset(picojs_runtime_t *rt)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    rt->app.reset();
    rt->app_count = 0;
    rt->mounted_app_count = 0;
    rt->frame_count = 0;
    rt->last_frame_ms = 0;
    rt->last_error_count = 0;
    rt->last_key[0] = 0;
    rt->app_mode = false;
    rt->js_installed = false;
    render_banner(rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_get_status(picojs_runtime_t *rt, picojs_runtime_status_t *out)
{
    if (!rt || !out) return ESP_ERR_INVALID_ARG;
    *out = {};
    out->initialized = rt->initialized;
    out->js_installed = rt->js_installed;
    out->app_mode = rt->app_mode;
    out->cols = rt->cols;
    out->rows = rt->rows;
    out->frame_count = rt->frame_count;
    out->app_count = rt->app_count;
    out->mounted_app_count = rt->mounted_app_count;
    out->last_frame_ms = rt->last_frame_ms;
    out->last_error_count = rt->last_error_count;
    return ESP_OK;
}

esp_err_t picojs_runtime_frame_js(JSContext *ctx, picojs_runtime_t *rt, uint32_t dt_ms)
{
    if (!ctx || !rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    rt->last_frame_ms = dt_ms;
    ++rt->frame_count;
    run_callbacks(ctx, rt, dt_ms);
    render_app(ctx, rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_frame(picojs_runtime_t *rt, uint32_t dt_ms)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    rt->last_frame_ms = dt_ms;
    ++rt->frame_count;
    render_app(nullptr, rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_key_js(JSContext *ctx, picojs_runtime_t *rt, const char *token)
{
    if (!ctx || !rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!token || token[0] == 0) return ESP_ERR_INVALID_ARG;
    std::snprintf(rt->last_key, sizeof(rt->last_key), "%s", token);
    if (rt->app) {
        for (auto &key : rt->app->keys) {
            if (key.token == token) {
                JSValue app_obj = make_app_object(ctx, rt->app.get());
                JSValue tok = JS_NewString(ctx, token);
                JSValue argv[2] = {app_obj, tok};
                JSValue ret = JS_Call(ctx, key.fn.value, JS_UNDEFINED, 2, argv);
                if (JS_IsException(ret)) ++rt->last_error_count;
                JS_FreeValue(ctx, ret);
                JS_FreeValue(ctx, tok);
                JS_FreeValue(ctx, app_obj);
                break;
            }
        }
    }
    render_app(ctx, rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_key(picojs_runtime_t *rt, const char *token)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!token || token[0] == 0) return ESP_ERR_INVALID_ARG;
    std::snprintf(rt->last_key, sizeof(rt->last_key), "%s", token);
    render_app(nullptr, rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_set_app_mode(picojs_runtime_t *rt, bool enabled)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    rt->app_mode = enabled;
    return ESP_OK;
}

bool picojs_runtime_app_mode(picojs_runtime_t *rt)
{
    return rt && rt->initialized && rt->app_mode;
}

esp_err_t picojs_runtime_dump_text(picojs_runtime_t *rt, char *dst, size_t dst_len)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!dst || dst_len == 0) return ESP_ERR_INVALID_ARG;
    const size_t row_dump_len = 5 + rt->cols + 1;
    const size_t required = static_cast<size_t>(rt->rows) * row_dump_len + 1;
    if (dst_len < required) return ESP_ERR_INVALID_SIZE;
    char *out = dst;
    size_t remaining = dst_len;
    for (uint16_t row = 0; row < rt->rows; ++row) {
        const int written = std::snprintf(out, remaining, "[%02u] ", row);
        if (written < 0 || static_cast<size_t>(written) >= remaining) return ESP_ERR_INVALID_SIZE;
        out += written;
        remaining -= written;
        for (uint16_t col = 0; col < rt->cols; ++col) {
            if (remaining <= 1) return ESP_ERR_INVALID_SIZE;
            *out++ = rt->cells[row * rt->cols + col].ch;
            --remaining;
        }
        if (remaining <= 1) return ESP_ERR_INVALID_SIZE;
        *out++ = '\n';
        --remaining;
        *out = 0;
    }
    return ESP_OK;
}
