// wifi_namespace.cpp — QuickJS WiFi status/request namespace for 0103 AtomS3R M12.
#include "wifi_namespace.h"

#include <stdio.h>

#include "esp_err.h"
#include "esp_netif_ip_addr.h"
#include "lwip/inet.h"
#include "wifi_app.h"

namespace {
const char *state_to_js(wifi_app_state_t state)
{
    switch (state) {
        case WIFI_APP_STATE_UNINIT:
            return "uninit";
        case WIFI_APP_STATE_IDLE:
            return "idle";
        case WIFI_APP_STATE_CONNECTING:
            return "connecting";
        case WIFI_APP_STATE_CONNECTED:
            return "connected";
        default:
            return "unknown";
    }
}

void ip_to_string(uint32_t ip4_host_order, char *out, size_t out_len)
{
    if (!out || out_len == 0) {
        return;
    }
    out[0] = '\0';
    if (ip4_host_order == 0) {
        return;
    }
    ip4_addr_t ip = {.addr = htonl(ip4_host_order)};
    snprintf(out, out_len, IPSTR, IP2STR(&ip));
}

JSValue throw_esp_error(JSContext *ctx, const char *operation, esp_err_t err)
{
    return JS_ThrowInternalError(ctx, "%s: %s", operation, esp_err_to_name(err));
}

JSValue make_request_result(JSContext *ctx, const char *requested, esp_err_t err)
{
    if (err != ESP_OK) {
        return throw_esp_error(ctx, requested, err);
    }
    wifi_app_status_t st = {};
    (void)wifi_app_get_status(&st);
    JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "ok", JS_NewBool(ctx, true), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "requested", JS_NewString(ctx, requested), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "state", JS_NewString(ctx, state_to_js(st.state)), JS_PROP_ENUMERABLE);
    return obj;
}

JSValue js_wifi_status(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;

    wifi_app_status_t st = {};
    esp_err_t err = wifi_app_get_status(&st);
    if (err != ESP_OK) {
        return throw_esp_error(ctx, "wifi.status", err);
    }

    char sta_ip[32] = {};
    char ap_ip[32] = {};
    ip_to_string(st.sta_ip4, sta_ip, sizeof(sta_ip));
    ip_to_string(st.ap_ip4, ap_ip, sizeof(ap_ip));

    JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "state", JS_NewString(ctx, state_to_js(st.state)), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "ssid", JS_NewString(ctx, st.ssid), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "hasSavedCredentials", JS_NewBool(ctx, st.has_saved_creds), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "hasRuntimeCredentials", JS_NewBool(ctx, st.has_runtime_creds), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "staIp", JS_NewString(ctx, sta_ip), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "apIp", JS_NewString(ctx, ap_ip), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "lastDisconnectReason", JS_NewInt32(ctx, st.last_disconnect_reason), JS_PROP_ENUMERABLE);
    return obj;
}

JSValue js_wifi_connect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return make_request_result(ctx, "connect", wifi_app_connect());
}

JSValue js_wifi_disconnect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return make_request_result(ctx, "disconnect", wifi_app_disconnect());
}

JSValue js_wifi_clear_credentials(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return make_request_result(ctx, "clearCredentials", wifi_app_clear_credentials());
}

bool set_function(JSContext *ctx, JSValueConst obj, const char *name, JSCFunction *fn, int argc)
{
    JSValue f = JS_NewCFunction(ctx, fn, name, argc);
    if (JS_IsException(f)) {
        return false;
    }
    return JS_DefinePropertyValueStr(ctx, obj, name, f, JS_PROP_ENUMERABLE) >= 0;
}

esp_err_t install_wifi_namespace_job(JSContext *ctx, void *user)
{
    (void)user;
    if (!ctx) {
        return ESP_ERR_INVALID_ARG;
    }

    JSValue wifi = JS_NewObject(ctx);
    if (JS_IsException(wifi)) {
        return ESP_ERR_NO_MEM;
    }

    bool ok = set_function(ctx, wifi, "status", js_wifi_status, 0) &&
              set_function(ctx, wifi, "connect", js_wifi_connect, 0) &&
              set_function(ctx, wifi, "disconnect", js_wifi_disconnect, 0) &&
              set_function(ctx, wifi, "clearCredentials", js_wifi_clear_credentials, 0);

    if (ok && JS_PreventExtensions(ctx, wifi) < 0) {
        ok = false;
    }

    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) {
        JS_FreeValue(ctx, wifi);
        return ESP_FAIL;
    }
    if (ok) {
        const int rc = JS_DefinePropertyValueStr(ctx, global, "wifi", wifi, JS_PROP_ENUMERABLE);
        wifi = JS_UNDEFINED;
        ok = rc >= 0;
    }
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, wifi);
    return ok ? ESP_OK : ESP_FAIL;
}
}  // namespace

esp_err_t install_wifi_namespace(qjs_service_t *svc)
{
    if (!svc) {
        return ESP_ERR_INVALID_ARG;
    }
    qjs_job_t job = {};
    job.fn = install_wifi_namespace_job;
    job.timeout_ms = 1000;
    return qjs_service_run(svc, &job);
}
