// images singleton bindings (ESP-54 P3/P4). Catalog accessors + display +
// remove are sync (owner-only SD reads); received(fn) registers the
// upload-completion callback via the module-cb path (the ESP-53 mailbox
// pattern). display() blits a stored .g4 via DrawOpKind::Bitmap.
#include "app_js_internal.h"
#include "app_images.h"
#include "app_events.h"

namespace pulp {

using namespace jsi;

extern "C" {

JSValue js_images_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(ImagesCount()));
}

JSValue js_images_name(JSContext *ctx, JSValue *, int argc,
                       JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const char *name =
        index >= 0 ? ImagesName(static_cast<uint32_t>(index)) : nullptr;
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "image index out of range");
    }
    return JS_NewString(ctx, name);
}

JSValue js_images_display(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "display(name)");
    }
    char name[32];
    JSValue err;
    if (!ArgString(ctx, argv[0], name, sizeof(name), &err)) {
        return err;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(ImagesDisplay(name)));
}

JSValue js_images_remove(JSContext *ctx, JSValue *, int argc,
                         JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "remove(name)");
    }
    char name[32];
    JSValue err;
    if (!ArgString(ctx, argv[0], name, sizeof(name), &err)) {
        return err;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(ImagesRemove(name)));
}

JSValue js_images_received(JSContext *ctx, JSValue *, int argc,
                           JSValue *argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "received(fn)");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Images, argv[0], &err)) {
        return err;
    }
    return JS_UNDEFINED;
}

}  // extern "C"

}  // namespace pulp
