// http_namespace_core.cpp — portable QuickJS HTTP/fetch bindings shared by host and firmware.
#include "http_namespace_core.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace qjs_http {
namespace {
constexpr size_t kMaxRoutes = 16;
constexpr size_t kMaxStaticMounts = 4;
constexpr size_t kMaxPathBytes = 127;
constexpr size_t kMaxHeaderCount = 16;
constexpr size_t kMaxHeaderBytes = 512;
constexpr size_t kMaxFetchBodyBytes = 4096;
constexpr size_t kMaxFetchResponseBytes = 16 * 1024;
constexpr uint32_t kDefaultFetchTimeoutMs = 1000;
constexpr uint32_t kMaxFetchTimeoutMs = 5000;
constexpr int kMaxRoutePromiseJobs = 64;

Runtime *g_active_runtime = nullptr;

bool is_valid_path(const char *path)
{
    if (!path || path[0] != '/') return false;
    const size_t n = std::strlen(path);
    if (n == 0 || n > kMaxPathBytes) return false;
    if (std::strstr(path, "//") || std::strstr(path, "/../") || std::strstr(path, "/./") ||
        std::strchr(path, '\\') || std::strchr(path, ':') || std::strchr(path, '?') || std::strchr(path, '#')) {
        return false;
    }
    if (n >= 2 && std::strcmp(path + n - 2, "/.") == 0) return false;
    if (n >= 3 && std::strcmp(path + n - 3, "/..") == 0) return false;
    return true;
}

std::string normalized_path(const char *path)
{
    std::string out = path ? path : "";
    while (out.size() > 1 && out.back() == '/') out.pop_back();
    return out;
}

JSValue throw_error(JSContext *ctx, const char *operation, const char *message)
{
    return JS_ThrowInternalError(ctx, "%s: %s", operation, message ? message : "failed");
}

JSValue throw_errno(JSContext *ctx, const char *operation, int err)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "native error %d", err);
    return throw_error(ctx, operation, buf);
}

bool js_to_string(JSContext *ctx, JSValueConst v, std::string *out)
{
    const char *s = JS_ToCString(ctx, v);
    if (!s) return false;
    out->assign(s);
    JS_FreeCString(ctx, s);
    return true;
}

bool js_to_string_len(JSContext *ctx, JSValueConst v, std::string *out)
{
    size_t len = 0;
    const char *s = JS_ToCStringLen(ctx, &len, v);
    if (!s) return false;
    out->assign(s, len);
    JS_FreeCString(ctx, s);
    return true;
}

bool get_prop_string(JSContext *ctx, JSValueConst obj, const char *name, std::string *out, bool *present = nullptr)
{
    if (present) *present = false;
    JSValue prop = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(prop)) return false;
    if (JS_IsUndefined(prop)) {
        JS_FreeValue(ctx, prop);
        return true;
    }
    if (present) *present = true;
    const bool ok = js_to_string(ctx, prop, out);
    JS_FreeValue(ctx, prop);
    return ok;
}

bool get_prop_u32(JSContext *ctx, JSValueConst obj, const char *name, uint32_t *out, bool *present = nullptr)
{
    if (present) *present = false;
    JSValue prop = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(prop)) return false;
    if (JS_IsUndefined(prop)) {
        JS_FreeValue(ctx, prop);
        return true;
    }
    if (present) *present = true;
    uint32_t v = 0;
    const int rc = JS_ToUint32(ctx, &v, prop);
    JS_FreeValue(ctx, prop);
    if (rc < 0) return false;
    *out = v;
    return true;
}

JSValue promise_call(JSContext *ctx, const char *name, JSValueConst value)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue promise = JS_GetPropertyStr(ctx, global, "Promise");
    JSValue fn = JS_GetPropertyStr(ctx, promise, name);
    JSValue argv[1] = {JS_DupValue(ctx, value)};
    JSValue result = JS_Call(ctx, fn, promise, 1, argv);
    JS_FreeValue(ctx, argv[0]);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, promise);
    JS_FreeValue(ctx, global);
    return result;
}

JSValue promise_resolve(JSContext *ctx, JSValueConst value) { return promise_call(ctx, "resolve", value); }
JSValue promise_reject(JSContext *ctx, JSValueConst value) { return promise_call(ctx, "reject", value); }

