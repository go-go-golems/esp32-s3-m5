// Internal contract between the JS host core (app_js.cpp) and the binding
// translation units (js_widgets.cpp, js_pages.cpp, js_services.cpp,
// js_probes.cpp, js_browser.cpp). NOT for use outside main/js_*.cpp —
// firmware code uses app_js.h. Everything here is owner-task-only.
//
// ESP-55 P8: the host is multi-context. Every piece of per-engine state
// (pages, hits, dyn values, callback registry, home/sleep callbacks,
// timer) lives in a JsCtxState. The OS context (g_os) holds the ROM
// image and all built-in apps; additional contexts (browser pages) are
// created and destroyed at run time. Exactly one context owns the panel
// at a time (g_fg). JSValues never cross contexts.
#pragma once

#include "app_events.h"
#include "app_js_bindings.h"
#include "s3paper/page.h"
#include "s3paper/widget.h"
#include "s3paper/widget_render.h"

namespace pulp {
namespace jsi {

constexpr uint32_t kAbiVersion = 2;
constexpr uint32_t kMaxJsHits = 48;
constexpr uint32_t kMaxPages = 12;
constexpr uint32_t kMaxDynValues = 48;
constexpr uint32_t kMaxContexts = 3;  // os + one page + one spare/probe

// Native page table entry. Opaque = (generation<<16 | index) + 1.
struct PageEntry {
    bool in_use = false;
    uint16_t generation = 0;
    char name[16] = {};
    s3paper::PageSlots slots{};
    int32_t gesture_cb[6] = {};  // 0 = none, indexed by GestureKind
    int32_t tick_cb = 0;         // on(100, fn)
    uint32_t every_ms = 0;
};

// Dynamic text values: text(fn) widgets re-evaluated on the page tick.
struct DynEntry {
    s3paper::WidgetHandle widget;
    int32_t cb_id;
};

// Kind is advisory (snapshots, teardown policy); the mechanics are
// identical for every context.
enum class CtxKind : uint8_t { kOs = 0, kPage, kProbe };

// One engine context and everything the bindings key off it. The struct
// pointer is stored in the engine's context opaque slot, so StateOf() is
// a single indirection.
struct JsCtxState {
    JSContext *ctx = nullptr;
    uint8_t *arena = nullptr;
    uint32_t arena_bytes = 0;
    CtxKind kind = CtxKind::kOs;
    PageEntry pages[kMaxPages];
    int32_t current_page = -1;
    DynEntry dyn[kMaxDynValues];
    uint32_t dyn_count = 0;
    s3paper::HitRegion hits[kMaxJsHits];
    uint32_t hit_count = 0;
    int32_t next_cb = 1;
    int32_t home_cb = 0;
    int32_t sleep_image_cb = 0;
    int64_t timer_due_us = 0;
};

// Module completions are owner-tagged: only the context that registered
// the callback receives it (in practice always g_os — pages cannot start
// module operations).
struct ModuleCb {
    JsCtxState *owner = nullptr;
    int32_t cb = 0;
};

// ---- host state (defined in app_js.cpp) ----
extern JsCtxState *g_os;   // ROM image + built-in apps; never destroyed
extern JsCtxState *g_fg;   // owns the panel, gestures and the tick
extern uint32_t g_evals;
extern uint32_t g_exceptions;
extern uint32_t g_dispatches;
extern ModuleCb g_module_cb[static_cast<uint8_t>(ModuleId::kCount)];
// Reclaim hook (js_browser.cpp): invoked with the outgoing non-OS
// foreground when the swipe-home grammar returns control to the OS.
extern void (*g_page_reclaim)(JsCtxState *st);

// ---- context lifecycle (defined in app_js.cpp) ----

// Allocates an arena + engine context with the given stdlib and registers
// it. `image`/`image_len`: optional bytecode to relocate+load BEFORE any
// eval (OS core only). Runs the two-line kernel eval. nullptr on failure.
JsCtxState *CreateContext(CtxKind kind, uint32_t arena_bytes,
                          const JSSTDLibraryDef *stdlib,
                          const uint8_t *image, uint32_t image_len);

// Frees the engine context and its arena. Refuses g_os. If st was the
// foreground, the foreground moves to g_os first (widget arena reset).
void DestroyContext(JsCtxState *st);

// Makes st the panel owner: resets the shared widget arena (the previous
// foreground's widgets die by generation bump) and clears st's stale
// page cursor. The new foreground must present before gestures resume
// (JsScreenActive's present-count guard covers the gap).
void SwitchForeground(JsCtxState *st);

// Resolves the state a binding was invoked in (engine context opaque).
JsCtxState *StateOf(JSContext *ctx);

// ---- helpers (defined in app_js.cpp) ----

void RecordException(JsCtxState *st, const char *where);

// Deadline-bounded eval into an arbitrary context.
StatusCode EvalInto(JsCtxState *st, const char *code, uint32_t timeout_ms,
                    const char *name);
// Back-compat wrapper: eval into the OS context (kernel, probes).
StatusCode EvalBounded(const char *code, uint32_t timeout_ms,
                       const char *name);

// Core of load(): read `path` ("rom:<app>", "page:<name>" or an
// SD-rooted virtual path) and evaluate it in st with JS_EVAL_RETVAL,
// returning the file's value (exception on failure).
JSValue LoadInto(JsCtxState *st, const char *path);

// Opaque packing for Widget/Page class instances.
void *PackWidget(s3paper::WidgetHandle h);
s3paper::WidgetHandle UnpackWidget(void *opaque);
void *PackPage(uint32_t index, uint16_t generation);
PageEntry *UnpackPage(JsCtxState *st, void *opaque);

// Wraps a widget-creation result as a Widget class instance (throws
// "widget arena full" on capacity).
JSValue MakeWidget(JSContext *ctx, s3paper::Result<s3paper::WidgetHandle> r);

// Resolves `this` as a live Widget node; nullptr means *err is set.
s3paper::WidgetNode *ThisNode(JSContext *ctx, JSValue *this_val,
                              s3paper::WidgetHandle *out_handle,
                              JSValue *err);

// Resolves `this` as a live Page entry in the calling context; nullptr
// means *err is set.
PageEntry *ThisPage(JSContext *ctx, JSValue *this_val, JSValue *err);

// Copies a JS string argument into a bounded buffer (GC-safe: the copy
// happens before any further allocating call).
bool ArgString(JSContext *ctx, JSValue arg, char *out, size_t cap,
               JSValue *err);

// Registers a JS closure in the calling context's __cbs array (which
// roots it against the compacting GC); returns its id, 0 on failure.
int32_t RegisterCb(JSContext *ctx, JSValue fn);

// Registers fn as the module's single pending completion callback, owned
// by the calling context. Returns false with *err set.
bool RegisterModuleCb(JSContext *ctx, ModuleId module, JSValue fn,
                      JSValue *err);
// Clears a pending module completion (op failed to start).
void CancelModuleCb(ModuleId module);

// Calls st's __cbs[id](a, b, c) under a deadline; JS_UNDEFINED when
// unset or throwing (exception recorded).
JSValue CallCbIn(JsCtxState *st, int32_t cb_id, int32_t a, int32_t b,
                 int32_t c, int argc);

// Presents a page owned by st. mode: 0 = partial, 1 = clean full,
// 2 = diff update (previous hit regions kept — runtime invariant).
// Only legal when st == g_fg.
StatusCode PresentPage(JsCtxState *st, PageEntry &page, int mode);

}  // namespace jsi
}  // namespace pulp
