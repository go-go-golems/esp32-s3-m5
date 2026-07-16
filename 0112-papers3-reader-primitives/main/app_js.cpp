#include "app_js.h"

#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "app_display.h"
#include "app_js_bindings.h"
#include "app_reader.h"
#include "app_reader_book.h"
#include "app_storage.h"
#include "app_ui.h"
#include "s3paper/content.h"
#include "s3paper/page.h"
#include "s3paper/paginator.h"
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

// Headless book service for the JS reader port (task wipy): the paginator
// and content sources are native primitives; JS owns chrome and gestures.
// Uses the SAME LayoutKey as the native reader, so content hashes and
// persisted positions interoperate (a book left in the JS reader resumes
// at the same page in the native one and vice versa).
struct JsBook {
    bool open = false;
    s3paper::MemoryContentSource embedded{kEmbeddedBookText,
                                          sizeof(kEmbeddedBookText) - 1};
    SdContentSource sd;
    s3paper::ContentSource *active = nullptr;
    s3paper::Paginator *paginator = nullptr;
    s3paper::ContentHash hash = 0;
    s3paper::TextLocator current{};
    s3paper::PageLayout page{};
    char title[40] = {};
};

JsBook s_book;

s3paper::LayoutKey JsBookKey(s3paper::ContentHash content) {
    s3paper::LayoutKey key{};
    key.content = content;
    key.font_id = s3paper::kFontBody;
    key.viewport_w = 540;
    key.viewport_h = 960;
    key.margin_x = 40;
    key.margin_top = 72;
    key.margin_bottom = 56;
    key.engine_version = s3paper::kLayoutEngineVersion;
    return key;
}

// Composes the page at `at` (by value: `at` may alias page.next).
StatusCode JsBookCompose(s3paper::TextLocator at) {
    const s3paper::Status composed =
        s_book.paginator->ComposePage(at, &s_book.page);
    if (!composed.ok()) {
        return composed.code;
    }
    s_book.current = at;
    if (StorageMounted()) {
        PositionStore(s_book.hash, s_book.current);
    }
    return StatusCode::Ok;
}

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
    "    _onGesture: null,\n"
    "    reset: function() { this._taps = {}; this._nextHit = 1;\n"
    "      this._onGesture = null; s3Reset(); return this; },\n"
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
    "  if (s3._onGesture && s3._onGesture(kind, x, y, hit)) { return; }\n"
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

// Byte-exact mirror of app_ui's BuildHelloFixture: the trace-equivalence
// harness (task 17nn) presents both through the fake backend and compares
// the normalized draw-op traces.
const char kHelloTraceJs[] =
    "s3.reset();\n"
    "var header = s3.col().pad(16, 40, 4, 40).gap(8)\n"
    "  .add(s3.text('Widget fixture: hello').font(s3.FONT_UI),\n"
    "       s3.divider(2, 0));\n"
    "var content = s3.col().pad(24, 40, 24, 40).gap(16).add(\n"
    "  s3.text('Hello, PaperS3.'),\n"
    "  s3.text('Rows, columns, dividers,'),\n"
    "  s3.row().gap(12).add(s3.text('progress:'),\n"
    "                       s3.progressBar(640, 24).height(24)),\n"
    "  s3.text('centered text').center().gray(96));\n"
    "var footer = s3.col().pad(6, 40, 10, 40).gap(6)\n"
    "  .add(s3.divider(1, 0),\n"
    "       s3.text('generic tree -> draw ops -> EPD')\n"
    "         .font(s3.FONT_UI).gray(96));\n"
    "s3.render({header: header, content: content, footer: footer,\n"
    "           full: true});\n";

// Library port (task wipy): real catalog data through the ABI, tap a row
// to open the book in the NATIVE reader (JS routes, native reads).
const char kLibraryJs[] =
    "s3.reset();\n"
    "var header = s3.col().pad(16, 40, 4, 40).gap(8)\n"
    "  .add(s3.text('JS Library').font(s3.FONT_UI), s3.divider(1, 0));\n"
    "var listW = s3.list().pad(12, 0, 0, 0);\n"
    "function addRow(line, idx) {\n"
    "  var r = s3.col().pad(10, 40, 8, 40).gap(8)\n"
    "    .add(s3.text(line), s3.divider(1, 176))\n"
    "    .onTap(function() { s3OpenBook(idx); });\n"
    "  listW.add(r);\n"
    "}\n"
    "addRow(s3EmbeddedLine(), -1);\n"
    "var n = s3LibraryCount();\n"
    "var i;\n"
    "for (i = 0; i < n; i++) { addRow(s3LibraryLine(i), i); }\n"
    "var footer = s3.col().pad(4, 40, 10, 40)\n"
    "  .add(s3.text('JS library - tap a book to read')\n"
    "         .font(s3.FONT_UI).gray(96));\n"
    "s3.render({header: header, content: listW, footer: footer,\n"
    "           full: true});\n";

