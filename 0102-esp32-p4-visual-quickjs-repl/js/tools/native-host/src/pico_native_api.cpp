#include "pico_native_api.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace pico_native {

using Clock = std::chrono::steady_clock;
const auto g_start = Clock::now();

struct Style {
  std::string fg = "fg";
  bool bold = false;
  bool dim = false;
};

struct Cell {
  std::string ch = " ";
  Style st;
};

struct Screen {
  int cols = 40;
  int rows = 30;
  std::vector<Cell> cells;
  Screen(int c, int r) : cols(c), rows(r), cells((size_t)c * (size_t)r) {}
  void clear() { for (auto &c : cells) { c.ch = " "; c.st = {}; } }
  bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < cols && y < rows; }
  void set(int x, int y, const std::string &ch, const Style &st = {}) {
    if (!in_bounds(x, y)) return;
    cells[(size_t)y * (size_t)cols + (size_t)x] = {ch.empty() ? " " : ch, st};
  }
  void text(int x, int y, const std::string &s, const Style &st = {}) {
    // Good enough for the box-drawing/BMP examples: advance by UTF-8 codepoint.
    int cx = x;
    for (size_t i = 0; i < s.size();) {
      unsigned char b = static_cast<unsigned char>(s[i]);
      size_t n = 1;
      if ((b & 0x80) == 0) n = 1;
      else if ((b & 0xE0) == 0xC0) n = 2;
      else if ((b & 0xF0) == 0xE0) n = 3;
      else if ((b & 0xF8) == 0xF0) n = 4;
      set(cx++, y, s.substr(i, std::min(n, s.size() - i)), st);
      i += n;
    }
  }
  void hline(int x, int y, int w, const std::string &ch, const Style &st = {}) { for (int i = 0; i < w; ++i) set(x + i, y, ch, st); }
  void vline(int x, int y, int h, const std::string &ch, const Style &st = {}) { for (int i = 0; i < h; ++i) set(x, y + i, ch, st); }
  void box(int x, int y, int w, int h, const std::string &frame, const Style &st = {}) {
    std::vector<std::string> f;
    std::string src = frame == "rounded" ? "╭╮╰╯─│" : frame == "double" ? "╔╗╚╝═║" : "┌┐└┘─│";
    for (size_t i = 0; i < src.size();) {
      unsigned char b = static_cast<unsigned char>(src[i]);
      size_t n = (b & 0x80) == 0 ? 1 : ((b & 0xE0) == 0xC0 ? 2 : ((b & 0xF0) == 0xE0 ? 3 : 4));
      f.push_back(src.substr(i, n)); i += n;
    }
    if (w <= 0 || h <= 0 || f.size() < 6) return;
    set(x, y, f[0], st); set(x + w - 1, y, f[1], st);
    set(x, y + h - 1, f[2], st); set(x + w - 1, y + h - 1, f[3], st);
    hline(x + 1, y, w - 2, f[4], st); hline(x + 1, y + h - 1, w - 2, f[4], st);
    vline(x, y + 1, h - 2, f[5], st); vline(x + w - 1, y + 1, h - 2, f[5], st);
  }
  std::string render() const {
    std::ostringstream oss;
    for (int y = 0; y < rows; ++y) {
      int last = cols - 1;
      while (last >= 0 && cells[(size_t)y * (size_t)cols + (size_t)last].ch == " ") --last;
      for (int x = 0; x <= last; ++x) oss << cells[(size_t)y * (size_t)cols + (size_t)x].ch;
      if (y + 1 < rows) oss << '\n';
    }
    return oss.str();
  }
};

struct Runtime;
struct App;
struct Panel;
struct Widget;

struct Rect { int x = 0, y = 0, w = 0, h = 0; };

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
  bool empty() const { return !ctx || JS_IsUndefined(value); }
};

struct Widget {
  enum Type { TEXT, GAUGE } type = TEXT;
  Panel *panel = nullptr;
  int x = 0, y = 0;
  std::string x_align;
  Style style;
  StoredValue value;
  std::string label;
  int width = 10;
  bool show_pct = false;
};

