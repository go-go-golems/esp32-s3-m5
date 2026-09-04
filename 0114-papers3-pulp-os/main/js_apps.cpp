// ESP-55 P4/P5: the `apps` singleton — ROM asset registry surface for the
// OS core's seeding pass, sync small-text writes (manifests, pulled
// installs), and the /apps/upload completion (module-cb pattern,
// ModuleId::Apps). Owner-task-only like every binding.
#include <cstring>

#include "app_files.h"
#include "app_js_internal.h"
#include "js_assets.h"
#include "net_serve.h"

namespace pulp {

using namespace jsi;

extern "C" {

JSValue js_apps_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(AssetsCount()));
}

JSValue js_apps_name(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const char *name =
        index >= 0 ? AssetsName(static_cast<uint32_t>(index)) : nullptr;
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "asset index out of range");
    }
    return JS_NewString(ctx, name);
}

JSValue js_apps_copy(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "(name, path) expected");
    }
    char name[40];
    char path[kFilesMaxPath];
    JSValue err;
    if (!ArgString(ctx, argv[0], name, sizeof(name), &err) ||
        !ArgString(ctx, argv[1], path, sizeof(path), &err)) {
        return err;
    }
    return JS_NewInt32(ctx, AssetsCopy(name, path));
}

JSValue js_apps_write_text(JSContext *ctx, JSValue *, int argc,
                           JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "(path, body) expected");
    }
    char path[kFilesMaxPath];
    JSValue err;
    if (!ArgString(ctx, argv[0], path, sizeof(path), &err)) {
        return err;
    }
    // No allocating JS call between ToCString and the write (GC safety).
    JSCStringBuf buf;
    size_t len;
    const char *body = JS_ToCStringLen(ctx, &len, argv[1], &buf);
    if (body == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(
        ctx, AssetsWriteText(path, body, static_cast<uint32_t>(len)));
}

JSValue js_apps_received(JSContext *ctx, JSValue *, int argc,
                         JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "(fn) expected");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Apps, argv[0], &err)) {
        return err;
    }
    return JS_UNDEFINED;
}

JSValue js_apps_upload_name(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, ServeAppsUploadName());
}

}  // extern "C"

}  // namespace pulp