// Fault app (task rs5w): presents, proves stale-handle and invalid-prop
// containment, then fails; the owner falls back to the native library.
const char kFaultJs[] =
    "s3.reset();\n"
    "var body = s3.col().pad(24, 40, 24, 40).gap(12)\n"
    "  .add(s3.text('fault app: about to fail'));\n"
    "s3.render({content: body, full: true});\n"
    "var w = s3.text('victim');\n"
    "s3Reset();\n"
    "var stale = 'MISSED';\n"
    "try { w.set('x'); } catch (e) { stale = e.message; }\n"
    "print('fault: stale handle -> ' + stale);\n"
    "var badProp = 'MISSED';\n"
    "var t2 = s3.text('probe');\n"
    "try { s3Config(t2.h, 99, 0, 0, 0, 0); } catch (e2) {\n"
    "  badProp = e2.message; }\n"
    "print('fault: invalid prop -> ' + badProp);\n"
    "throw new Error('deliberate failure after present');\n";

// Reader port (task wipy): native pagination through the book ABI, JS
// chrome, JS gesture policy (kind 0=Tap 2=SwipeLeft 3=SwipeRight), partial
// page turns, full-refresh policy left to the native planner.
const char kReaderJs[] =
    "if (s3BookOpen(-1) !== 0) { throw new Error('book open failed'); }\n"
    "var jsTurns = 0;\n"
    "function renderReader(fullRefresh) {\n"
    "  s3.reset();\n"
    "  var header = s3.col().pad(16, 40, 4, 40).gap(8)\n"
    "    .add(s3.text(s3BookTitle() + '  [js reader]').font(s3.FONT_UI),\n"
    "         s3.divider(1, 0));\n"
    "  var body = s3.col().pad(4, 40, 0, 40);\n"
    "  var n = s3BookLineCount();\n"
    "  var i;\n"
    "  for (i = 0; i < n; i++) { body.add(s3.text(s3BookLine(i))); }\n"
    "  var footer = s3.col().pad(4, 40, 10, 40).gap(6)\n"
    "    .add(s3.divider(1, 0),\n"
    "         s3.text(Math.floor(s3BookProgress() / 10) + '%  turns ' +\n"
    "                 jsTurns + '  -  js reader')\n"
    "           .font(s3.FONT_UI).gray(96));\n"
    "  s3._onGesture = function(kind, x, y, hit) {\n"
    "    var moved = -1;\n"
    "    if (kind === 2) { moved = s3BookNext(); }\n"
    "    else if (kind === 3) { moved = s3BookPrev(); }\n"
    "    else if (kind === 0) {\n"
    "      moved = (x >= 270) ? s3BookNext() : s3BookPrev();\n"
    "    } else { return false; }\n"
    "    if (moved === 0) { jsTurns++; renderReader(false); }\n"
    "    return true;\n"
    "  };\n"
    "  s3.render({header: header, content: body, footer: footer,\n"
    "             full: fullRefresh});\n"
    "}\n"
    "renderReader(true);\n";

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

namespace {

// Copies a fake-backend trace, replacing volatile "id=NNN" frame ids with
// "id=#" so two presents of the same content compare equal.
void NormalizeTraceInto(const char *src, char *dst, uint32_t cap) {
    uint32_t w = 0;
    for (const char *p = src; *p != '\0' && w + 5 < cap;) {
        if (strncmp(p, "id=", 3) == 0) {
            dst[w++] = 'i';
            dst[w++] = 'd';
            dst[w++] = '=';
            dst[w++] = '#';
            p += 3;
            while (*p >= '0' && *p <= '9') {
                p++;
            }
            continue;
        }
        dst[w++] = *p++;
    }
    dst[w] = '\0';
}

}  // namespace

