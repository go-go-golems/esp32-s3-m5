// mdns singleton bindings (ESP-54 P2, ESP-58 browse). announce/stop stay
// side effects of serve.start/stop/wifi.off; browse is the singleton's
// one async verb: browse(fn) -> fn(kDoneMdnsBrowse, count, err), results
// read via count()/name(i)/indexUrl(i) (the wifi.scan idiom).
#include "app_js_internal.h"
#include "net_mdns.h"

namespace pulp {

using namespace jsi;

extern "C" {

JSValue js_mdns_status(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(MdnsStatus()));
}

JSValue js_mdns_host(JSContext *ctx, JSValue *, int, JSValue *) {
    char host[24];
    MdnsHost(host, sizeof(host));
    return JS_NewString(ctx, host);
}

JSValue js_mdns_url(JSContext *ctx, JSValue *, int, JSValue *) {
    char url[40];
    MdnsUrl(url, sizeof(url));
    return JS_NewString(ctx, url);
}

JSValue js_mdns_browse(JSContext *ctx, JSValue *, int argc,
                       JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "browse(fn)");
    }
    JSValue err;
    if (!RegisterModuleCb(ctx, ModuleId::Mdns, argv[0], &err)) {
        return err;
    }
    const StatusCode status = MdnsBrowse();
    if (status != StatusCode::Ok) {
        CancelModuleCb(ModuleId::Mdns);
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

JSValue js_mdns_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(MdnsResultCount()));
}

JSValue js_mdns_name(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int i = 0;
    if (argc < 1 || JS_ToInt32(ctx, &i, argv[0])) {
        return JS_EXCEPTION;
    }
    return JS_NewString(ctx,
                        MdnsResultName(static_cast<uint32_t>(i < 0 ? 0
                                                                   : i)));
}

JSValue js_mdns_index_url(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    int i = 0;
    if (argc < 1 || JS_ToInt32(ctx, &i, argv[0])) {
        return JS_EXCEPTION;
    }
    return JS_NewString(
        ctx, MdnsResultIndexUrl(static_cast<uint32_t>(i < 0 ? 0 : i)));
}

}  // extern "C"

}  // namespace pulp
