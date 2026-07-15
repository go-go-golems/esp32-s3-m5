#include "app_js.h"

#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "app_js_bindings.h"
#include "app_ui.h"
#include "s3paper/page.h"
#include "s3paper/text.h"
#include "s3paper/widget.h"
#include "s3paper/widget_render.h"

extern "C" const JSSTDLibraryDef js_stdlib;  // main/js_stdlib_table.c

namespace reader {
namespace {

const char *kTag = "js";

constexpr uint32_t kArenaBytes = 160 * 1024;
constexpr uint32_t kMaxJsHits = 16;
constexpr uint32_t kAbiVersion = 1;

JSContext *s_ctx = nullptr;
uint8_t *s_arena = nullptr;
int64_t s_deadline_us = 0;
uint32_t s_evals = 0;
uint32_t s_exceptions = 0;
uint32_t s_dispatches = 0;
char s_last_error[48] = {};

// JS page state: hit regions of the last JS present and the global present
// sequence at that moment (another present -> JS screen no longer active).
s3paper::HitRegion s_js_hits[kMaxJsHits];
uint32_t s_js_hit_count = 0;
uint32_t s_js_present_seq = 0;
bool s_js_presented = false;
s3paper::PageId s_js_page = s3paper::kNoPage;

int InterruptHandler(JSContext *, void *) {
    return s_deadline_us != 0 && esp_timer_get_time() > s_deadline_us;
}

void LogFunc(void *, const void *buf, size_t len) {
    fwrite(buf, 1, len, stdout);
}

// Evaluates under a deadline; prints and records exceptions.
StatusCode EvalBounded(const char *code, uint32_t timeout_ms,
                       const char *name) {
    s_deadline_us = esp_timer_get_time() +
                    static_cast<int64_t>(timeout_ms) * 1000;
    const JSValue v = JS_Eval(s_ctx, code, strlen(code), name, 0);
    s_deadline_us = 0;
    s_evals++;
    if (JS_IsException(v)) {
        s_exceptions++;
        const JSValue err = JS_GetException(s_ctx);
        JSCStringBuf buf;
        const char *msg = JS_ToCString(s_ctx, err, &buf);
        snprintf(s_last_error, sizeof(s_last_error), "%s",
                 msg != nullptr ? msg : "?");
        printf("js error in %s: ", name);
        JS_PrintValueF(s_ctx, err, JS_DUMP_LONG);
        printf("\n");
        return StatusCode::CorruptData;
    }
    return StatusCode::Ok;
}

// ---- handle packing (same generation scheme as s3paper WidgetHandle) ----

int32_t Pack(s3paper::WidgetHandle h) {
    return static_cast<int32_t>(static_cast<uint32_t>(h.generation) << 16 |
                                h.index);
}

s3paper::WidgetHandle Unpack(int32_t packed) {
    return s3paper::WidgetHandle{
        static_cast<uint16_t>(static_cast<uint32_t>(packed) & 0xFFFF),
        static_cast<uint16_t>(static_cast<uint32_t>(packed) >> 16)};
}

// Validated handle argument: throws TypeError on stale/invalid (muo1).
bool ArgHandle(JSContext *ctx, JSValue arg, s3paper::WidgetHandle *out,
               JSValue *err) {
    int packed = 0;
    if (JS_ToInt32(ctx, &packed, arg)) {
        *err = JS_EXCEPTION;
        return false;
    }
    const s3paper::WidgetHandle h = Unpack(packed);
    if (UiArena().Get(h) == nullptr) {
        *err = JS_ThrowTypeError(ctx, "stale widget handle");
        return false;
    }
    *out = h;
    return true;
}

JSValue NewHandleOrThrow(JSContext *ctx,
                         s3paper::Result<s3paper::WidgetHandle> r) {
    if (!r.ok()) {
        return JS_ThrowTypeError(ctx, "widget arena full");
    }
    return JS_NewInt32(ctx, Pack(r.value));
}

// ---- embedded facade and acceptance apps (ES5-stricter dialect) ----

const char kFacadeJs[] =
    "var s3 = (function() {\n"
    "  function W(h) { this.h = h; }\n"
    "  W.prototype.pad = function(t, r, b, l) {\n"
    "    s3Config(this.h, 1, t, r, b, l); return this; };\n"
    "  W.prototype.gap = function(g) {\n"
    "    s3Config(this.h, 2, g, 0, 0, 0); return this; };\n"
    "  W.prototype.mainAlign = function(a) {\n"
    "    s3Config(this.h, 3, a, 0, 0, 0); return this; };\n"
    "  W.prototype.crossAlign = function(a) {\n"
    "    s3Config(this.h, 4, a, 0, 0, 0); return this; };\n"
    "  W.prototype.width = function(v) {\n"
    "    s3Config(this.h, 5, v, 0, 0, 0); return this; };\n"
    "  W.prototype.height = function(v) {\n"
    "    s3Config(this.h, 6, v, 0, 0, 0); return this; };\n"
    "  W.prototype.flex = function(f) {\n"
    "    s3Config(this.h, 7, f, 0, 0, 0); return this; };\n"
    "  W.prototype.font = function(f) {\n"
    "    s3Config(this.h, 8, f, 0, 0, 0); return this; };\n"
    "  W.prototype.gray = function(g) {\n"
    "    s3Config(this.h, 9, g, 0, 0, 0); return this; };\n"
    "  W.prototype.center = function() {\n"
    "    s3Config(this.h, 10, 1, 0, 0, 0); return this; };\n"
    "  W.prototype.dep = function(d) {\n"
    "    s3Config(this.h, 12, d, 0, 0, 0); return this; };\n"
    "  W.prototype.onTap = function(fn) {\n"
    "    var id = api._nextHit++; api._taps[id] = fn;\n"
    "    s3Config(this.h, 11, id, 1, 0, 0); return this; };\n"
    "  W.prototype.add = function() {\n"
    "    for (var i = 0; i < arguments.length; i++)\n"
    "      s3AddChild(this.h, arguments[i].h);\n"
    "    return this; };\n"
    "  W.prototype.set = function(s) { s3SetText(this.h, s); return this; };\n"
    "  W.prototype.progress = function(p) {\n"
    "    s3SetProgress(this.h, p); return this; };\n"
    "  var api = {\n"
    "    _taps: {}, _nextHit: 1,\n"
    "    FONT_UI: 0, FONT_BODY: 1,\n"
    "    text: function(s) { return new W(s3Text(s)); },\n"
    "    row: function() { return new W(s3Row()); },\n"
    "    col: function() { return new W(s3Col()); },\n"
    "    spacer: function(px, fl) { return new W(s3Spacer(px || 0, fl || 0)); },\n"
    "    divider: function(t, g) { return new W(s3Divider(t || 1, g || 0)); },\n"
    "    progressBar: function(p, h) {\n"
    "      return new W(s3Progress(p, h || 12, 0)); },\n"
    "    list: function() { return new W(s3List()); },\n"
    "    reset: function() { this._taps = {}; this._nextHit = 1;\n"
    "      s3Reset(); return this; },\n"
    "    render: function(sl) {\n"
    "      return s3Present(sl.header ? sl.header.h : 0,\n"
    "                       sl.content ? sl.content.h : 0,\n"
    "                       sl.footer ? sl.footer.h : 0,\n"
    "                       sl.overlay ? sl.overlay.h : 0,\n"
    "                       sl.full ? 1 : 0); }\n"
    "  };\n"
    "  return api;\n"
    "})();\n"
    "function s3Dispatch(kind, x, y, hit) {\n"
    "  if (hit && s3._taps[hit]) { s3._taps[hit](kind, x, y); }\n"
    "}\n";

const char kHelloJs[] =
    "s3.reset();\n"
    "var header = s3.col().pad(16, 40, 4, 40).gap(8)\n"
    "  .add(s3.text('JS app: hello').font(s3.FONT_UI), s3.divider(2, 0));\n"
    "var content = s3.col().pad(24, 40, 24, 40).gap(16).add(\n"
    "  s3.text('Hello from MicroQuickJS.'),\n"
    "  s3.text('This tree was built by ES5,'),\n"
    "  s3.text('rendered by s3paper_core.').center().gray(96),\n"
    "  s3.progressBar(420, 24).height(24));\n"
    "var footer = s3.col().pad(6, 40, 10, 40).gap(6)\n"
    "  .add(s3.divider(1, 0),\n"
    "       s3.text('fluent facade -> native widgets -> EPD')\n"
    "         .font(s3.FONT_UI).gray(96));\n"
    "s3.render({header: header, content: content, footer: footer,\n"
    "           full: true});\n";

const char kStatusJs[] =
    "s3.reset();\n"
    "var taps = 0;\n"
    "var header = s3.col().pad(16, 40, 4, 40).gap(8)\n"
    "  .add(s3.text('JS app: status').font(s3.FONT_UI), s3.divider(2, 0));\n"
    "var counter = s3.text('taps: 0');\n"
    "var uptime = s3.text('uptime: ' + Math.floor(millis() / 1000) + ' s');\n"
    "var tapRow = s3.row().pad(12, 0, 12, 0).gap(12).add(counter)\n"
    "  .onTap(function(kind, x, y) {\n"
    "    taps++;\n"
    "    counter.set('taps: ' + taps);\n"
    "    uptime.set('uptime: ' + Math.floor(millis() / 1000) + ' s');\n"
    "    s3.render({header: header, content: content, footer: footer});\n"
    "  });\n"
    "var content = s3.col().pad(24, 40, 24, 40).gap(14)\n"
    "  .add(uptime, tapRow, s3.divider(1, 176));\n"
    "var footer = s3.col().pad(6, 40, 10, 40).gap(6)\n"
    "  .add(s3.divider(1, 0),\n"
    "       s3.text('tap the counter line').font(s3.FONT_UI).gray(96));\n"
    "s3.render({header: header, content: content, footer: footer,\n"
    "           full: true});\n";

}  // namespace

StatusCode JsInit() {
    if (s_ctx != nullptr) {
        return StatusCode::Ok;
    }
    // PSRAM first (the Phase 11 spike measured identical eval speed there);
    // internal RAM is the reader's scarce resource.
    s_arena = static_cast<uint8_t *>(
        heap_caps_malloc(kArenaBytes, MALLOC_CAP_SPIRAM));
    if (s_arena == nullptr) {
        s_arena = static_cast<uint8_t *>(
            heap_caps_malloc(kArenaBytes, MALLOC_CAP_INTERNAL));
    }
    if (s_arena == nullptr) {
        return StatusCode::OutOfMemory;
    }
    s_ctx = JS_NewContext(s_arena, kArenaBytes, &js_stdlib);
    if (s_ctx == nullptr) {
        free(s_arena);
        s_arena = nullptr;
        return StatusCode::OutOfMemory;
    }
    JS_SetLogFunc(s_ctx, LogFunc);
    JS_SetInterruptHandler(s_ctx, InterruptHandler);
    const StatusCode facade = EvalBounded(kFacadeJs, 1000, "<facade>");
    if (facade != StatusCode::Ok) {
        ESP_LOGE(kTag, "facade failed to load");
        return facade;
    }
    ESP_LOGI(kTag, "context ready: arena=%u bytes, abi=v%u",
             static_cast<unsigned>(kArenaBytes),
             static_cast<unsigned>(kAbiVersion));
    return StatusCode::Ok;
}

StatusCode JsRunApp(uint32_t which) {
    const StatusCode init = JsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    const char *src = which == 2 ? kStatusJs : kHelloJs;
    const char *name = which == 2 ? "<status>" : "<hello>";
    return EvalBounded(src, 3000, name);
}

bool JsScreenActive() {
    return s_js_presented && UiPresentCount() == s_js_present_seq;
}

bool JsHandleGesture(const s3paper::GestureEvent &gesture) {
    if (s_ctx == nullptr || !JsScreenActive()) {
        return false;
    }
    uint32_t hit = 0;
    const s3paper::Result<uint32_t> tested =
        s3paper::HitTest(s_js_hits, s_js_hit_count, gesture.pos);
    if (tested.ok()) {
        hit = tested.value;
    }
    char call[80];
    snprintf(call, sizeof(call), "s3Dispatch(%d,%d,%d,%u)",
             static_cast<int>(gesture.kind), static_cast<int>(gesture.pos.x),
             static_cast<int>(gesture.pos.y), static_cast<unsigned>(hit));
    s_dispatches++;
    (void)EvalBounded(call, 1000, "<dispatch>");
    return true;
}

void FillJsSnapshot(JsSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->initialized = s_ctx != nullptr ? 1 : 0;
    out->screen_active = JsScreenActive() ? 1 : 0;
    out->arena_bytes = s_ctx != nullptr ? kArenaBytes : 0;
    out->evals = s_evals;
    out->exceptions = s_exceptions;
    out->dispatches = s_dispatches;
    snprintf(out->last_error, sizeof(out->last_error), "%s", s_last_error);
}

// ---- C ABI implementations referenced by the generated stdlib table ----
// Defined inside namespace reader (extern "C" linkage ignores namespaces)
// so they can reach the anonymous-namespace host state above.

extern "C" {

JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    for (int i = 0; i < argc; i++) {
        if (i != 0) {
            putchar(' ');
        }
        if (JS_IsString(ctx, argv[i])) {
            JSCStringBuf buf;
            size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, argv[i], &buf);
            fwrite(str, 1, len, stdout);
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    putchar('\n');
    return JS_UNDEFINED;
}

JSValue js_gc(JSContext *ctx, JSValue *, int, JSValue *) {
    JS_GC(ctx);
    return JS_UNDEFINED;
}

JSValue js_load(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_ThrowTypeError(ctx, "load() not supported");
}

JSValue js_setTimeout(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_ThrowTypeError(ctx, "setTimeout() not supported");
}

JSValue js_clearTimeout(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_ThrowTypeError(ctx, "clearTimeout() not supported");
}

JSValue js_date_now(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt64(ctx, esp_timer_get_time() / 1000);
}

JSValue js_performance_now(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt64(ctx, esp_timer_get_time() / 1000);
}

JSValue js_millis(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt64(ctx, esp_timer_get_time() / 1000);
}

JSValue js_s3_version(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, kAbiVersion);
}

JSValue js_s3_reset(JSContext *ctx, JSValue *, int, JSValue *) {
    UiArena().Reset();
    return JS_UNDEFINED;
}

JSValue js_s3_text(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "s3Text(str)");
    }
    // Copy out before any further JS API use (GC may move the string).
    char text[s3paper::TextProps::kCapacity];
    {
        JSCStringBuf buf;
        size_t len;
        const char *str = JS_ToCStringLen(ctx, &len, argv[0], &buf);
        if (str == nullptr) {
            return JS_EXCEPTION;
        }
        snprintf(text, sizeof(text), "%.*s", static_cast<int>(len), str);
    }
    return NewHandleOrThrow(
        ctx, s3paper::NewText(UiArena(), text, s3paper::kFontBody, 0));
}

