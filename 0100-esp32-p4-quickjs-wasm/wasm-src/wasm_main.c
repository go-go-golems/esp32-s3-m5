/*
 * wasm_main.c — reactor wrapper around QuickJS, compiled to wasm32-wasip1.
 * Exports qjs_init / qjs_eval; imports host_* from module "env" (WAMR native symbols).
 * See design doc §7.1. This is the Boundary-B (QuickJS<->user JS) glue.
 *
 * Build: see build-quickjs-wasm.sh
 */
#include "quickjs.h"
#include <string.h>

/* ---- Boundary A imports: provided by WAMR native symbols, module "env". ----
 * host_print takes a single NUL-terminated string: WAMR signature "($)"
 * copies the guest string and NUL-terminates it for the host. */
__attribute__((import_module("env"), import_name("host_print")))
extern void host_print(const char *s);

__attribute__((import_module("env"), import_name("host_millis")))
extern int host_millis(void);

__attribute__((import_module("env"), import_name("host_gpio_write")))
extern void host_gpio_write(int pin, int val);

/* ---- QuickJS state ---- */
static JSRuntime *rt;
static JSContext  *ctx;

/* ---- Boundary B: JS globals implemented in C, calling the imports ---- */
static JSValue js_print(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv)
{
    (void)ths; (void)argc;
    const char *s = JS_ToCString(c, argv[0]);
    if (s) {
        host_print(s);
        host_print("\n");
        JS_FreeCString(c, s);
    }
    return JS_UNDEFINED;
}

static JSValue js_millis(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv)
{
    (void)ths; (void)argc; (void)argv;
    return JS_NewInt32(c, host_millis());
}

static JSValue js_gpio_write(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv)
{
    (void)ths;
    int pin = 0, val = 0;
    if (argc >= 2) {
        JS_ToInt32(c, &pin, argv[0]);
        JS_ToInt32(c, &val, argv[1]);
    }
    host_gpio_write(pin, val);
    return JS_UNDEFINED;
}

/* ---- Exported entry points (called by the WAMR host) ---- */
void qjs_init(void)
{
    rt = JS_NewRuntime();
    JS_SetMemoryLimit(rt, 256 * 1024);
    /* Disable QuickJS's C-stack overflow check: under WAMR's interpreter the
       C stack pointer (__builtin_frame_address) does not track wasm/js call
       recursion, so the default limit false-trips as 'SyntaxError: stack
       overflow' even for `1+2`. Wasm stack bounds are enforced by WAMR. */
    JS_SetMaxStackSize(rt, 0);
    ctx = JS_NewContext(rt);
    JSValue g = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, g, "print",      JS_NewCFunction(ctx, js_print,      "print", 1));
    JS_SetPropertyStr(ctx, g, "millis",     JS_NewCFunction(ctx, js_millis,     "millis", 0));
    JS_SetPropertyStr(ctx, g, "gpio_write", JS_NewCFunction(ctx, js_gpio_write, "gpio_write", 2));
    JS_FreeValue(ctx, g);
}

int qjs_eval(const char *src, int len)
{
    if (!ctx) {
        qjs_init();
    }
    JSValue r = JS_Eval(ctx, src, (size_t)len, "<console>", JS_EVAL_TYPE_GLOBAL);
    int ok = JS_IsException(r) ? -1 : 0;
    if (ok != 0) {
        JSValue e = JS_GetException(ctx);
        const char *m = JS_ToCString(ctx, e);
        if (m) {
            host_print(m);
            host_print("\n");
            JS_FreeCString(ctx, m);
        }
        JS_FreeValue(ctx, e);
    }
    JS_FreeValue(ctx, r);
    return ok;
}