struct Timer {
  int ms = 0;
  int acc = 0;
  StoredValue fn;
  Timer() = default;
  Timer(int ms_, int acc_, StoredValue fn_) : ms(ms_), acc(acc_), fn(std::move(fn_)) {}
  Timer(Timer &&) noexcept = default;
  Timer &operator=(Timer &&) noexcept = default;
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;
};

struct Panel {
  App *app = nullptr;
  Rect rect;
  std::string frame;
  StoredValue title;
  StoredValue title_right;
  std::vector<std::unique_ptr<Widget>> widgets;
};

struct App {
  Runtime *rt = nullptr;
  std::string name;
  std::map<std::string, std::unique_ptr<Panel>> panels;
  std::vector<std::unique_ptr<Panel>> panel_order;
  std::vector<Timer> timers;
  std::map<std::string, StoredValue> keys;
  StoredValue statusbar;
  bool mounted = false;
  bool exited = false;
};

struct Runtime {
  JSRuntime *jsrt = nullptr;
  JSContext *ctx = nullptr;
  Screen screen;
  std::unique_ptr<App> app;
  double battery = 64;
  std::string toast;
  Runtime(int c, int r) : screen(c, r) {}
};

JSClassID g_app_class;
JSClassID g_panel_class;
JSClassID g_widget_class;
Runtime *g_current_runtime = nullptr;

std::string js_to_string(JSContext *ctx, JSValueConst v) {
  const char *s = JS_ToCString(ctx, v);
  if (!s) return "";
  std::string out(s);
  JS_FreeCString(ctx, s);
  return out;
}

double js_to_double(JSContext *ctx, JSValueConst v) {
  double d = 0;
  JS_ToFloat64(ctx, &d, v);
  return d;
}

std::string resolve_string(Runtime *rt, const StoredValue &v) {
  if (!v.ctx || JS_IsUndefined(v.value)) return "";
  JSContext *ctx = rt->ctx;
  if (JS_IsFunction(ctx, v.value)) {
    JSValue r = JS_Call(ctx, v.value, JS_UNDEFINED, 0, nullptr);
    std::string out = js_to_string(ctx, r);
    JS_FreeValue(ctx, r);
    return out;
  }
  return js_to_string(ctx, v.value);
}

double resolve_number(Runtime *rt, const StoredValue &v) {
  if (!v.ctx || JS_IsUndefined(v.value)) return 0;
  JSContext *ctx = rt->ctx;
  if (JS_IsFunction(ctx, v.value)) {
    JSValue r = JS_Call(ctx, v.value, JS_UNDEFINED, 0, nullptr);
    double out = js_to_double(ctx, r);
    JS_FreeValue(ctx, r);
    return out;
  }
  return js_to_double(ctx, v.value);
}

int resolve_x(const Widget &w, int content_w, int text_w) {
  if (w.x_align == "center") return std::max(0, (content_w - text_w) / 2);
  if (w.x_align == "right") return std::max(0, content_w - text_w);
  return w.x;
}

std::string pad_int(int v, int width, char fill = '0') {
  std::string s = std::to_string(v);
  while ((int)s.size() < width) s.insert(s.begin(), fill);
  return s;
}