JSValue js_s3_row(JSContext *ctx, JSValue *, int, JSValue *) {
    return NewHandleOrThrow(ctx, s3paper::NewRow(UiArena()));
}

JSValue js_s3_col(JSContext *ctx, JSValue *, int, JSValue *) {
    return NewHandleOrThrow(ctx, s3paper::NewCol(UiArena()));
}

JSValue js_s3_spacer(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int fixed = 0;
    int flex = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &fixed, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &flex, argv[1])) return JS_EXCEPTION;
    return NewHandleOrThrow(
        ctx, s3paper::NewSpacer(UiArena(), fixed,
                                static_cast<uint16_t>(flex < 0 ? 0 : flex)));
}

JSValue js_s3_divider(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int thickness = 1;
    int gray = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &thickness, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &gray, argv[1])) return JS_EXCEPTION;
    return NewHandleOrThrow(
        ctx, s3paper::NewDivider(UiArena(), thickness,
                                 static_cast<s3paper::Gray8>(gray)));
}

JSValue js_s3_progress(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int permille = 0;
    int height = 12;
    int gray = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &permille, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &height, argv[1])) return JS_EXCEPTION;
    if (argc >= 3 && JS_ToInt32(ctx, &gray, argv[2])) return JS_EXCEPTION;
    return NewHandleOrThrow(
        ctx, s3paper::NewProgress(UiArena(),
                                  static_cast<uint16_t>(
                                      permille < 0 ? 0 : permille),
                                  height, static_cast<s3paper::Gray8>(gray)));
}

