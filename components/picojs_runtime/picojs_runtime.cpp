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

struct ScreenCell {
    char ch = ' ';
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
    std::string value;
    int x = 0;
    int y = 0;
    std::string x_align;
    std::string fg;
    bool bold = false;
};

struct GaugeWidget {
    Panel *panel = nullptr;
    int x = 0;
    int y = 0;
    int width = 16;
    std::string label;
    std::string source;
    int value = 0;
    bool show_pct = false;
};

struct Panel {
    App *app = nullptr;
    std::string id;
    std::string frame;
    std::string title;
    std::vector<std::unique_ptr<TextWidget>> texts;
    std::vector<std::unique_ptr<GaugeWidget>> gauges;
};

struct App {
    struct picojs_runtime *rt = nullptr;
    std::string name;
    bool mounted = false;
    std::string statusbar;
    char layout_axis = 0;
    std::vector<LayoutSegment> layout_segments;
    std::vector<std::unique_ptr<Panel>> panels;
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
    for (int col = x; col < rt->cols && *text; ++col, ++text) {
        put_char(rt, col, y, *text);
    }
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

Rect panel_rect(const App *app, const Panel *panel)
{
    const picojs_runtime *rt = app ? app->rt : nullptr;
    Rect fallback;
    fallback.w = rt ? rt->cols : PICOJS_RUNTIME_DEFAULT_COLS;
    fallback.h = rt ? std::max<int>(1, rt->rows - 1) : PICOJS_RUNTIME_DEFAULT_ROWS - 1;
    if (!app || !panel) return fallback;
    for (const auto &segment : app->layout_segments) {
        if (segment.id == panel->id) return segment.rect;
    }
    return fallback;
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
        if (app->layout_axis == 'c') {
            segment.rect = {pos, 0, sz, std::max<int>(1, app->rt->rows - 1)};
        } else {
            segment.rect = {0, pos, app->rt->cols, sz};
        }
        pos += sz;
    }
}

int gauge_value(const picojs_runtime *rt, const GaugeWidget *gauge)
{
    if (!gauge) return 0;
    if (gauge->source == "battery") return 72 + (int)((rt ? rt->frame_count : 0) % 7);
    return gauge->value;
}

void draw_gauge(picojs_runtime *rt, const GaugeWidget *gauge, int inner_x, int inner_y, int inner_w, int inner_h)
{
    if (!rt || !gauge) return;
    const int y = inner_y + gauge->y;
    if (y < inner_y || y >= inner_y + inner_h) return;
    const int x = inner_x + gauge->x;
    const int bar_w = std::max(4, std::min(gauge->width, inner_w - gauge->x - 8));
    const int value = std::max(0, std::min(100, gauge_value(rt, gauge)));
    char line[96] = {};
    const int filled = (bar_w * value) / 100;
    char bar[48] = {};
    const int capped_w = std::min<int>(bar_w, sizeof(bar) - 1);
    for (int i = 0; i < capped_w; ++i) bar[i] = i < filled ? '#' : '-';
    bar[capped_w] = 0;
    if (gauge->show_pct) {
        std::snprintf(line, sizeof(line), "%s[%s] %d%%", gauge->label.c_str(), bar, value);
    } else {
        std::snprintf(line, sizeof(line), "%s[%s]", gauge->label.c_str(), bar);
    }
    put_text(rt, x, y, line);
}

void render_app(picojs_runtime *rt)
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
            if (!panel->title.empty()) {
                put_text(rt, rect.x + 2, rect.y, panel->title.c_str());
            }
        }
        for (const auto &text_ptr : panel->texts) {
            const TextWidget *text = text_ptr.get();
            const int y = inner_y + text->y;
            if (y < inner_y || y >= inner_y + inner_h) continue;
            if (text->x_align == "center") {
                const int len = (int)std::min<size_t>(text->value.size(), inner_w);
                const int x = inner_x + std::max(0, (inner_w - len) / 2);
                put_text(rt, x, y, text->value.c_str());
            } else {
                put_text(rt, inner_x + text->x, y, text->value.c_str());
            }
        }
        for (const auto &gauge_ptr : panel->gauges) {
            draw_gauge(rt, gauge_ptr.get(), inner_x, inner_y, inner_w, inner_h);
        }
    }
    if (!rt->app->statusbar.empty()) {
        put_text(rt, 0, rt->rows - 1, rt->app->statusbar.c_str());
    }
}