std::string exception_to_string(JSContext *ctx)
{
    JSValue ex = JS_GetException(ctx);
    std::string out;
    if (!js_to_string(ctx, ex, &out)) out = "handler exception";
    JS_FreeValue(ctx, ex);
    return out;
}

std::string value_to_string(JSContext *ctx, JSValueConst value, const char *fallback)
{
    std::string out;
    if (!js_to_string(ctx, value, &out)) out = fallback ? fallback : "<value>";
    return out;
}

void make_error_response(HttpResponse *out, int status, const std::string &message)
{
    if (!out) return;
    out->status = status;
    out->content_type = "text/plain; charset=utf-8";
    out->body = message;
    out->body_set = true;
}

bool is_promise(JSContext *ctx, JSValueConst value)
{
    return JS_PromiseState(ctx, value) >= 0;
}

bool drain_route_promise(JSContext *ctx, JSValueConst promise, std::string *error)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    int jobs = 0;
    while (JS_PromiseState(ctx, promise) == JS_PROMISE_PENDING) {
        JSContext *job_ctx = nullptr;
        int rc = JS_ExecutePendingJob(rt, &job_ctx);
        if (rc < 0) {
            if (error) *error = exception_to_string(job_ctx ? job_ctx : ctx);
            return false;
        }
        if (rc == 0) {
            if (error) *error = "route promise did not settle";
            return false;
        }
        jobs++;
        if (jobs >= kMaxRoutePromiseJobs) {
            if (error) *error = "too many route promise jobs";
            return false;
        }
    }
    return true;
}

bool define_value(JSContext *ctx, JSValueConst obj, const char *name, JSValue value, int flags = JS_PROP_ENUMERABLE)
{
    return JS_DefinePropertyValueStr(ctx, obj, name, value, flags) >= 0;
}

JSValue make_ok_result(JSContext *ctx, const HostStatus &st)
{
    JSValue obj = JS_NewObject(ctx);
    define_value(ctx, obj, "ok", JS_NewBool(ctx, true));
    define_value(ctx, obj, "running", JS_NewBool(ctx, st.running));
    define_value(ctx, obj, "port", JS_NewInt32(ctx, st.port));
    return obj;
}

JSValue headers_to_object(JSContext *ctx, const std::vector<Header> &headers)
{
    JSValue obj = JS_NewObject(ctx);
    for (const Header &h : headers) {
        if (!h.name.empty()) {
            define_value(ctx, obj, h.name.c_str(), JS_NewString(ctx, h.value.c_str()));
        }
    }
    return obj;
}

std::string json_stringify(JSContext *ctx, JSValueConst value, std::string *error)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
    JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");
    JSValue argv[1] = {JS_DupValue(ctx, value)};
    JSValue out = JS_Call(ctx, stringify, json, 1, argv);
    JS_FreeValue(ctx, argv[0]);
    JS_FreeValue(ctx, stringify);
    JS_FreeValue(ctx, json);
    JS_FreeValue(ctx, global);
    if (JS_IsException(out)) {
        if (error) *error = "JSON.stringify failed";
        return {};
    }
    std::string s;
    if (!js_to_string(ctx, out, &s) && error) *error = "JSON.stringify did not produce a string";
    JS_FreeValue(ctx, out);
    return s;
}

