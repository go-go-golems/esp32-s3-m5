// wifi singleton bindings (ESP-53 P3). Async verbs use the Wifi module
// callback slot; status and mailbox accessors are synchronous.
#include "app_js_internal.h"
#include "net_wifi.h"
#include "s3paper_storage/storage.h"

namespace pulp {

using namespace jsi;

namespace {

void CancelWifiCb() {
    g_module_cb[static_cast<uint8_t>(ModuleId::Wifi)] = 0;
}

}  // namespace

extern "C" {

JSValue js_wifi_status(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, WifiStatus());
}

JSValue js_wifi_ip(JSContext *ctx, JSValue *, int, JSValue *) {
    char ip[16];
    WifiIp(ip, sizeof(ip));
    return JS_NewString(ctx, ip);
}

JSValue js_wifi_ssid_current(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, WifiSsidCurrent());
}

JSValue js_wifi_rssi_current(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, WifiRssiCurrent());
}

JSValue js_wifi_scan(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "scan(fn)");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Wifi, argv[0], &err)) {
        return err;
    }
    const StatusCode status = WifiScan();
    if (status != StatusCode::Ok) {
        CancelWifiCb();
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

JSValue js_wifi_join(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 3) {
        return JS_ThrowTypeError(ctx, "join(ssid, pass, fn)");
    }
    char ssid[33];
    char pass[65];
    JSValue err;
    if (!ArgString(ctx, argv[0], ssid, sizeof(ssid), &err) ||
        !ArgString(ctx, argv[1], pass, sizeof(pass), &err)) {
        return err;
    }
    if (!RegisterModuleCb(ctx, ModuleId::Wifi, argv[2], &err)) {
        return err;
    }
    const StatusCode status = WifiJoin(ssid, pass);
    if (status != StatusCode::Ok) {
        CancelWifiCb();
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

JSValue js_wifi_join_saved(JSContext *ctx, JSValue *, int argc,
                           JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "joinSaved(fn)");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Wifi, argv[0], &err)) {
        return err;
    }
    const StatusCode status = WifiJoinSaved();
    if (status != StatusCode::Ok) {
        CancelWifiCb();
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

JSValue js_wifi_save(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "save(ssid, pass)");
    }
    char ssid[33];
    char pass[65];
    JSValue err;
    if (!ArgString(ctx, argv[0], ssid, sizeof(ssid), &err) ||
        !ArgString(ctx, argv[1], pass, sizeof(pass), &err)) {
        return err;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(
                                s3paper_storage::WifiCredsSet(ssid, pass)));
}

JSValue js_wifi_forget(JSContext *ctx, JSValue *, int argc,
                       JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "forget(ssid)");
    }
    char ssid[33];
    JSValue err;
    if (!ArgString(ctx, argv[0], ssid, sizeof(ssid), &err)) {
        return err;
    }
    return JS_NewInt32(
        ctx, static_cast<int32_t>(s3paper_storage::WifiCredsForget(ssid)));
}

JSValue js_wifi_saved_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(
        ctx, static_cast<int32_t>(s3paper_storage::WifiCredsCount()));
}

JSValue js_wifi_saved_ssid(JSContext *ctx, JSValue *, int argc,
                           JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const s3paper_storage::WifiCredential *cred =
        index >= 0 ? s3paper_storage::WifiCredsGetRanked(
                         static_cast<uint32_t>(index))
                   : nullptr;
    if (cred == nullptr) {
        return JS_ThrowTypeError(ctx, "saved index out of range");
    }
    return JS_NewString(ctx, cred->ssid);
}

JSValue js_wifi_off(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(WifiOff()));
}

JSValue js_wifi_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(WifiScanCount()));
}

JSValue js_wifi_ssid(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const char *ssid =
        index >= 0 ? WifiScanSsid(static_cast<uint32_t>(index)) : nullptr;
    if (ssid == nullptr) {
        return JS_ThrowTypeError(ctx, "scan index out of range");
    }
    return JS_NewString(ctx, ssid);
}

JSValue js_wifi_rssi(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(ctx, WifiScanRssi(static_cast<uint32_t>(index)));
}

JSValue js_wifi_secure(JSContext *ctx, JSValue *, int argc,
                       JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(ctx, WifiScanSecure(static_cast<uint32_t>(index)));
}

}  // extern "C"

}  // namespace pulp