void draw_panel(Runtime *rt, Panel *p) {
  Screen &s = rt->screen;
  if (!p->frame.empty()) s.box(p->rect.x, p->rect.y, p->rect.w, p->rect.h, p->frame, {.fg = "dim"});
  if (!JS_IsUndefined(p->title.value)) s.text(p->rect.x + 2, p->rect.y, " " + resolve_string(rt, p->title) + " ", {.fg = "white", .bold = true});
  if (!JS_IsUndefined(p->title_right.value)) {
    std::string t = " " + resolve_string(rt, p->title_right) + " ";
    s.text(p->rect.x + p->rect.w - 2 - (int)t.size(), p->rect.y, t, {.fg = "amber"});
  }
  Rect c = p->frame.empty() ? p->rect : Rect{p->rect.x + 1, p->rect.y + 1, p->rect.w - 2, p->rect.h - 2};
  for (auto &wp : p->widgets) {
    Widget &w = *wp;
    if (w.type == Widget::TEXT) {
      std::string txt = resolve_string(rt, w.value);
      int x = c.x + resolve_x(w, c.w, (int)txt.size());
      s.text(x, c.y + w.y, txt, w.style);
    } else if (w.type == Widget::GAUGE) {
      double val = std::clamp(resolve_number(rt, w.value), 0.0, 100.0);
      int fill = (int)std::round((val / 100.0) * w.width);
      std::string bar((size_t)fill, '#');
      bar += std::string((size_t)std::max(0, w.width - fill), '-');
      std::string line = w.label.empty() ? bar : w.label + " " + bar;
      if (w.show_pct) line += " " + pad_int((int)std::round(val), 2, ' ') + "%";
      s.text(c.x + w.x, c.y + w.y, line, {.fg = "green"});
    }
  }
}

void render_app(Runtime *rt) {
  rt->screen.clear();
  if (!rt->app || !rt->app->mounted) return;
  for (auto &p : rt->app->panel_order) draw_panel(rt, p.get());
  if (!JS_IsUndefined(rt->app->statusbar.value)) {
    int y = rt->screen.rows - 1;
    rt->screen.hline(0, y, rt->screen.cols, " ");
    rt->screen.text(1, y, resolve_string(rt, rt->app->statusbar), {.fg = "dim"});
  }
}

JSValue js_print(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
  for (int i = 0; i < argc; ++i) {
    if (i) std::fputc(' ', stdout);
    std::string s = js_to_string(ctx, argv[i]);
    std::fputs(s.c_str(), stdout);
  }
  std::fputc('\n', stdout);
  return JS_UNDEFINED;
}

JSValue js_millis(JSContext *ctx, JSValueConst, int, JSValueConst *) { return JS_NewInt64(ctx, (int64_t)host_millis()); }
JSValue js_gc(JSContext *ctx, JSValueConst, int, JSValueConst *) { JS_RunGC(JS_GetRuntime(ctx)); return JS_UNDEFINED; }

JSValue js_os_clock(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
  std::string fmt = argc > 0 ? js_to_string(ctx, argv[0]) : "HH:mm";
  uint64_t sec = host_millis() / 1000 + 14 * 3600 + 32 * 60;
  int h = (int)((sec / 3600) % 24), m = (int)((sec / 60) % 60), s = (int)(sec % 60);
  auto replace = [](std::string str, const std::string &from, const std::string &to) {
    size_t p = str.find(from); if (p != std::string::npos) str.replace(p, from.size(), to); return str;
  };
  fmt = replace(fmt, "HH", pad_int(h, 2)); fmt = replace(fmt, "mm", pad_int(m, 2)); fmt = replace(fmt, "ss", pad_int(s, 2));
  return JS_NewString(ctx, fmt.c_str());
}

JSValue js_os_launch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
  if (g_current_runtime && argc > 0) {
    g_current_runtime->toast = "launch -> " + js_to_string(ctx, argv[0]);
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue os = JS_GetPropertyStr(ctx, global, "OS");
    JS_SetPropertyStr(ctx, os, "toast", JS_NewString(ctx, g_current_runtime->toast.c_str()));
    JS_FreeValue(ctx, os); JS_FreeValue(ctx, global);
  }
  return JS_UNDEFINED;
}

JSValue make_app_object(JSContext *ctx, App *app);
JSValue make_panel_object(JSContext *ctx, Panel *panel);
JSValue make_widget_object(JSContext *ctx, Widget *widget);

JSValue js_os_app(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
  Runtime *rt = g_current_runtime;
  if (!rt) return JS_ThrowInternalError(ctx, "no runtime");
  auto app = std::make_unique<App>();
  app->rt = rt;
  app->name = argc ? js_to_string(ctx, argv[0]) : "app";
  rt->app = std::move(app);
  return make_app_object(ctx, rt->app.get());
}

