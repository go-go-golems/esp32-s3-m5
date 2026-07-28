// mdns singleton bindings (ESP-54 P2). Read-only accessors; mDNS has no
// async verbs (announce/stop are side effects of serve.start/stop/wifi.off).
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

}  // extern "C"

}  // namespace pulp
