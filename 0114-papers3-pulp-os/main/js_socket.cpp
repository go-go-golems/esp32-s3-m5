#include "app_js_internal.h"
#include "net_socket.h"

namespace pulp {
using namespace jsi;

namespace {
JSValue ThrowStatus(JSContext *ctx, const char *what, StatusCode status) {
    return JS_ThrowTypeError(ctx, "%s: %s", what, StatusCodeName(status));
}
}

extern "C" {

JSValue js_socket_open(JSContext *ctx, JSValue *this_val, int argc,
                       JSValue *argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "open(url)");
    char url[256];
    JSValue error;
    if (!ArgString(ctx, argv[0], url, sizeof(url), &error)) return error;
    const StatusCode status = SocketBegin(url);
    if (status != StatusCode::Ok) return ThrowStatus(ctx, "socket.open", status);
    return *this_val;
}

JSValue js_socket_bearer(JSContext *ctx, JSValue *this_val, int, JSValue *) {
    const StatusCode status = SocketBearer();
    if (status != StatusCode::Ok) {
        return ThrowStatus(ctx, "socket.bearer", status);
    }
    return *this_val;
}

JSValue js_socket_start(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(SocketStart()));
}
JSValue js_socket_stop(JSContext *ctx, JSValue *, int, JSValue *) {
    SocketStop();
    return JS_UNDEFINED;
}
JSValue js_socket_state(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, SocketStatus());
}
JSValue js_socket_state_name(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, SocketStateName());
}
JSValue js_socket_message_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(SocketMessageCount()));
}
JSValue js_socket_received(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(SocketReceived()));
}
JSValue js_socket_dropped(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(SocketDropped()));
}
JSValue js_socket_error(JSContext *ctx, JSValue *, int, JSValue *) {
    SocketSnapshot snapshot;
    FillSocketSnapshot(&snapshot);
    return JS_NewString(ctx, snapshot.error);
}
JSValue js_socket_message_seq(JSContext *ctx, JSValue *, int argc,
                              JSValue *argv) {
    int index = -1;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) return JS_EXCEPTION;
    char message[513];
    uint64_t seq = 0;
    if (index < 0 || !SocketCopyMessage(static_cast<uint32_t>(index), message,
                                        sizeof(message), &seq)) {
        return JS_ThrowTypeError(ctx, "message index out of range");
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(seq & 0x7fffffff));
}
JSValue js_socket_message(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = -1;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) return JS_EXCEPTION;
    char message[513];
    uint64_t seq = 0;
    if (index < 0 || !SocketCopyMessage(static_cast<uint32_t>(index), message,
                                        sizeof(message), &seq)) {
        return JS_ThrowTypeError(ctx, "message index out of range");
    }
    return JS_NewString(ctx, message);
}

}  // extern "C"
}  // namespace pulp