bool convert_handler_result(JSContext *ctx, JSValueConst value, HttpResponse *out, std::string *error)
{
    if (!out) return false;
    *out = HttpResponse{};

    if (JS_IsString(value) || JS_IsNumber(value) || JS_IsBool(value)) {
        if (!js_to_string(ctx, value, &out->body)) {
            if (error) *error = "response value conversion failed";
            return false;
        }
        out->body_set = true;
        return true;
    }

    if (!JS_IsObject(value)) {
        out->status = 204;
        out->content_type = "text/plain; charset=utf-8";
        out->body.clear();
        out->body_set = true;
        return true;
    }

    uint32_t status = 200;
    bool present = false;
    if (!get_prop_u32(ctx, value, "status", &status, &present)) {
        if (error) *error = "response.status must be a number";
        return false;
    }
    if (present && (status < 100 || status > 599)) {
        if (error) *error = "response.status out of range";
        return false;
    }
    out->status = (int)status;

    std::string content_type;
    if (!get_prop_string(ctx, value, "contentType", &content_type, &present)) {
        if (error) *error = "response.contentType must be a string";
        return false;
    }
    if (present && !content_type.empty()) out->content_type = content_type;

    JSValue json = JS_GetPropertyStr(ctx, value, "json");
    if (JS_IsException(json)) return false;
    if (!JS_IsUndefined(json)) {
        std::string err;
        out->body = json_stringify(ctx, json, &err);
        JS_FreeValue(ctx, json);
        if (!err.empty()) {
            if (error) *error = err;
            return false;
        }
        out->content_type = "application/json; charset=utf-8";
        out->body_set = true;
        return true;
    }
    JS_FreeValue(ctx, json);

    JSValue text = JS_GetPropertyStr(ctx, value, "text");
    if (JS_IsException(text)) return false;
    if (!JS_IsUndefined(text)) {
        const bool ok = js_to_string_len(ctx, text, &out->body);
        JS_FreeValue(ctx, text);
        if (!ok) {
            if (error) *error = "response.text conversion failed";
            return false;
        }
        out->body_set = true;
        return true;
    }
    JS_FreeValue(ctx, text);

    out->status = 204;
    out->body.clear();
    out->body_set = true;
    return true;
}

bool convert_or_error_response(JSContext *ctx, JSValueConst value, HttpResponse *out, std::string *error)
{
    std::string conversion_error;
    if (convert_handler_result(ctx, value, out, &conversion_error)) return true;
    const std::string msg = conversion_error.empty() ? "invalid route response" : conversion_error;
    if (error) *error = msg;
    make_error_response(out, 500, "route response error: " + msg);
    return true;
}

JSValue js_resp_text(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    JSValue body = JS_GetPropertyStr(ctx, this_val, "__body");
    if (JS_IsException(body)) return body;
    JSValue p = promise_resolve(ctx, body);
    JS_FreeValue(ctx, body);
    return p;
}

JSValue js_resp_json(JSContext *ctx, JSValueConst this_val, int, JSValueConst *)
{
    JSValue body = JS_GetPropertyStr(ctx, this_val, "__body");
    if (JS_IsException(body)) return body;
    size_t len = 0;
    const char *s = JS_ToCStringLen(ctx, &len, body);
    JS_FreeValue(ctx, body);
    if (!s) return JS_EXCEPTION;
    JSValue parsed = JS_ParseJSON(ctx, s, len, "<fetch-json>");
    JS_FreeCString(ctx, s);
    if (JS_IsException(parsed)) {
        JSValue ex = JS_GetException(ctx);
        JSValue p = promise_reject(ctx, ex);
        JS_FreeValue(ctx, ex);
        return p;
    }
    JSValue p = promise_resolve(ctx, parsed);
    JS_FreeValue(ctx, parsed);
    return p;
}

JSValue make_fetch_response(JSContext *ctx, const FetchRequest &req, const FetchResult &res)
{
    JSValue obj = JS_NewObject(ctx);
    define_value(ctx, obj, "ok", JS_NewBool(ctx, res.status >= 200 && res.status <= 299));
    define_value(ctx, obj, "status", JS_NewInt32(ctx, res.status));
    define_value(ctx, obj, "statusText", JS_NewString(ctx, res.status_text.c_str()));
    define_value(ctx, obj, "url", JS_NewString(ctx, res.final_url.empty() ? req.url.c_str() : res.final_url.c_str()));
    define_value(ctx, obj, "headers", headers_to_object(ctx, res.headers));
    define_value(ctx, obj, "__body", JS_NewStringLen(ctx, res.body.data(), res.body.size()), 0);
    define_value(ctx, obj, "text", JS_NewCFunction(ctx, js_resp_text, "text", 0));
    define_value(ctx, obj, "json", JS_NewCFunction(ctx, js_resp_json, "json", 0));
    JS_PreventExtensions(ctx, obj);
    return obj;
}