JSValue js_s3_list(JSContext *ctx, JSValue *, int, JSValue *) {
    return NewHandleOrThrow(ctx, s3paper::NewList(UiArena()));
}

JSValue js_s3_region(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int id = 0;
    int interval = 0;
    int quiet = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &id, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &interval, argv[1])) return JS_EXCEPTION;
    if (argc >= 3 && JS_ToInt32(ctx, &quiet, argv[2])) return JS_EXCEPTION;
    return NewHandleOrThrow(
        ctx, s3paper::NewRegion(UiArena(), static_cast<uint32_t>(id),
                                static_cast<uint32_t>(interval), quiet != 0));
}

JSValue js_s3_add_child(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "s3AddChild(parent, child)");
    }
    s3paper::WidgetHandle parent;
    s3paper::WidgetHandle child;
    JSValue err;
    if (!ArgHandle(ctx, argv[0], &parent, &err)) return err;
    if (!ArgHandle(ctx, argv[1], &child, &err)) return err;
    const s3paper::Status st = UiArena().AddChild(parent, child);
    if (!st.ok()) {
        return JS_ThrowTypeError(ctx, "AddChild failed: %s",
                                 s3paper::StatusCodeName(st.code));
    }
    return JS_UNDEFINED;
}

JSValue js_s3_set_text(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "s3SetText(handle, str)");
    }
    s3paper::WidgetHandle h;
    JSValue err;
    if (!ArgHandle(ctx, argv[0], &h, &err)) return err;
    char text[s3paper::TextProps::kCapacity];
    {
        JSCStringBuf buf;
        size_t len;
        const char *str = JS_ToCStringLen(ctx, &len, argv[1], &buf);
        if (str == nullptr) {
            return JS_EXCEPTION;
        }
        snprintf(text, sizeof(text), "%.*s", static_cast<int>(len), str);
    }
    const s3paper::Status st = UiArena().SetText(h, text);
    if (!st.ok()) {
        return JS_ThrowTypeError(ctx, "SetText failed: %s",
                                 s3paper::StatusCodeName(st.code));
    }
    return JS_UNDEFINED;
}

