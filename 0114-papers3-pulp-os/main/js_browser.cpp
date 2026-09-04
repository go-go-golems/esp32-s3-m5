// ESP-55 P9/P10: the page runtime. browser.run(path, url) executes a page
// script — a descriptor ({title, main(ui, nav)}) — in a fresh sandboxed
// context (UI stdlib: only drawing + nav are callable). nav.* records a
// request in a native mailbox and posts ModuleDone{Nav}; the browser app
// (OS context) watches and decides. Owner-task-only.
#include <cstdio>
#include <cstring>

#include "app_files.h"
#include "app_js_internal.h"
#include "app_owner.h"
#include "esp_log.h"
#include "js_assets.h"

extern "C" const JSSTDLibraryDef js_stdlib_ui;  // main/js_stdlib_table_ui.c

namespace pulp {

using namespace jsi;

namespace {

const char *kTag = "browser";

constexpr uint32_t kPageArenaBytes = 96 * 1024;
constexpr int32_t kNavGo = 1;
constexpr int32_t kNavBack = 2;
constexpr int32_t kNavReload = 3;

JsCtxState *s_page = nullptr;
char s_page_url[256] = "";
int32_t s_nav_kind = 0;
char s_nav_url[256] = "";

void ReclaimPage(JsCtxState *st) {
    if (st == s_page && s_page != nullptr) {
        ESP_LOGI(kTag, "page reclaimed (home gesture)");
        DestroyContext(s_page);
        s_page = nullptr;
        s_page_url[0] = '\0';
    }
}

void TeardownPage() {
    if (s_page != nullptr) {
        DestroyContext(s_page);  // switches foreground to g_os if needed
        s_page = nullptr;
        s_page_url[0] = '\0';
    }
}

// Creates the page context, evaluates the shared ui helpers, loads the
// page script, validates the descriptor and runs main(ui, nav).
StatusCode RunPage(const char *path, const char *url) {
    TeardownPage();
    s_page = CreateContext(CtxKind::kPage, kPageArenaBytes, &js_stdlib_ui,
                           nullptr, 0);
    if (s_page == nullptr) {
        return StatusCode::OutOfMemory;
    }
    const char *helpers = nullptr;
    uint32_t hlen = 0;
    if (!PageAssetsFind("ui-helpers", &helpers, &hlen) ||
        EvalInto(s_page, helpers, 2000, "<ui-helpers>") !=
            StatusCode::Ok) {
        TeardownPage();
        return StatusCode::CorruptData;
    }
    const JSValue desc = LoadInto(s_page, path);
    if (JS_IsException(desc)) {
        TeardownPage();
        return StatusCode::CorruptData;
    }
    // Root the descriptor as a page-context global (property set is safe
    // with a fresh value; storing roots it against the compacting GC).
    const JSValue global = JS_GetGlobalObject(s_page->ctx);
    if (JS_IsException(
            JS_SetPropertyStr(s_page->ctx, global, "__page", desc))) {
        TeardownPage();
        return StatusCode::CorruptData;
    }
    if (EvalInto(s_page,
                 "if (!__page || typeof __page.main !== 'function') {"
                 "  throw new TypeError('bad page descriptor');"
                 "}",
                 1000, "<page-validate>") != StatusCode::Ok) {
        TeardownPage();
        return StatusCode::CorruptData;
    }
    snprintf(s_page_url, sizeof(s_page_url), "%s", url);
    SwitchForeground(s_page);
    if (EvalInto(s_page, "__page.main(ui, nav);", 3000, "<page-main>") !=
        StatusCode::Ok) {
        TeardownPage();  // switches back to g_os
        return StatusCode::CorruptData;
    }
    return StatusCode::Ok;
}

void PostNav(int32_t kind, const char *url) {
    s_nav_kind = kind;
    snprintf(s_nav_url, sizeof(s_nav_url), "%s", url);
    // Deferred through the owner queue: the browser's watcher runs on a
    // LATER loop pass, safely outside the page context's call frame (so
    // it may teardown the page context that posted this).
    (void)PostModuleDone(ModuleId::Nav, kDoneNavRequest, kind, 0);
}

struct HookInstaller {
    HookInstaller() { g_page_reclaim = &ReclaimPage; }
};
HookInstaller s_hook;

}  // namespace

extern "C" {

// ---- nav (callable from pages; the request is advisory) ----

JSValue js_nav_go(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    char url[256];
    JSValue err;
    if (argc < 1 || !ArgString(ctx, argv[0], url, sizeof(url), &err)) {
        return JS_ThrowTypeError(ctx, "nav.go(url)");
    }
    PostNav(kNavGo, url);
    return JS_UNDEFINED;
}

JSValue js_nav_back(JSContext *ctx, JSValue *, int, JSValue *) {
    (void)ctx;
    PostNav(kNavBack, "");
    return JS_UNDEFINED;
}

JSValue js_nav_reload(JSContext *ctx, JSValue *, int, JSValue *) {
    (void)ctx;
    PostNav(kNavReload, "");
    return JS_UNDEFINED;
}

JSValue js_nav_url(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, s_page_url);
}

// ---- browser (OS context only; denied in the UI stdlib) ----

JSValue js_browser_run(JSContext *ctx, JSValue *, int argc,
                       JSValue *argv) {
    char path[kFilesMaxPath];
    char url[256] = "";
    JSValue err;
    if (argc < 1 || !ArgString(ctx, argv[0], path, sizeof(path), &err)) {
        return JS_ThrowTypeError(ctx, "browser.run(path[, url])");
    }
    if (argc >= 2 && !ArgString(ctx, argv[1], url, sizeof(url), &err)) {
        return err;
    }
    return JS_NewInt32(ctx,
                       static_cast<int32_t>(RunPage(path, url)));
}

JSValue js_browser_close(JSContext *ctx, JSValue *, int, JSValue *) {
    TeardownPage();
    return JS_NewInt32(ctx, 0);
}

JSValue js_browser_watch(JSContext *ctx, JSValue *, int argc,
                         JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "(fn) expected");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Nav, argv[0], &err)) {
        return err;
    }
    return JS_UNDEFINED;
}

JSValue js_browser_nav_url(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, s_nav_url);
}

JSValue js_browser_nav_kind(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, s_nav_kind);
}

}  // extern "C"

}  // namespace pulp