// Exit-gate harness (task 17nn): the native hello fixture and its JS mirror
// must produce identical normalized draw-op traces and effective intents.
StatusCode JsTraceCompare() {
    const StatusCode init = JsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    constexpr uint32_t kTraceCap = 12 * 1024;
    static char *s_native_trace = nullptr;
    static char *s_js_trace = nullptr;
    if (s_native_trace == nullptr) {
        s_native_trace = static_cast<char *>(
            heap_caps_malloc(kTraceCap, MALLOC_CAP_SPIRAM));
        s_js_trace = static_cast<char *>(
            heap_caps_malloc(kTraceCap, MALLOC_CAP_SPIRAM));
        if (s_native_trace == nullptr || s_js_trace == nullptr) {
            return StatusCode::OutOfMemory;
        }
    }

    s3paper::PageSlots slots{};
    const StatusCode built = UiBuildFixtureSlots(1, &slots);
    if (built != StatusCode::Ok) {
        return built;
    }
    UiSetTracePresent(true);
    const UiPresentResult native = UiPresentPage(
        slots, s3paper::PresentIntent::CleanFull, true, nullptr, 0, nullptr);
    if (native.status != StatusCode::Ok) {
        UiSetTracePresent(false);
        return native.status;
    }
    NormalizeTraceInto(FakeTrace(), s_native_trace, kTraceCap);

    const StatusCode js = EvalBounded(kHelloTraceJs, 3000, "<hello-trace>");
    UiSetTracePresent(false);
    if (js != StatusCode::Ok) {
        return js;
    }
    NormalizeTraceInto(FakeTrace(), s_js_trace, kTraceCap);

    const bool equal = strcmp(s_native_trace, s_js_trace) == 0;
    printf("js trace-compare: %s (native=%u bytes, js=%u bytes)\n",
           equal ? "EQUAL" : "DIFFER",
           static_cast<unsigned>(strlen(s_native_trace)),
           static_cast<unsigned>(strlen(s_js_trace)));
    if (!equal) {
        printf("--- native ---\n%s--- js ---\n%s---\n", s_native_trace,
               s_js_trace);
    }
    return equal ? StatusCode::Ok : StatusCode::CorruptData;
}

StatusCode JsRunApp(uint32_t which) {
    const StatusCode init = JsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    switch (which) {
        case 1: return EvalBounded(kHelloJs, 3000, "<hello>");
        case 2: return EvalBounded(kStatusJs, 3000, "<status>");
        case 3: return EvalBounded(kLibraryJs, 3000, "<library>");
        case 4: return EvalBounded(kFaultJs, 3000, "<fault>");
        case 5: return JsTraceCompare();
        case 6: return EvalBounded(kReaderJs, 5000, "<reader>");
        default: return StatusCode::InvalidArgument;
    }
}

StatusCode JsSyntheticGesture(uint32_t kind, int32_t x, int32_t y) {
    if (!JsScreenActive()) {
        return StatusCode::Busy;
    }
    s3paper::GestureEvent gesture{};
    gesture.kind = static_cast<s3paper::GestureKind>(kind);
    gesture.pos = s3paper::Point{x, y};
    gesture.t_us = esp_timer_get_time();
    return JsHandleGesture(gesture) ? StatusCode::Ok
                                    : StatusCode::InvalidArgument;
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

JSValue js_s3_library_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(LibraryCount()));
}

JSValue js_s3_library_line(JSContext *ctx, JSValue *, int argc,
                           JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    if (index < 0 || static_cast<uint32_t>(index) >= LibraryCount()) {
        return JS_ThrowTypeError(ctx, "book index out of range");
    }
    char line[96];
    ReaderFormatLibraryLine(static_cast<uint32_t>(index), line,
                            sizeof(line));
    return JS_NewString(ctx, line);
}

JSValue js_s3_embedded_line(JSContext *ctx, JSValue *, int, JSValue *) {
    char line[96];
    ReaderFormatLibraryLine(0xFFFFFFFF, line, sizeof(line));
    return JS_NewString(ctx, line);
}

JSValue js_s3_open_book(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = -1;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const StatusCode opened =
        index < 0 ? ReaderOpen()
                  : ReaderOpenSd(static_cast<uint32_t>(index));
    return JS_NewInt32(ctx, static_cast<int32_t>(opened));
}

