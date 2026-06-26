#include "host_http_ops.h"
#include "http_namespace_core.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

extern "C" {
#include "quickjs.h"
}

namespace {

std::string read_file(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

JSValue js_print(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    for (int i = 0; i < argc; ++i) {
        const char *s = JS_ToCString(ctx, argv[i]);
        if (!s) return JS_EXCEPTION;
        if (i) std::printf(" ");
        std::printf("%s", s);
        JS_FreeCString(ctx, s);
    }
    std::printf("\n");
    return JS_UNDEFINED;
}

JSValue js_millis(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    static const auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    return JS_NewInt64(ctx, (int64_t)ms);
}

JSValue js_gc(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    JS_RunGC(JS_GetRuntime(ctx));
    return JS_UNDEFINED;
}

bool define_function(JSContext *ctx, JSValueConst global, const char *name, JSCFunction *fn, int argc)
{
    JSValue f = JS_NewCFunction(ctx, fn, name, argc);
    if (JS_IsException(f)) return false;
    return JS_DefinePropertyValueStr(ctx, global, name, f, JS_PROP_ENUMERABLE) >= 0;
}

bool install_base_globals(JSContext *ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    bool ok = define_function(ctx, global, "print", js_print, 1) &&
              define_function(ctx, global, "millis", js_millis, 0) &&
              define_function(ctx, global, "gc", js_gc, 0);
    JS_FreeValue(ctx, global);
    return ok;
}

std::string exception_to_string(JSContext *ctx)
{
    JSValue ex = JS_GetException(ctx);
    const char *s = JS_ToCString(ctx, ex);
    std::string out = s ? s : "<exception>";
    if (s) JS_FreeCString(ctx, s);
    JS_FreeValue(ctx, ex);
    return out;
}

bool eval_source(JSContext *ctx, const std::string &code, const std::string &filename)
{
    JSValue val = JS_Eval(ctx, code.data(), code.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
        std::fprintf(stderr, "eval error: %s\n", exception_to_string(ctx).c_str());
        return false;
    }
    JS_FreeValue(ctx, val);
    return true;
}

bool drain_jobs(JSRuntime *rt, JSContext *ctx)
{
    JSContext *job_ctx = nullptr;
    while (true) {
        int rc = JS_ExecutePendingJob(rt, &job_ctx);
        if (rc == 0) return true;
        if (rc < 0) {
            std::fprintf(stderr, "promise job error: %s\n", exception_to_string(job_ctx ? job_ctx : ctx).c_str());
            return false;
        }
    }
}

void print_usage(const char *argv0)
{
    std::fprintf(stderr, "usage: %s <script.js> [--dispatch /path] [--fake-async-fetch]\n", argv0);
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 2;
    }
    std::string script = argv[1];
    std::string dispatch_path;
    bool fake_async_fetch = false;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--dispatch") == 0 && i + 1 < argc) {
            dispatch_path = argv[++i];
        } else if (std::strcmp(argv[i], "--fake-async-fetch") == 0) {
            fake_async_fetch = true;
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }

    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    if (!rt || !ctx) {
        std::fprintf(stderr, "failed to create QuickJS runtime\n");
        return 1;
    }

    qjs_http_host::HostState host_state;
    host_state.fake_async_fetch = fake_async_fetch;
    auto *http = new qjs_http::Runtime(ctx, qjs_http_host::make_host_ops(&host_state));
    if (!http) {
        std::fprintf(stderr, "failed to create HTTP runtime\n");
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }

    bool ok = install_base_globals(ctx) && http->install_global() == 0;
    if (ok) {
        std::string code = read_file(script);
        if (code.empty()) {
            std::fprintf(stderr, "script is empty or unreadable: %s\n", script.c_str());
            ok = false;
        } else {
            ok = eval_source(ctx, code, script) && drain_jobs(rt, ctx);
        }
    }

    if (ok && !dispatch_path.empty()) {
        qjs_http::HttpResponse response;
        std::string error;
        if (!http->dispatch_get(dispatch_path.c_str(), &response, &error)) {
            std::fprintf(stderr, "dispatch error: %s\n", error.c_str());
            ok = false;
        } else {
            std::printf("DISPATCH status=%d content-type=%s\n", response.status, response.content_type.c_str());
            std::printf("%s\n", response.body.c_str());
            ok = drain_jobs(rt, ctx);
        }
    }

    delete http;
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return ok ? 0 : 1;
}
