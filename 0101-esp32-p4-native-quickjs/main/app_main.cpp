// 0101 — ESP32-P4 native QuickJS smoke firmware.
// Creates a native QuickJS runtime/context and evaluates print(1+2) at boot.
#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

extern "C" {
#include "quickjs.h"
}

namespace {
constexpr const char *kTag = "0101_qjs";
constexpr size_t kQuickJsMemoryLimit = 2 * 1024 * 1024;
constexpr size_t kQuickJsStackLimit = 64 * 1024;

JSValue js_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    for (int i = 0; i < argc; ++i) {
        const char *s = JS_ToCString(ctx, argv[i]);
        if (!s) {
            return JS_EXCEPTION;
        }
        if (i > 0) {
            std::fputc(' ', stdout);
        }
        std::fputs(s, stdout);
        JS_FreeCString(ctx, s);
    }
    std::fputc('\n', stdout);
    std::fflush(stdout);
    return JS_UNDEFINED;
}

JSValue js_millis(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_NewInt64(ctx, esp_timer_get_time() / 1000);
}

bool install_globals(JSContext *ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) {
        return false;
    }

    JSValue print_fn = JS_NewCFunction(ctx, js_print, "print", 1);
    JSValue millis_fn = JS_NewCFunction(ctx, js_millis, "millis", 0);
    const int print_rc = JS_SetPropertyStr(ctx, global, "print", print_fn);
    const int millis_rc = JS_SetPropertyStr(ctx, global, "millis", millis_fn);
    JS_FreeValue(ctx, global);
    return print_rc >= 0 && millis_rc >= 0;
}

void log_exception(JSContext *ctx)
{
    JSValue exc = JS_GetException(ctx);
    const char *s = JS_ToCString(ctx, exc);
    ESP_LOGE(kTag, "exception: %s", s ? s : "<exception stringify failed>");
    if (s) {
        JS_FreeCString(ctx, s);
    }
    JS_FreeValue(ctx, exc);
}

void run_native_smoke()
{
    ESP_LOGI(kTag,
             "heap before: internal=%u 8bit=%u psram=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    const int64_t t0 = esp_timer_get_time();
    JSRuntime *rt = JS_NewRuntime();
    if (!rt) {
        ESP_LOGE(kTag, "JS_NewRuntime failed");
        return;
    }
    JS_SetMemoryLimit(rt, kQuickJsMemoryLimit);
    JS_SetMaxStackSize(rt, kQuickJsStackLimit);
    JS_SetCanBlock(rt, 0);

    JSContext *ctx = JS_NewContext(rt);
    if (!ctx) {
        ESP_LOGE(kTag, "JS_NewContext failed");
        JS_FreeRuntime(rt);
        return;
    }
    if (!install_globals(ctx)) {
        ESP_LOGE(kTag, "install_globals failed");
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return;
    }

    const int64_t t_ready = esp_timer_get_time();
    ESP_LOGI(kTag, "native QuickJS ready in %lld ms", (long long)((t_ready - t0) / 1000));

    const char *src = "print(1+2)";
    ESP_LOGI(kTag, "native eval: %s", src);
    JSValue result = JS_Eval(ctx, src, std::strlen(src), "<boot-smoke>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        log_exception(ctx);
        JS_FreeValue(ctx, result);
    } else {
        JS_FreeValue(ctx, result);
        ESP_LOGI(kTag, "native eval ok");
    }

    const char *bench = "(function(){let t=millis(); let s=0; for(let i=0;i<10000;i++) s+=i; print('sum10k='+String(millis()-t)+',s='+String(s));})()";
    ESP_LOGI(kTag, "native bench: sum10k");
    result = JS_Eval(ctx, bench, std::strlen(bench), "<boot-bench>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        log_exception(ctx);
    }
    JS_FreeValue(ctx, result);

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    ESP_LOGI(kTag,
             "heap after: internal=%u 8bit=%u psram=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0101 ESP32-P4 native QuickJS smoke (ticket ESP32-P4-NATIVE-QUICKJS)");
    run_native_smoke();
}