bool parse_headers_object(JSContext *ctx, JSValueConst headers_obj, std::vector<Header> *out, std::string *error)
{
    if (JS_IsUndefined(headers_obj)) return true;
    if (!JS_IsObject(headers_obj)) {
        if (error) *error = "fetch headers must be an object";
        return false;
    }
    JSPropertyEnum *props = nullptr;
    uint32_t len = 0;
    if (JS_GetOwnPropertyNames(ctx, &props, &len, headers_obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
        return false;
    }
    bool ok = true;
    for (uint32_t i = 0; i < len && ok; ++i) {
        if (out->size() >= kMaxHeaderCount) {
            if (error) *error = "too many fetch headers";
            ok = false;
            break;
        }
        const char *name = JS_AtomToCString(ctx, props[i].atom);
        if (!name) {
            ok = false;
            break;
        }
        JSValue v = JS_GetProperty(ctx, headers_obj, props[i].atom);
        std::string value;
        if (JS_IsException(v) || !js_to_string(ctx, v, &value)) {
            if (error) *error = "fetch header values must be strings";
            ok = false;
        } else if (std::strlen(name) + value.size() > kMaxHeaderBytes) {
            if (error) *error = "fetch header too large";
            ok = false;
        } else {
            out->push_back(Header{name, value});
        }
        JS_FreeValue(ctx, v);
        JS_FreeCString(ctx, name);
    }
    for (uint32_t i = 0; i < len; ++i) JS_FreeAtom(ctx, props[i].atom);
    js_free(ctx, props);
    return ok;
}

bool parse_fetch_request(JSContext *ctx, int argc, JSValueConst *argv, FetchRequest *out, std::string *error)
{
    if (argc < 1) {
        if (error) *error = "fetch(url, options) requires a URL";
        return false;
    }
    if (!js_to_string(ctx, argv[0], &out->url)) {
        if (error) *error = "fetch URL must be a string";
        return false;
    }
    if (out->url.rfind("http://", 0) != 0) {
        if (error) *error = "only http:// URLs are supported in this milestone";
        return false;
    }

    out->method = "GET";
    out->timeout_ms = kDefaultFetchTimeoutMs;
    out->max_response_bytes = kMaxFetchResponseBytes;

    if (argc >= 2 && !JS_IsUndefined(argv[1])) {
        if (!JS_IsObject(argv[1])) {
            if (error) *error = "fetch options must be an object";
            return false;
        }
        bool present = false;
        if (!get_prop_string(ctx, argv[1], "method", &out->method, &present)) {
            if (error) *error = "fetch method must be a string";
            return false;
        }
        std::transform(out->method.begin(), out->method.end(), out->method.begin(), [](unsigned char c) { return (char)std::toupper(c); });
        if (out->method != "GET" && out->method != "POST") {
            if (error) *error = "fetch supports GET and POST only";
            return false;
        }
        std::string body;
        if (!get_prop_string(ctx, argv[1], "body", &body, &present)) {
            if (error) *error = "fetch body must be a string";
            return false;
        }
        if (present) out->body = body;
        if (out->body.size() > kMaxFetchBodyBytes) {
            if (error) *error = "fetch body too large";
            return false;
        }
        uint32_t timeout = out->timeout_ms;
        if (!get_prop_u32(ctx, argv[1], "timeoutMs", &timeout, &present)) {
            if (error) *error = "fetch timeoutMs must be a number";
            return false;
        }
        if (present) {
            if (timeout == 0 || timeout > kMaxFetchTimeoutMs) {
                if (error) *error = "fetch timeoutMs out of range";
                return false;
            }
            out->timeout_ms = timeout;
        }
        JSValue headers = JS_GetPropertyStr(ctx, argv[1], "headers");
        if (JS_IsException(headers)) return false;
        bool ok = parse_headers_object(ctx, headers, &out->headers, error);
        JS_FreeValue(ctx, headers);
        if (!ok) return false;
    }
    return true;
}

JSValue js_http_status(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "http.status", "runtime unavailable");
    HostStatus st = rt->status();
    JSValue obj = JS_NewObject(ctx);
    define_value(ctx, obj, "running", JS_NewBool(ctx, st.running));
    define_value(ctx, obj, "port", JS_NewInt32(ctx, st.port));
    JSValue mounts = JS_NewArray(ctx);
    auto ms = rt->static_mounts();
    for (uint32_t i = 0; i < ms.size(); ++i) {
        JSValue m = JS_NewObject(ctx);
        define_value(ctx, m, "prefix", JS_NewString(ctx, ms[i].url_prefix.c_str()));
        define_value(ctx, m, "root", JS_NewString(ctx, ms[i].virtual_root.c_str()));
        JS_SetPropertyUint32(ctx, mounts, i, m);
    }
    define_value(ctx, obj, "staticMounts", mounts);
    JSValue routes = JS_NewArray(ctx);
    auto rs = rt->dynamic_routes();
    for (uint32_t i = 0; i < rs.size(); ++i) {
        JSValue r = JS_NewObject(ctx);
        define_value(ctx, r, "method", JS_NewString(ctx, rs[i].method.c_str()));
        define_value(ctx, r, "path", JS_NewString(ctx, rs[i].path.c_str()));
        JS_SetPropertyUint32(ctx, routes, i, r);
    }
    define_value(ctx, obj, "routes", routes);
    JSValue limits = JS_NewObject(ctx);
    define_value(ctx, limits, "maxRoutes", JS_NewInt32(ctx, (int32_t)kMaxRoutes));
    define_value(ctx, limits, "maxStaticMounts", JS_NewInt32(ctx, (int32_t)kMaxStaticMounts));
    define_value(ctx, limits, "maxFetchBodyBytes", JS_NewInt32(ctx, (int32_t)kMaxFetchBodyBytes));
    define_value(ctx, limits, "maxFetchResponseBytes", JS_NewInt32(ctx, (int32_t)kMaxFetchResponseBytes));
    define_value(ctx, obj, "limits", limits);
    return obj;
}

