#include "app_js_internal.h"
#include "net_auth.h"

namespace pulp {
using namespace jsi;

extern "C" {

JSValue js_auth_configure(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 4) {
        return JS_ThrowTypeError(ctx,
                                 "configure(issuer, clientId, scopes, resource)");
    }
    char issuer[192], client[48], scopes[160], resource[192];
    JSValue error;
    if (!ArgString(ctx, argv[0], issuer, sizeof(issuer), &error) ||
        !ArgString(ctx, argv[1], client, sizeof(client), &error) ||
        !ArgString(ctx, argv[2], scopes, sizeof(scopes), &error) ||
        !ArgString(ctx, argv[3], resource, sizeof(resource), &error)) {
        return error;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(
                                AuthConfigure(issuer, client, scopes, resource)));
}

JSValue js_auth_start(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(AuthStart()));
}
JSValue js_auth_state(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, AuthStatus());
}
JSValue js_auth_state_name(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, AuthStateName());
}
JSValue js_auth_user_code(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, AuthUserCode());
}
JSValue js_auth_verification_uri(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, AuthVerificationUri());
}
JSValue js_auth_verification_uri_complete(JSContext *ctx, JSValue *, int,
                                          JSValue *) {
    return JS_NewString(ctx, AuthVerificationUriComplete());
}
JSValue js_auth_error(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, AuthErrorName());
}
JSValue js_auth_grant_seconds(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, AuthGrantSecondsLeft());
}
JSValue js_auth_token_seconds(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, AuthTokenSecondsLeft());
}
JSValue js_auth_poll_seconds(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, AuthPollSecondsLeft());
}
JSValue js_auth_clear(JSContext *ctx, JSValue *, int, JSValue *) {
    AuthClear();
    return JS_UNDEFINED;
}

}  // extern "C"
}  // namespace pulp
