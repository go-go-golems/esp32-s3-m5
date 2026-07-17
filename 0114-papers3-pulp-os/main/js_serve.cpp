// serve singleton bindings (ESP-53 P5). Route handlers are SYNCHRONOUS:
// they return a response token minted by serve.text()/json()/status(),
// which writes the pending response slot — nothing async lives inside a
// route. Route callbacks are rooted like any other (__cbs) and survive
// until resetTree, which also clears the native route table.
#include "app_js_internal.h"
#include "net_serve.h"

namespace pulp {

using namespace jsi;

namespace {

char s_staged_path[kServeMaxPath];

}  // namespace

// Owner-loop entry (called from ServeOwnerDispatch): run one route
// callback under the usual deadline. The handler's return value is
// ignored — serve.text/json/status already wrote the response slot.
void JsServeInvokeRoute(int32_t cb_id) {
    g_dispatches++;
    (void)CallCb(cb_id, 1, 0, 0, 1);
}

extern "C" {

JSValue js_serve_get(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "get(path)");
    }
    JSValue err;
    if (!ArgString(ctx, argv[0], s_staged_path, sizeof(s_staged_path),
                   &err)) {
        return err;
    }
    return *this_val;
}

JSValue js_serve_handle(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "handle(fn)");
    }
    if (s_staged_path[0] == '\0') {
        return JS_ThrowTypeError(ctx, "handle() without get(path)");
    }
    const int32_t cb_id = RegisterCb(ctx, argv[0]);
    if (cb_id == 0) {
        return JS_EXCEPTION;
    }
    const StatusCode added = ServeRouteAdd(s_staged_path, cb_id);
    s_staged_path[0] = '\0';
    if (added != StatusCode::Ok) {
        return JS_ThrowTypeError(ctx, "route add: %s",
                                 StatusCodeName(added));
    }
    return *this_val;
}

JSValue js_serve_text(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "text(body)");
    }
    JSCStringBuf buf;
    size_t len;
    const char *body = JS_ToCStringLen(ctx, &len, argv[0], &buf);
    if (body == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(
        ctx, ServeRespond(200, 0, body, static_cast<uint32_t>(len)) ? 1
                                                                    : 0);
}

JSValue js_serve_json(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "json(body)");
    }
    JSCStringBuf buf;
    size_t len;
    const char *body = JS_ToCStringLen(ctx, &len, argv[0], &buf);
    if (body == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(
        ctx, ServeRespond(200, 1, body, static_cast<uint32_t>(len)) ? 1
                                                                    : 0);
}

JSValue js_serve_status(JSContext *ctx, JSValue *, int argc,
                        JSValue *argv) {
    int status = 0;
    if (argc < 1 || JS_ToInt32(ctx, &status, argv[0])) {
        return JS_EXCEPTION;
    }
    if (status < 100 || status > 599) {
        return JS_ThrowTypeError(ctx, "status(100..599)");
    }
    return JS_NewInt32(ctx,
                       ServeRespond(status, 0, "", 0) ? 1 : 0);
}

JSValue js_serve_query(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, ServeRequestQuery());
}

JSValue js_serve_files(JSContext *ctx, JSValue *this_val, int argc,
                       JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "files(prefix, dir)");
    }
    char prefix[8];
    char dir[64];
    JSValue err;
    if (!ArgString(ctx, argv[0], prefix, sizeof(prefix), &err) ||
        !ArgString(ctx, argv[1], dir, sizeof(dir), &err)) {
        return err;
    }
    const StatusCode mounted = ServeFilesMount(prefix, dir);
    if (mounted != StatusCode::Ok) {
        return JS_ThrowTypeError(ctx, "files mount: %s",
                                 StatusCodeName(mounted));
    }
    return *this_val;
}

JSValue js_serve_start(JSContext *ctx, JSValue *, int argc,
                       JSValue *argv) {
    int port = 80;
    if (argc >= 1 && JS_ToInt32(ctx, &port, argv[0])) {
        return JS_EXCEPTION;
    }
    if (port <= 0 || port > 65535) {
        return JS_ThrowTypeError(ctx, "start(port)");
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(ServeStart(
                                static_cast<uint16_t>(port))));
}

JSValue js_serve_stop(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(ServeStop()));
}

JSValue js_serve_url(JSContext *ctx, JSValue *, int, JSValue *) {
    char url[32];
    ServeUrl(url, sizeof(url));
    return JS_NewString(ctx, url);
}

}  // extern "C"

}  // namespace pulp