JSValue js_app_state(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) { return argc ? JS_DupValue(ctx, argv[0]) : JS_NewObject(ctx); }
JSValue js_app_mount(JSContext *ctx, JSValueConst this_val, int, JSValueConst *) { auto *a = static_cast<App *>(JS_GetOpaque(this_val, g_app_class)); if (a) a->mounted = true; return JS_DupValue(ctx, this_val); }
JSValue js_app_exit(JSContext *ctx, JSValueConst this_val, int, JSValueConst *) { auto *a = static_cast<App *>(JS_GetOpaque(this_val, g_app_class)); if (a) a->exited = true; return JS_DupValue(ctx, this_val); }
JSValue js_app_statusbar(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *a = static_cast<App *>(JS_GetOpaque(this_val, g_app_class)); if (a && argc) a->statusbar = StoredValue(ctx, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue js_app_on(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  auto *a = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
  if (a && argc >= 3 && js_to_string(ctx, argv[0]) == "tick") {
    int32_t ms = 0; JS_ToInt32(ctx, &ms, argv[1]);
    a->timers.emplace_back(ms, 0, StoredValue(ctx, argv[2]));
  }
  return JS_DupValue(ctx, this_val);
}
JSValue js_app_key(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  auto *a = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
  if (a && argc >= 2) a->keys[js_to_string(ctx, argv[0])] = StoredValue(ctx, argv[1]);
  return JS_DupValue(ctx, this_val);
}
JSValue js_app_panel(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  auto *a = static_cast<App *>(JS_GetOpaque(this_val, g_app_class));
  if (!a) return JS_ThrowTypeError(ctx, "bad app");
  std::string id = argc ? js_to_string(ctx, argv[0]) : "main";
  auto p = std::make_unique<Panel>();
  p->app = a;
  p->rect = {0, 0, a->rt->screen.cols, a->rt->screen.rows - 1};
  (void)id;
  Panel *ptr = p.get();
  a->panel_order.push_back(std::move(p));
  return make_panel_object(ctx, ptr);
}

JSValue js_panel_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *p = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class)); if (p) p->frame = argc ? js_to_string(ctx, argv[0]) : "single"; return JS_DupValue(ctx, this_val); }
JSValue js_panel_title(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *p = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class)); if (p && argc) p->title = StoredValue(ctx, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue js_panel_title_right(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *p = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class)); if (p && argc) p->title_right = StoredValue(ctx, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue panel_new_widget(JSContext *ctx, JSValueConst this_val, Widget::Type type) { auto *p = static_cast<Panel *>(JS_GetOpaque(this_val, g_panel_class)); if (!p) return JS_ThrowTypeError(ctx, "bad panel"); auto w = std::make_unique<Widget>(); w->panel = p; w->type = type; Widget *ptr = w.get(); p->widgets.push_back(std::move(w)); return make_widget_object(ctx, ptr); }
JSValue js_panel_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { JSValue o = panel_new_widget(ctx, this_val, Widget::TEXT); auto *w = static_cast<Widget *>(JS_GetOpaque(o, g_widget_class)); if (w && argc) w->value = StoredValue(ctx, argv[0]); return o; }
JSValue js_panel_gauge(JSContext *ctx, JSValueConst this_val, int, JSValueConst *) { return panel_new_widget(ctx, this_val, Widget::GAUGE); }

JSValue js_widget_at(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w && argc) { if (JS_IsString(argv[0])) w->x_align = js_to_string(ctx, argv[0]); else JS_ToInt32(ctx, &w->x, argv[0]); if (argc > 1) JS_ToInt32(ctx, &w->y, argv[1]); } return JS_DupValue(ctx, this_val); }
JSValue js_widget_fg(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w && argc) w->style.fg = js_to_string(ctx, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue js_widget_bold(JSContext *ctx, JSValueConst this_val, int, JSValueConst *) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w) w->style.bold = true; return JS_DupValue(ctx, this_val); }
JSValue js_widget_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w && argc) w->label = js_to_string(ctx, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue js_widget_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w && argc) w->value = StoredValue(ctx, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue js_widget_width(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w && argc) JS_ToInt32(ctx, &w->width, argv[0]); return JS_DupValue(ctx, this_val); }
JSValue js_widget_show_pct(JSContext *ctx, JSValueConst this_val, int, JSValueConst *) { auto *w = static_cast<Widget *>(JS_GetOpaque(this_val, g_widget_class)); if (w) w->show_pct = true; return JS_DupValue(ctx, this_val); }
JSValue js_widget_style(JSContext *ctx, JSValueConst this_val, int, JSValueConst *) { return JS_DupValue(ctx, this_val); }

