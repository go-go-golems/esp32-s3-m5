#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_timer.h"

#include "mquickjs.h"

static int64_t get_time_ms(void) {
  return esp_timer_get_time() / 1000;
}

static JSValue js_date_now(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_performance_now(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_load(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "load() not supported");
}

static JSValue js_setTimeout(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "setTimeout() not supported");
}

static JSValue js_clearTimeout(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "clearTimeout() not supported");
}

static JSValue js_print(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;

  for (int i = 0; i < argc; i++) {
    if (i != 0) {
      putchar(' ');
    }

    JSValue v = argv[i];
    if (JS_IsString(ctx, v)) {
      JSCStringBuf buf;
      memset(&buf, 0, sizeof(buf));
      size_t len = 0;
      const char* str = JS_ToCStringLen(ctx, &len, v, &buf);
      if (str && len > 0) {
        fwrite(str, 1, len, stdout);
      }
    } else {
      JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
    }
  }

  putchar('\n');
  return JS_UNDEFINED;
}

static JSValue js_gc(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  JS_GC(ctx);
  return JS_UNDEFINED;
}

#include "esp32_stdlib.h"