JSValue js_s3_book_open(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = -1;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    delete s_book.paginator;
    s_book.paginator = nullptr;
    s_book.open = false;
    s_book.sd.Close();
    if (index < 0) {
        s_book.active = &s_book.embedded;
        snprintf(s_book.title, sizeof(s_book.title), "%s",
                 kEmbeddedBookTitle);
    } else {
        const BookEntry *book = LibraryGet(static_cast<uint32_t>(index));
        if (book == nullptr) {
            return JS_NewInt32(
                ctx, static_cast<int32_t>(StatusCode::InvalidArgument));
        }
        if (s_book.sd.Open(book->path) != StatusCode::Ok) {
            return JS_NewInt32(ctx,
                               static_cast<int32_t>(StatusCode::Busy));
        }
        s_book.active = &s_book.sd;
        snprintf(s_book.title, sizeof(s_book.title), "%s", book->title);
    }
    const s3paper::Result<s3paper::ContentHash> hash = s_book.active->Hash();
    if (!hash.ok()) {
        return JS_NewInt32(ctx, static_cast<int32_t>(hash.code));
    }
    s_book.hash = hash.value;
    s_book.paginator =
        new s3paper::Paginator(s_book.active, JsBookKey(hash.value));
    s3paper::TextLocator start{};
    s3paper::TextLocator persisted{};
    if (StorageMounted() && PositionLookup(s_book.hash, &persisted) &&
        s_book.paginator->Validate(persisted).ok()) {
        start = persisted;  // resumes exactly where the native reader was
    } else {
        const s3paper::Result<s3paper::TextLocator> begin =
            s_book.paginator->Begin();
        if (!begin.ok()) {
            return JS_NewInt32(ctx, static_cast<int32_t>(begin.code));
        }
        start = begin.value;
    }
    const StatusCode composed = JsBookCompose(start);
    if (composed == StatusCode::Ok) {
        s_book.open = true;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(composed));
}

JSValue js_s3_book_title(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, s_book.open ? s_book.title : "");
}

JSValue js_s3_book_line_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(
        ctx, s_book.open ? static_cast<int32_t>(s_book.page.line_count) : 0);
}

JSValue js_s3_book_line(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    if (!s_book.open || index < 0 ||
        static_cast<uint32_t>(index) >= s_book.page.line_count) {
        return JS_ThrowTypeError(ctx, "line index out of range");
    }
    const s3paper::PageLine &line = s_book.page.lines[index];
    // Text widget capacity bounds a line; truncate at a UTF-8 boundary.
    char buf[s3paper::TextProps::kCapacity];
    uint32_t len = line.byte_len < sizeof(buf) - 1
                       ? line.byte_len
                       : static_cast<uint32_t>(sizeof(buf) - 1);
    const s3paper::Result<uint32_t> got = s_book.active->ReadAt(
        line.byte_start, reinterpret_cast<uint8_t *>(buf), len);
    if (!got.ok()) {
        return JS_ThrowTypeError(ctx, "line read failed");
    }
    len = got.value;
    while (len > 0 && (static_cast<uint8_t>(buf[len - 1]) & 0xC0) == 0x80) {
        len--;  // do not split a UTF-8 sequence at the truncation point
    }
    return JS_NewStringLen(ctx, buf, len);
}

JSValue js_s3_book_next(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s_book.open) {
        return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Busy));
    }
    if (s_book.page.at_end) {
        return JS_NewInt32(
            ctx, static_cast<int32_t>(StatusCode::InvalidArgument));
    }
    return JS_NewInt32(
        ctx, static_cast<int32_t>(JsBookCompose(s_book.page.next)));
}

JSValue js_s3_book_prev(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s_book.open) {
        return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Busy));
    }
    const s3paper::Result<s3paper::TextLocator> prev =
        s_book.paginator->PreviousPageStart(s_book.current);
    if (!prev.ok()) {
        return JS_NewInt32(ctx, static_cast<int32_t>(prev.code));
    }
    if (prev.value.byte_offset == s_book.current.byte_offset) {
        return JS_NewInt32(
            ctx, static_cast<int32_t>(StatusCode::InvalidArgument));
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(JsBookCompose(prev.value)));
}

JSValue js_s3_book_progress(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s_book.open) {
        return JS_NewInt32(ctx, 0);
    }
    const s3paper::Result<uint32_t> progress =
        s_book.paginator->ProgressPermille(s_book.page.next);
    return JS_NewInt32(ctx,
                       progress.ok() ? static_cast<int32_t>(progress.value)
                                     : 0);
}

}  // extern "C"

}  // namespace reader
