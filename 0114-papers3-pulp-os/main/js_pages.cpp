// Page class + the paper singleton. Pages are retained-by-name entries in
// the native page table; show()/update() drive the runtime presents.
#include <cstring>

#include "app_js_internal.h"
#include "s3paper/refresh_planner.h"
#include "s3paper_runtime/runtime.h"

namespace pulp {

using namespace jsi;

extern "C" {

// page(name): retained by name — calling again returns the same page.
JSValue js_pulp_page(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "page(name)");
    }
    char name[16];
    JSValue err;
    if (!ArgString(ctx, argv[0], name, sizeof(name), &err)) {
        return err;
    }
    int32_t index = -1;
    for (uint32_t i = 0; i < kMaxPages; ++i) {
        if (g_pages[i].in_use &&
            strncmp(g_pages[i].name, name, sizeof(name)) == 0) {
            index = static_cast<int32_t>(i);
            break;
        }
    }
    if (index < 0) {
        for (uint32_t i = 0; i < kMaxPages; ++i) {
            if (!g_pages[i].in_use) {
                index = static_cast<int32_t>(i);
                PageEntry &page = g_pages[i];
                const uint16_t generation = page.generation;
                page = PageEntry{};
                page.generation = generation;
                page.in_use = true;
                snprintf(page.name, sizeof(page.name), "%s", name);
                break;
            }
        }
    }
    if (index < 0) {
        return JS_ThrowTypeError(ctx, "page table full");
    }
    const JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_PAGE);
    if (JS_IsException(obj)) {
        return obj;
    }
    JS_SetOpaque(ctx, obj,
                 PackPage(static_cast<uint32_t>(index),
                          g_pages[index].generation));
    return obj;
}

JSValue js_page_ctor(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_ThrowTypeError(ctx, "use the page(name) factory");
}

void js_page_finalizer(JSContext *, void *) {
    // No-op: the native page table owns entries; wrappers are views.
}

static JSValue PageSetSlot(JSContext *ctx, JSValue *this_val, int argc,
                           JSValue *argv, int slot) {
    JSValue err;
    PageEntry *page = ThisPage(ctx, this_val, &err);
    if (page == nullptr) return err;
    if (argc < 1 || JS_GetClassID(ctx, argv[0]) != JS_CLASS_WIDGET) {
        return JS_ThrowTypeError(ctx, "expecting Widget");
    }
    const s3paper::WidgetHandle h =
        UnpackWidget(JS_GetOpaque(ctx, argv[0]));
    if (s3paper_runtime::Arena().Get(h) == nullptr) {
        return JS_ThrowTypeError(ctx, "stale widget handle");
    }
    switch (slot) {
        case 0: page->slots.header = h; break;
        case 1: page->slots.content = h; break;
        case 2: page->slots.footer = h; break;
        default: page->slots.overlay = h; break;
    }
    return *this_val;
}

JSValue js_p_header(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv) {
    return PageSetSlot(ctx, this_val, argc, argv, 0);
}

JSValue js_p_content(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv) {
    return PageSetSlot(ctx, this_val, argc, argv, 1);
}

JSValue js_p_footer(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv) {
    return PageSetSlot(ctx, this_val, argc, argv, 2);
}

JSValue js_p_overlay(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv) {
    return PageSetSlot(ctx, this_val, argc, argv, 3);
}

// on(gesture, fn): gesture 0..5 = GestureKind, 100 = tick.
JSValue js_p_on(JSContext *ctx, JSValue *this_val, int argc,
                JSValue *argv) {
    JSValue err;
    PageEntry *page = ThisPage(ctx, this_val, &err);
    if (page == nullptr) return err;
    if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "on(gesture, fn)");
    }
    int kind = -1;
    if (JS_ToInt32(ctx, &kind, argv[0])) return JS_EXCEPTION;
    const int32_t id = RegisterCb(ctx, argv[1]);
    if (id == 0) {
        return JS_EXCEPTION;
    }
    if (kind >= 0 && kind < 6) {
        page->gesture_cb[kind] = id;
    } else if (kind == 100) {
        page->tick_cb = id;
    } else {
        return JS_ThrowTypeError(ctx, "bad gesture kind");
    }
    return *this_val;
}

JSValue js_p_every(JSContext *ctx, JSValue *this_val, int argc,
                   JSValue *argv) {
    JSValue err;
    PageEntry *page = ThisPage(ctx, this_val, &err);
    if (page == nullptr) return err;
    int ms = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &ms, argv[0])) return JS_EXCEPTION;
    if (ms != 0 && ms < 250) {
        ms = 250;  // owner-loop tick resolution floor
    }
    page->every_ms = static_cast<uint32_t>(ms < 0 ? 0 : ms);
    return *this_val;
}

JSValue js_p_show(JSContext *ctx, JSValue *this_val, int argc,
                  JSValue *argv) {
    JSValue err;
    PageEntry *page = ThisPage(ctx, this_val, &err);
    if (page == nullptr) return err;
    int full = 0;
    if (argc >= 1 && !JS_IsUndefined(argv[0])) {
        int v = 0;
        if (JS_ToInt32(ctx, &v, argv[0])) {
            return JS_EXCEPTION;
        }
        full = v != 0;
    }
    const StatusCode presented = PresentPage(*page, full ? 1 : 0);
    return JS_NewInt32(ctx, static_cast<int32_t>(presented));
}

JSValue js_p_update(JSContext *ctx, JSValue *this_val, int, JSValue *) {
    JSValue err;
    PageEntry *page = ThisPage(ctx, this_val, &err);
    if (page == nullptr) return err;
    const StatusCode presented = PresentPage(*page, 2);
    return JS_NewInt32(ctx, static_cast<int32_t>(presented));
}

// ---- paper singleton ----

JSValue js_paper_home(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "paper.home(fn)");
    }
    const int32_t id = RegisterCb(ctx, argv[0]);
    if (id == 0) {
        return JS_EXCEPTION;
    }
    g_home_cb = id;
    return JS_UNDEFINED;
}

JSValue js_paper_sleep_image(JSContext *ctx, JSValue *, int argc,
                             JSValue *argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "paper.sleepImage(fn)");
    }
    const int32_t id = RegisterCb(ctx, argv[0]);
    if (id == 0) {
        return JS_EXCEPTION;
    }
    g_sleep_image_cb = id;
    return JS_UNDEFINED;
}

JSValue js_paper_refresh_turns(JSContext *ctx, JSValue *, int argc,
                               JSValue *argv) {
    int turns = 0;
    if (argc < 1 || JS_ToInt32(ctx, &turns, argv[0])) {
        return JS_EXCEPTION;
    }
    if (turns < 1 || turns > 4096) {
        return JS_ThrowTypeError(ctx, "bad turn budget");
    }
    s3paper::RefreshPolicy policy = s3paper_runtime::Planner().policy();
    policy.max_turns_between_full = static_cast<uint32_t>(turns);
    s3paper_runtime::Planner().set_policy(policy);
    return JS_UNDEFINED;
}

JSValue js_paper_version(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(kAbiVersion));
}

}  // extern "C"

}  // namespace pulp