JSValue js_http_start(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "http.start", "runtime unavailable");
    uint32_t port = 80;
    if (argc >= 1 && !JS_IsUndefined(argv[0]) && JS_ToUint32(ctx, &port, argv[0]) < 0) return JS_EXCEPTION;
    if (port == 0 || port >= 65535) return JS_ThrowRangeError(ctx, "http.start: invalid port");
    int rc = rt->start((uint16_t)port);
    if (rc != 0) return throw_errno(ctx, "http.start", rc);
    return make_ok_result(ctx, rt->status());
}

JSValue js_http_stop(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "http.stop", "runtime unavailable");
    int rc = rt->stop();
    if (rc != 0) return throw_errno(ctx, "http.stop", rc);
    return make_ok_result(ctx, rt->status());
}

JSValue js_http_static(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "http.static", "runtime unavailable");
    if (argc < 2) return JS_ThrowTypeError(ctx, "http.static(prefix, virtualRoot) requires two arguments");
    std::string prefix, root;
    if (!js_to_string(ctx, argv[0], &prefix) || !js_to_string(ctx, argv[1], &root)) return JS_EXCEPTION;
    int rc = rt->add_static_mount(prefix.c_str(), root.c_str());
    if (rc != 0) return throw_errno(ctx, "http.static", rc);
    JSValue obj = JS_NewObject(ctx);
    define_value(ctx, obj, "ok", JS_NewBool(ctx, true));
    define_value(ctx, obj, "prefix", JS_NewString(ctx, normalized_path(prefix.c_str()).c_str()));
    define_value(ctx, obj, "root", JS_NewString(ctx, normalized_path(root.c_str()).c_str()));
    return obj;
}

JSValue js_http_clear_static(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "http.clearStatic", "runtime unavailable");
    int rc = rt->clear_static_mounts();
    if (rc != 0) return throw_errno(ctx, "http.clearStatic", rc);
    JSValue obj = JS_NewObject(ctx);
    define_value(ctx, obj, "ok", JS_NewBool(ctx, true));
    return obj;
}

JSValue js_http_get(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "http.get", "runtime unavailable");
    if (argc < 2) return JS_ThrowTypeError(ctx, "http.get(path, handler) requires two arguments");
    std::string path;
    if (!js_to_string(ctx, argv[0], &path)) return JS_EXCEPTION;
    if (!JS_IsFunction(ctx, argv[1])) return JS_ThrowTypeError(ctx, "http.get handler must be a function");
    int rc = rt->add_get_route(path.c_str(), argv[1]);
    if (rc != 0) return throw_errno(ctx, "http.get", rc);
    JSValue obj = JS_NewObject(ctx);
    define_value(ctx, obj, "ok", JS_NewBool(ctx, true));
    define_value(ctx, obj, "method", JS_NewString(ctx, "GET"));
    define_value(ctx, obj, "path", JS_NewString(ctx, normalized_path(path.c_str()).c_str()));
    return obj;
}