JSValue js_s3_set_progress(JSContext *ctx, JSValue *, int argc,
                           JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "s3SetProgress(handle, permille)");
    }
    s3paper::WidgetHandle h;
    JSValue err;
    if (!ArgHandle(ctx, argv[0], &h, &err)) return err;
    int permille = 0;
    if (JS_ToInt32(ctx, &permille, argv[1])) return JS_EXCEPTION;
    const s3paper::Status st = UiArena().SetProgress(
        h, static_cast<uint16_t>(permille < 0 ? 0 : permille));
    if (!st.ok()) {
        return JS_ThrowTypeError(ctx, "SetProgress failed: %s",
                                 s3paper::StatusCodeName(st.code));
    }
    return JS_UNDEFINED;
}

// s3Config(handle, prop, a, b, c, d): versioned property ABI (task qoou).
//  1 padding(t,r,b,l)  2 gap(a)      3 main_align(a)  4 cross_align(a)
//  5 fixed_w(a)        6 fixed_h(a)  7 flex(a)        8 font(a, Text only)
//  9 gray(a)          10 text_align(a, Text only)
// 11 hit(a=id, b=z)   12 dependency(a)
JSValue js_s3_config(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "s3Config(handle, prop, ...)");
    }
    s3paper::WidgetHandle h;
    JSValue err;
    if (!ArgHandle(ctx, argv[0], &h, &err)) return err;
    int prop = 0;
    if (JS_ToInt32(ctx, &prop, argv[1])) return JS_EXCEPTION;
    int a = 0, b = 0, c = 0, d = 0;
    if (argc >= 3 && JS_ToInt32(ctx, &a, argv[2])) return JS_EXCEPTION;
    if (argc >= 4 && JS_ToInt32(ctx, &b, argv[3])) return JS_EXCEPTION;
    if (argc >= 5 && JS_ToInt32(ctx, &c, argv[4])) return JS_EXCEPTION;
    if (argc >= 6 && JS_ToInt32(ctx, &d, argv[5])) return JS_EXCEPTION;

    s3paper::WidgetNode *n = UiArena().Configure(h);
    if (n == nullptr) {
        return JS_ThrowTypeError(ctx, "stale widget handle");
    }
    const bool is_text = n->kind == s3paper::WidgetKind::Text;
    switch (prop) {
        case 1: n->padding = s3paper::Insets{a, b, c, d}; break;
        case 2: n->gap = a; break;
        case 3:
            if (a < 0 || a > 3) return JS_ThrowTypeError(ctx, "bad align");
            n->main_align = static_cast<s3paper::MainAlign>(a);
            break;
        case 4:
            if (a < 0 || a > 3) return JS_ThrowTypeError(ctx, "bad align");
            n->cross_align = static_cast<s3paper::CrossAlign>(a);
            break;
        case 5: n->fixed_w = a; break;
        case 6: n->fixed_h = a; break;
        case 7: n->flex = static_cast<uint16_t>(a < 0 ? 0 : a); break;
        case 8:
            if (!is_text) return JS_ThrowTypeError(ctx, "font: not a Text");
            if (a < 0 || a >= s3paper::kFontCount) {
                return JS_ThrowTypeError(ctx, "bad font id");
            }
            n->props.text.font_id = static_cast<uint8_t>(a);
            break;
        case 9:
            if (!is_text) return JS_ThrowTypeError(ctx, "gray: not a Text");
            n->props.text.gray = static_cast<s3paper::Gray8>(a);
            break;
        case 10:
            if (!is_text) return JS_ThrowTypeError(ctx, "align: not a Text");
            if (a < 0 || a > 2) return JS_ThrowTypeError(ctx, "bad align");
            n->props.text.align = static_cast<s3paper::TextAlign>(a);
            break;
        case 11:
            n->hit_id = static_cast<uint32_t>(a);
            n->hit_z = static_cast<int16_t>(b);
            break;
        case 12: n->dependency = static_cast<uint32_t>(a); break;
        default:
            return JS_ThrowTypeError(ctx, "unknown prop %d (abi v%d)", prop,
                                     static_cast<int>(kAbiVersion));
    }
    return JS_UNDEFINED;
}

