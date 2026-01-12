#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "mquickjs.h"
#include "mquickjs_build.h"

// Time functions
static int64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

// Stub functions for load, setTimeout, clearTimeout (not implemented on embedded)
static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowTypeError(ctx, "load() not supported on embedded platform");
}

static JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowTypeError(ctx, "setTimeout() not supported on embedded platform");
}

static JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowTypeError(ctx, "clearTimeout() not supported on embedded platform");
}

static JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int i;
    JSValue v;
    
    for(i = 0; i < argc; i++) {
        if (i != 0)
            putchar(' ');
        v = argv[i];
        if (JS_IsString(ctx, v)) {
            JSCStringBuf buf;
            const char *str;
            size_t len;
            str = JS_ToCStringLen(ctx, &len, v, &buf);
            fwrite(str, 1, len, stdout);
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    putchar('\n');
    return JS_UNDEFINED;
}

static JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    JS_GC(ctx);
    return JS_UNDEFINED;
}

// Exercizer stubs (generator-only)
static JSValue js_gpio_high(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_gpio_low(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_gpio_square(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_gpio_pulse(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_gpio_set_many(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_gpio_stop(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_config(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_scan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_write_reg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_read_reg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_deconfig(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_tx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

static JSValue js_i2c_txrx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

// Now include the standard library
#define JS_CLASS_COUNT JS_CLASS_USER

#include "mqjs_stdlib.c"