void add_funcs(JSContext *ctx, JSValue obj, const JSCFunctionListEntry *funcs, int count) { JS_SetPropertyFunctionList(ctx, obj, funcs, count); }
JSValue make_app_object(JSContext *ctx, App *app) { JSValue o = JS_NewObjectClass(ctx, g_app_class); JS_SetOpaque(o, app); static const JSCFunctionListEntry funcs[] = { JS_CFUNC_DEF("state", 1, js_app_state), JS_CFUNC_DEF("panel", 1, js_app_panel), JS_CFUNC_DEF("statusbar", 1, js_app_statusbar), JS_CFUNC_DEF("on", 3, js_app_on), JS_CFUNC_DEF("key", 2, js_app_key), JS_CFUNC_DEF("mount", 0, js_app_mount), JS_CFUNC_DEF("exit", 0, js_app_exit) }; add_funcs(ctx, o, funcs, (int)(sizeof(funcs)/sizeof(funcs[0]))); return o; }
JSValue make_panel_object(JSContext *ctx, Panel *panel) { JSValue o = JS_NewObjectClass(ctx, g_panel_class); JS_SetOpaque(o, panel); static const JSCFunctionListEntry funcs[] = { JS_CFUNC_DEF("frame", 1, js_panel_frame), JS_CFUNC_DEF("title", 1, js_panel_title), JS_CFUNC_DEF("titleRight", 1, js_panel_title_right), JS_CFUNC_DEF("text", 1, js_panel_text), JS_CFUNC_DEF("gauge", 0, js_panel_gauge) }; add_funcs(ctx, o, funcs, (int)(sizeof(funcs)/sizeof(funcs[0]))); return o; }
JSValue make_widget_object(JSContext *ctx, Widget *widget) { JSValue o = JS_NewObjectClass(ctx, g_widget_class); JS_SetOpaque(o, widget); static const JSCFunctionListEntry funcs[] = { JS_CFUNC_DEF("at", 2, js_widget_at), JS_CFUNC_DEF("fg", 1, js_widget_fg), JS_CFUNC_DEF("bold", 0, js_widget_bold), JS_CFUNC_DEF("label", 1, js_widget_label), JS_CFUNC_DEF("value", 1, js_widget_value), JS_CFUNC_DEF("width", 1, js_widget_width), JS_CFUNC_DEF("showPct", 0, js_widget_show_pct), JS_CFUNC_DEF("style", 1, js_widget_style) }; add_funcs(ctx, o, funcs, (int)(sizeof(funcs)/sizeof(funcs[0]))); return o; }

void register_classes(JSRuntime *rt) {
  JS_NewClassID(&g_app_class); JS_NewClassID(&g_panel_class); JS_NewClassID(&g_widget_class);
  JSClassDef app_def = {"PicoApp"}; JSClassDef panel_def = {"PicoPanel"}; JSClassDef widget_def = {"PicoWidget"};
  JS_NewClass(rt, g_app_class, &app_def); JS_NewClass(rt, g_panel_class, &panel_def); JS_NewClass(rt, g_widget_class, &widget_def);
}