JSValue js_s3_present(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 5) {
        return JS_ThrowTypeError(ctx, "s3Present(h, c, f, o, full)");
    }
    s3paper::PageSlots slots{};
    s3paper::WidgetHandle *fields[4] = {&slots.header, &slots.content,
                                        &slots.footer, &slots.overlay};
    for (int i = 0; i < 4; i++) {
        int packed = 0;
        if (JS_ToInt32(ctx, &packed, argv[i])) return JS_EXCEPTION;
        if (packed == 0) {
            *fields[i] = s3paper::kNullWidget;
            continue;
        }
        JSValue err;
        if (!ArgHandle(ctx, argv[i], fields[i], &err)) return err;
    }
    int full = 0;
    if (JS_ToInt32(ctx, &full, argv[4])) return JS_EXCEPTION;

    if (s_js_page == s3paper::kNoPage) {
        const s3paper::Result<s3paper::PageId> registered =
            UiRouter().Register("js", slots);
        if (registered.ok()) {
            s_js_page = registered.value;
        }
    }
    if (s_js_page != s3paper::kNoPage) {
        (void)UiRouter().SetSlots(s_js_page, slots);
        (void)UiRouter().Push(s_js_page);
    }
    const UiPresentResult presented = UiPresentPage(
        slots,
        full != 0 ? s3paper::PresentIntent::CleanFull
                  : s3paper::PresentIntent::TextPage,
        full != 0, s_js_hits, kMaxJsHits, nullptr);
    if (presented.status == StatusCode::Ok) {
        s_js_hit_count = presented.hit_count;
        s_js_present_seq = UiPresentCount();
        s_js_presented = true;
        ESP_LOGI(kTag, "js present: hits=%u full=%d",
                 static_cast<unsigned>(s_js_hit_count),
                 presented.full_refresh ? 1 : 0);
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(presented.status));
}

}  // extern "C"

}  // namespace reader
