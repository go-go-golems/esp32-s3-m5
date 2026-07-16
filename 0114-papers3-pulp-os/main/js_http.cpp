// http singleton bindings (ESP-53 P4): a builder with a terminal verb.
// get/header/limit/done return the singleton itself for chaining and
// THROW on misuse (a broken chain should fail loudly at the call site);
// send() returns a status int. Completion: fn(kDoneHttp, status, len).
#include "app_js_internal.h"
#include "net_http.h"

namespace pulp {

using namespace jsi;

namespace {

void CancelHttpCb() {
    g_module_cb[static_cast<uint8_t>(ModuleId::Http)] = 0;
}

JSValue ThrowStatus(JSContext *ctx, const char *what, StatusCode status) {
    return JS_ThrowTypeError(ctx, "%s: %s", what, StatusCodeName(status));
}

}  // namespace

extern "C" {

JSValue js_http_get(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "get(url)");
    }
    char url[kHttpMaxUrl];
    JSValue err;
    if (!ArgString(ctx, argv[0], url, sizeof(url), &err)) {
        return err;
    }
    const StatusCode status = HttpBegin(url);
    if (status != StatusCode::Ok) {
        return ThrowStatus(ctx, "http.get", status);
    }
    return *this_val;
}

JSValue js_http_header(JSContext *ctx, JSValue *this_val, int argc,
                       JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "header(key, value)");
    }
    char key[32];
    char value[64];
    JSValue err;
    if (!ArgString(ctx, argv[0], key, sizeof(key), &err) ||
        !ArgString(ctx, argv[1], value, sizeof(value), &err)) {
        return err;
    }
    const StatusCode status = HttpHeader(key, value);
    if (status != StatusCode::Ok) {
        return ThrowStatus(ctx, "http.header", status);
    }
    return *this_val;
}

JSValue js_http_limit(JSContext *ctx, JSValue *this_val, int argc,
                      JSValue *argv) {
    int limit = 0;
    if (argc < 1 || JS_ToInt32(ctx, &limit, argv[0])) {
        return JS_EXCEPTION;
    }
    const StatusCode status =
        HttpLimit(limit > 0 ? static_cast<uint32_t>(limit) : 0);
    if (status != StatusCode::Ok) {
        return ThrowStatus(ctx, "http.limit", status);
    }
    return *this_val;
}

JSValue js_http_done(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "done(fn)");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Http, argv[0], &err)) {
        return err;
    }
    return *this_val;
}

JSValue js_http_send(JSContext *ctx, JSValue *, int, JSValue *) {
    const StatusCode status = HttpSend();
    if (status != StatusCode::Ok) {
        CancelHttpCb();
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

JSValue js_http_abort(JSContext *ctx, JSValue *, int, JSValue *) {
    HttpAbort();
    return JS_UNDEFINED;
}

JSValue js_http_status(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, HttpStatusCode());
}

JSValue js_http_length(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(HttpLength()));
}

JSValue js_http_body(JSContext *ctx, JSValue *, int, JSValue *) {
    uint32_t len = 0;
    const char *body = HttpBody(&len);
    if (body == nullptr) {
        return JS_NewString(ctx, "");
    }
    return JS_NewStringLen(ctx, body, len);
}

JSValue js_http_body_line_count(JSContext *ctx, JSValue *, int,
                                JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(HttpLineCount()));
}

JSValue js_http_body_line(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    uint32_t len = 0;
    const char *line =
        index >= 0 ? HttpLine(static_cast<uint32_t>(index), &len)
                   : nullptr;
    if (line == nullptr) {
        return JS_ThrowTypeError(ctx, "body line out of range");
    }
    return JS_NewStringLen(ctx, line, len);
}

}  // extern "C"

}  // namespace pulp