JSValue js_fetch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    Runtime *rt = active_runtime();
    if (!rt) return throw_error(ctx, "fetch", "runtime unavailable");
    if (!rt->ops().fetch) return throw_error(ctx, "fetch", "host fetch adapter unavailable");
    FetchRequest req;
    std::string parse_error;
    if (!parse_fetch_request(ctx, argc, argv, &req, &parse_error)) {
        JSValue err = JS_NewError(ctx);
        define_value(ctx, err, "message", JS_NewString(ctx, parse_error.c_str()), 0);
        JSValue p = promise_reject(ctx, err);
        JS_FreeValue(ctx, err);
        return p;
    }
    FetchResult result;
    std::string fetch_error;
    int rc = rt->ops().fetch(rt->ops().user, &req, &result, &fetch_error);
    if (rc != 0) {
        JSValue err = JS_NewError(ctx);
        define_value(ctx, err, "message", JS_NewString(ctx, fetch_error.empty() ? "fetch failed" : fetch_error.c_str()), 0);
        JSValue p = promise_reject(ctx, err);
        JS_FreeValue(ctx, err);
        return p;
    }
    JSValue response = make_fetch_response(ctx, req, result);
    JSValue p = promise_resolve(ctx, response);
    JS_FreeValue(ctx, response);
    return p;
}

bool set_function(JSContext *ctx, JSValueConst obj, const char *name, JSCFunction *fn, int argc)
{
    JSValue f = JS_NewCFunction(ctx, fn, name, argc);
    if (JS_IsException(f)) return false;
    return JS_DefinePropertyValueStr(ctx, obj, name, f, JS_PROP_ENUMERABLE) >= 0;
}

}  // namespace

Runtime *active_runtime() { return g_active_runtime; }
void set_active_runtime(Runtime *runtime) { g_active_runtime = runtime; }

Runtime::Runtime(JSContext *ctx, const HostOps &ops) : ctx_(ctx), ops_(ops) {}

Runtime::~Runtime()
{
    clear_routes();
    if (g_active_runtime == this) g_active_runtime = nullptr;
}

int Runtime::install_global()
{
    if (!ctx_) return -1;
    set_active_runtime(this);
    JSValue http = JS_NewObject(ctx_);
    if (JS_IsException(http)) return -1;
    bool ok = set_function(ctx_, http, "status", js_http_status, 0) &&
              set_function(ctx_, http, "start", js_http_start, 1) &&
              set_function(ctx_, http, "stop", js_http_stop, 0) &&
              set_function(ctx_, http, "static", js_http_static, 2) &&
              set_function(ctx_, http, "clearStatic", js_http_clear_static, 0) &&
              set_function(ctx_, http, "get", js_http_get, 2);
    if (ok && JS_PreventExtensions(ctx_, http) < 0) ok = false;

    JSValue global = JS_GetGlobalObject(ctx_);
    if (JS_IsException(global)) {
        JS_FreeValue(ctx_, http);
        return -1;
    }
    if (ok) {
        int rc = JS_DefinePropertyValueStr(ctx_, global, "http", http, JS_PROP_ENUMERABLE);
        http = JS_UNDEFINED;
        ok = rc >= 0;
    }
    if (ok) {
        JSValue f = JS_NewCFunction(ctx_, js_fetch, "fetch", 2);
        if (JS_IsException(f)) {
            ok = false;
        } else {
            ok = JS_DefinePropertyValueStr(ctx_, global, "fetch", f, JS_PROP_ENUMERABLE) >= 0;
        }
    }
    JS_FreeValue(ctx_, global);
    JS_FreeValue(ctx_, http);
    return ok ? 0 : -1;
}

void Runtime::clear_routes()
{
    if (ctx_) {
        for (Route &r : routes_) JS_FreeValue(ctx_, r.handler);
    }
    routes_.clear();
}

int Runtime::start(uint16_t port)
{
    int rc = ops_.start ? ops_.start(ops_.user, port) : 0;
    if (rc == 0) {
        running_ = true;
        port_ = port;
    }
    return rc;
}

int Runtime::stop()
{
    int rc = ops_.stop ? ops_.stop(ops_.user) : 0;
    if (rc == 0) {
        running_ = false;
        port_ = 0;
    }
    return rc;
}

