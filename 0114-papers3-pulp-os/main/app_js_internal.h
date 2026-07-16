// Internal contract between the JS host core (app_js.cpp) and the binding
// translation units (js_widgets.cpp, js_pages.cpp, js_services.cpp,
// js_probes.cpp). NOT for use outside main/js_*.cpp — firmware code uses
// app_js.h. Everything here is owner-task-only.
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

// ---- host state (defined in app_js.cpp) ----
extern JSContext *g_ctx;
extern uint32_t g_evals;
extern uint32_t g_exceptions;
extern uint32_t g_dispatches;
extern PageEntry g_pages[kMaxPages];
extern int32_t g_current_page;
extern DynEntry g_dyn[kMaxDynValues];
extern uint32_t g_dyn_count;
extern int32_t g_next_cb;
extern int32_t g_home_cb;
extern int32_t g_sleep_image_cb;
// One pending completion callback per module (ESP-53); 0 = none. Bindings
// set via RegisterModuleCb; JsModuleDone consumes; resetTree clears.
extern int32_t g_module_cb[static_cast<uint8_t>(ModuleId::kCount)];
extern s3paper::HitRegion g_hits[kMaxJsHits];
extern uint32_t g_hit_count;
extern int64_t g_timer_due_us;

// ---- helpers (defined in app_js.cpp) ----

void RecordException(const char *where);
StatusCode EvalBounded(const char *code, uint32_t timeout_ms,
                       const char *name);

// Opaque packing for Widget/Page class instances.
void *PackWidget(s3paper::WidgetHandle h);
s3paper::WidgetHandle UnpackWidget(void *opaque);
void *PackPage(uint32_t index, uint16_t generation);
PageEntry *UnpackPage(void *opaque);

// Wraps a widget-creation result as a Widget class instance (throws
// "widget arena full" on capacity).
JSValue MakeWidget(JSContext *ctx, s3paper::Result<s3paper::WidgetHandle> r);

// Resolves `this` as a live Widget node; nullptr means *err is set.
s3paper::WidgetNode *ThisNode(JSContext *ctx, JSValue *this_val,
                              s3paper::WidgetHandle *out_handle,
                              JSValue *err);

// Resolves `this` as a live Page entry; nullptr means *err is set.
PageEntry *ThisPage(JSContext *ctx, JSValue *this_val, JSValue *err);

// Copies a JS string argument into a bounded buffer (GC-safe: the copy
// happens before any further allocating call).
bool ArgString(JSContext *ctx, JSValue arg, char *out, size_t cap,
               JSValue *err);

// Registers a JS closure in the kernel's __cbs array (which roots it
// against the compacting GC); returns its id, 0 on failure.
int32_t RegisterCb(JSContext *ctx, JSValue fn);

// Registers fn as the module's single pending completion callback.
// Returns false with *err set (TypeError on a non-function, Busy-style
// TypeError when an operation is already in flight).
bool RegisterModuleCb(JSContext *ctx, ModuleId module, JSValue fn,
                      JSValue *err);

// Calls __cbs[id](a, b, c) under a deadline; JS_UNDEFINED when unset or
// throwing (exception recorded).
JSValue CallCb(int32_t cb_id, int32_t a, int32_t b, int32_t c, int argc);

// Presents a page's slots. mode: 0 = partial, 1 = clean full, 2 = diff
// update (previous hit regions kept — runtime invariant).
StatusCode PresentPage(PageEntry &page, int mode);

}  // namespace jsi
}  // namespace pulp