std::string js_to_string(JSContext *ctx, JSValueConst v)
{
    const char *s = JS_ToCString(ctx, v);
    if (!s) return {};
    std::string out(s);
    JS_FreeCString(ctx, s);
    return out;
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
    LayoutBuilder layout;
    layout.app = app;
    JSValue layout_obj = make_layout_object(ctx, &layout);
    JSValue ret = JS_Call(ctx, argv[0], JS_UNDEFINED, 1, &layout_obj);
    if (JS_IsException(ret)) {
        ++app->rt->last_error_count;
    }
    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, layout_obj);
    recompute_layout(app);
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_statusbar(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (app && argc > 0) app->statusbar = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_app_mount(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *app = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
    if (app && app->rt) {
        app->mounted = true;
        app->rt->mounted_app_count = 1;
        render_app(app->rt);
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
    if (panel && argc > 0) panel->title = js_to_string(ctx, argv[0]);
    return JS_DupValue(ctx, this_val);
}

JSValue js_panel_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    auto *panel = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class));
    if (!panel) return JS_ThrowTypeError(ctx, "bad panel object");
    auto text = std::make_unique<TextWidget>();
    text->panel = panel;
    text->value = argc > 0 ? js_to_string(ctx, argv[0]) : "";
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
        if (JS_IsString(argv[0])) {
            text->x_align = js_to_string(ctx, argv[0]);
        } else {
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
        if (JS_IsString(argv[0])) {
            gauge->source = js_to_string(ctx, argv[0]);
        } else {
            int32_t value = 0;
            JS_ToInt32(ctx, &value, argv[0]);
            gauge->value = (int)value;
            gauge->source.clear();
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

JSValue js_gauge_show_pct(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    auto *gauge = static_cast<GaugeWidget *>(JS_GetOpaque(this_val, g_gauge_class));
    if (gauge) gauge->show_pct = true;
    return JS_DupValue(ctx, this_val);
}

JSValue make_app_object(JSContext *ctx, App *app)
{
    JSValue obj = JS_NewObjectClass(ctx, g_app_class);
    JS_SetOpaque(obj, app);
    set_func(ctx, obj, "layout", js_app_layout, 1);
    set_func(ctx, obj, "panel", js_app_panel, 1);
    set_func(ctx, obj, "statusbar", js_app_statusbar, 1);
    set_func(ctx, obj, "mount", js_app_mount, 0);
    return obj;
}

JSValue make_panel_object(JSContext *ctx, Panel *panel)
{
    JSValue obj = JS_NewObjectClass(ctx, g_panel_class);
    JS_SetOpaque(obj, panel);
    set_func(ctx, obj, "frame", js_panel_frame, 1);
    set_func(ctx, obj, "title", js_panel_title, 1);
    set_func(ctx, obj, "text", js_panel_text, 1);
    set_func(ctx, obj, "gauge", js_panel_gauge, 0);
    return obj;
}

JSValue make_text_object(JSContext *ctx, TextWidget *text)
{
    JSValue obj = JS_NewObjectClass(ctx, g_text_class);
    JS_SetOpaque(obj, text);
    set_func(ctx, obj, "at", js_text_at, 2);
    set_func(ctx, obj, "fg", js_text_fg, 1);
    set_func(ctx, obj, "bold", js_text_bold, 0);
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
    set_func(ctx, obj, "showPct", js_gauge_show_pct, 0);
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
}

esp_err_t register_classes(JSRuntime *js_rt)
{
    ensure_class_ids();
    JSClassDef os_def = {};
    os_def.class_name = "PicoJSOS";
    JSClassDef app_def = {};
    app_def.class_name = "PicoJSApp";
    JSClassDef panel_def = {};
    panel_def.class_name = "PicoJSPanel";
    JSClassDef text_def = {};
    text_def.class_name = "PicoJSText";
    JSClassDef gauge_def = {};
    gauge_def.class_name = "PicoJSGauge";
    JSClassDef layout_def = {};
    layout_def.class_name = "PicoJSLayout";
    if (!JS_IsRegisteredClass(js_rt, g_os_class) && JS_NewClass(js_rt, g_os_class, &os_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_app_class) && JS_NewClass(js_rt, g_app_class, &app_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_panel_class) && JS_NewClass(js_rt, g_panel_class, &panel_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_text_class) && JS_NewClass(js_rt, g_text_class, &text_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_gauge_class) && JS_NewClass(js_rt, g_gauge_class, &gauge_def) < 0) return ESP_FAIL;
    if (!JS_IsRegisteredClass(js_rt, g_layout_class) && JS_NewClass(js_rt, g_layout_class, &layout_def) < 0) return ESP_FAIL;
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
    if (rt->cols == 0 || rt->rows == 0 || static_cast<size_t>(rt->cols) * rt->rows > kMaxCells) {
        return ESP_ERR_INVALID_ARG;
    }

    rt->initialized = true;
    render_banner(rt.get());
    ESP_LOGI(kTag, "runtime initialized: %ux%u cells frame_interval=%ums",
             rt->cols, rt->rows, (unsigned)rt->frame_interval_ms);
    *out = rt.release();
    return ESP_OK;
}

void picojs_runtime_destroy(picojs_runtime_t *rt)
{
    delete rt;
}

esp_err_t picojs_runtime_install(JSContext *ctx, picojs_runtime_t *rt)
{
    if (!ctx || !rt || !rt->initialized) return ESP_ERR_INVALID_ARG;
    JSRuntime *js_rt = JS_GetRuntime(ctx);
    esp_err_t class_err = register_classes(js_rt);
    if (class_err != ESP_OK) return class_err;

    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) return ESP_FAIL;
    JSValue os = JS_NewObjectClass(ctx, g_os_class);
    if (JS_IsException(os)) {
        JS_FreeValue(ctx, global);
        return ESP_FAIL;
    }
    JS_SetOpaque(os, rt);
    set_func(ctx, os, "app", js_os_app, 1);
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
    out->cols = rt->cols;
    out->rows = rt->rows;
    out->frame_count = rt->frame_count;
    out->app_count = rt->app_count;
    out->mounted_app_count = rt->mounted_app_count;
    out->last_frame_ms = rt->last_frame_ms;
    out->last_error_count = rt->last_error_count;
    return ESP_OK;
}

esp_err_t picojs_runtime_frame(picojs_runtime_t *rt, uint32_t dt_ms)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    rt->last_frame_ms = dt_ms;
    ++rt->frame_count;
    render_app(rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_key(picojs_runtime_t *rt, const char *token)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!token || token[0] == 0) return ESP_ERR_INVALID_ARG;
    std::snprintf(rt->last_key, sizeof(rt->last_key), "%s", token);
    render_app(rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_dump_text(picojs_runtime_t *rt, char *dst, size_t dst_len)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!dst || dst_len == 0) return ESP_ERR_INVALID_ARG;

    const size_t row_dump_len = 5 + rt->cols + 1; // "[NN] " + cells + "\n"
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