int Runtime::add_static_mount(const char *url_prefix, const char *virtual_root)
{
    if (!is_valid_path(url_prefix) || !is_valid_path(virtual_root)) return -2;
    std::string prefix = normalized_path(url_prefix);
    std::string root = normalized_path(virtual_root);
    int rc = ops_.add_static_mount ? ops_.add_static_mount(ops_.user, prefix.c_str(), root.c_str()) : 0;
    if (rc != 0) return rc;
    for (auto &m : static_mounts_) {
        if (m.url_prefix == prefix) {
            m.virtual_root = root;
            return 0;
        }
    }
    if (static_mounts_.size() >= kMaxStaticMounts) return -3;
    static_mounts_.push_back(StaticMountInfo{prefix, root});
    return 0;
}

int Runtime::clear_static_mounts()
{
    int rc = ops_.clear_static_mounts ? ops_.clear_static_mounts(ops_.user) : 0;
    if (rc == 0) static_mounts_.clear();
    return rc;
}

HostStatus Runtime::status() const
{
    HostStatus st{running_, port_};
    if (ops_.status) {
        HostStatus native = {};
        if (ops_.status(ops_.user, &native) == 0) st = native;
    }
    return st;
}

Runtime::Route *Runtime::find_route(const char *method, const char *path)
{
    for (Route &r : routes_) {
        if (r.method == method && r.path == path) return &r;
    }
    return nullptr;
}

int Runtime::add_get_route(const char *path, JSValueConst handler)
{
    if (!is_valid_path(path)) return -2;
    std::string p = normalized_path(path);
    Route *existing = find_route("GET", p.c_str());
    if (existing) {
        JS_FreeValue(ctx_, existing->handler);
        existing->handler = JS_DupValue(ctx_, handler);
        return 0;
    }
    if (routes_.size() >= kMaxRoutes) return -3;
    Route r;
    r.method = "GET";
    r.path = p;
    r.handler = JS_DupValue(ctx_, handler);
    routes_.push_back(r);
    return 0;
}

bool Runtime::dispatch_get(const char *path, HttpResponse *out, std::string *error)
{
    if (!path || !out) return false;
    std::string p = normalized_path(path);
    Route *r = find_route("GET", p.c_str());
    if (!r) {
        if (error) *error = "route not found";
        return false;
    }
    JSValue req = JS_NewObject(ctx_);
    define_value(ctx_, req, "method", JS_NewString(ctx_, "GET"));
    define_value(ctx_, req, "path", JS_NewString(ctx_, p.c_str()));
    define_value(ctx_, req, "query", JS_NewString(ctx_, ""));
    define_value(ctx_, req, "headers", JS_NewObject(ctx_));
    JSValue argv[1] = {req};
    JSValue result = JS_Call(ctx_, r->handler, JS_UNDEFINED, 1, argv);
    JS_FreeValue(ctx_, req);
    if (JS_IsException(result)) {
        std::string msg = exception_to_string(ctx_);
        if (error) *error = msg;
        make_error_response(out, 500, "route handler exception: " + msg);
        return true;
    }

    bool ok = true;
    if (is_promise(ctx_, result)) {
        std::string promise_error;
        if (!drain_route_promise(ctx_, result, &promise_error)) {
            if (error) *error = promise_error;
            const int status = (promise_error == "route promise did not settle" || promise_error == "too many route promise jobs") ? 504 : 500;
            make_error_response(out, status, "route promise error: " + promise_error);
            JS_FreeValue(ctx_, result);
            return true;
        }

        const JSPromiseStateEnum state = JS_PromiseState(ctx_, result);
        JSValue settled = JS_PromiseResult(ctx_, result);
        if (state == JS_PROMISE_REJECTED) {
            std::string msg = value_to_string(ctx_, settled, "route promise rejected");
            if (error) *error = msg;
            make_error_response(out, 500, "route promise rejected: " + msg);
            JS_FreeValue(ctx_, settled);
            JS_FreeValue(ctx_, result);
            return true;
        }
        ok = convert_or_error_response(ctx_, settled, out, error);
        JS_FreeValue(ctx_, settled);
    } else {
        ok = convert_or_error_response(ctx_, result, out, error);
    }
    JS_FreeValue(ctx_, result);
    return ok;
}

std::vector<DynamicRouteInfo> Runtime::dynamic_routes() const
{
    std::vector<DynamicRouteInfo> out;
    for (const Route &r : routes_) out.push_back(DynamicRouteInfo{r.method, r.path});
    return out;
}

}  // namespace qjs_http
