#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "esp_spiffs.h"
#include "esp_timer.h"

#include "mquickjs.h"

static esp_err_t ensure_spiffs_mounted(bool format_if_mount_failed) {
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 8,
      .format_if_mount_failed = format_if_mount_failed,
  };

  esp_err_t err = esp_vfs_spiffs_register(&conf);
  if (err == ESP_ERR_INVALID_STATE) {
    err = ESP_OK;
  }
  return err;
}

static JSValue js_throw_spiffs(JSContext* ctx, const char* msg, esp_err_t err) {
  return JS_ThrowTypeError(ctx, "%s (%d)", msg, (int)err);
}

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
  if (argc < 1) {
    return JS_ThrowTypeError(ctx, "load(path) requires 1 argument");
  }
  if (!JS_IsString(ctx, argv[0])) {
    return JS_ThrowTypeError(ctx, "load(path) expects a string path");
  }

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t path_len = 0;
  const char* path = JS_ToCStringLen(ctx, &path_len, argv[0], &sbuf);
  if (!path || path_len == 0) {
    return JS_ThrowTypeError(ctx, "load(path) failed to read path string");
  }

  esp_err_t mount_err = ensure_spiffs_mounted(false);
  if (mount_err != ESP_OK) {
    return js_throw_spiffs(ctx, "SPIFFS not mounted; use :autoload --format once", mount_err);
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return JS_ThrowTypeError(ctx, "load: failed to open %s (errno=%d)", path, errno);
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return JS_ThrowTypeError(ctx, "load: fseek failed");
  }
  long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return JS_ThrowTypeError(ctx, "load: ftell failed");
  }
  if (size > (32 * 1024)) {
    fclose(f);
    return JS_ThrowTypeError(ctx, "load: file too large (%ld bytes)", size);
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return JS_ThrowTypeError(ctx, "load: fseek failed");
  }

  char* buf = malloc((size_t)size + 1);
  if (!buf) {
    fclose(f);
    return JS_ThrowOutOfMemory(ctx);
  }
  size_t n = fread(buf, 1, (size_t)size, f);
  fclose(f);
  if (n != (size_t)size) {
    free(buf);
    return JS_ThrowTypeError(ctx, "load: read failed");
  }
  buf[n] = 0;

  JSValue val = JS_Eval(ctx, buf, n, path, 0);
  free(buf);
  return val;
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