bool eval_file(JSContext *ctx, const std::string &path, std::string *error) {
  std::ifstream in(path);
  if (!in) { if (error) *error = "open failed: " + path; return false; }
  std::stringstream ss; ss << in.rdbuf(); std::string code = ss.str();
  JSValue v = JS_Eval(ctx, code.c_str(), code.size(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(v)) {
    JSValue exc = JS_GetException(ctx);
    if (error) *error = js_to_string(ctx, exc);
    JS_FreeValue(ctx, exc);
    JS_FreeValue(ctx, v);
    return false;
  }
  JS_FreeValue(ctx, v);
  return true;
}

uint64_t host_millis() { return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - g_start).count(); }

Runtime *runtime_create(int cols, int rows) {
  auto *rt = new Runtime(cols, rows);
  rt->jsrt = JS_NewRuntime();
  register_classes(rt->jsrt);
  rt->ctx = JS_NewContext(rt->jsrt);
  g_current_runtime = rt;
  JSContext *ctx = rt->ctx;
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "print", JS_NewCFunction(ctx, js_print, "print", 1));
  JS_SetPropertyStr(ctx, global, "millis", JS_NewCFunction(ctx, js_millis, "millis", 0));
  JS_SetPropertyStr(ctx, global, "gc", JS_NewCFunction(ctx, js_gc, "gc", 0));

  JSValue os = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, os, "app", JS_NewCFunction(ctx, js_os_app, "app", 1));
  JS_SetPropertyStr(ctx, os, "clock", JS_NewCFunction(ctx, js_os_clock, "clock", 1));
  JS_SetPropertyStr(ctx, os, "launch", JS_NewCFunction(ctx, js_os_launch, "launch", 1));
  JS_SetPropertyStr(ctx, os, "battery", JS_NewFloat64(ctx, rt->battery));
  JS_SetPropertyStr(ctx, os, "toast", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, global, "OS", os);
  JS_FreeValue(ctx, global);
  return rt;
}

void runtime_destroy(Runtime *rt) {
  if (!rt) return;
  if (g_current_runtime == rt) g_current_runtime = nullptr;
  // Destroy native objects first so StoredValue fields release duplicated
  // JSValue handles while the QuickJS context is still alive.
  rt->app.reset();
  if (rt->ctx) {
    JS_FreeContext(rt->ctx);
    rt->ctx = nullptr;
  }
  if (rt->jsrt) {
    JS_FreeRuntime(rt->jsrt);
    rt->jsrt = nullptr;
  }
  delete rt;
}
JSContext *runtime_context(Runtime *rt) { return rt ? rt->ctx : nullptr; }
bool runtime_load_file(Runtime *rt, const std::string &path, std::string *error) { g_current_runtime = rt; return rt && eval_file(rt->ctx, path, error); }
void runtime_run_frame(Runtime *rt, int dt_ms) {
  if (!rt || !rt->app || rt->app->exited) return;
  for (auto &t : rt->app->timers) {
    t.acc += dt_ms;
    if (t.acc >= t.ms) { t.acc = 0; JS_Call(rt->ctx, t.fn.value, JS_UNDEFINED, 0, nullptr); }
  }
  render_app(rt);
}
void runtime_send_key(Runtime *rt, const std::string &token) {
  if (!rt || !rt->app) return;
  auto it = rt->app->keys.find(token);
  if (it == rt->app->keys.end()) return;
  JSValue arg = JS_NewString(rt->ctx, token.c_str());
  JSValue app_obj = make_app_object(rt->ctx, rt->app.get());
  JSValueConst argv[2] = {app_obj, arg};
  JS_Call(rt->ctx, it->second.value, JS_UNDEFINED, 2, argv);
  JS_FreeValue(rt->ctx, app_obj); JS_FreeValue(rt->ctx, arg);
}
std::string runtime_render_text(Runtime *rt) { if (!rt) return {}; render_app(rt); return rt->screen.render(); }

}  // namespace pico_native
