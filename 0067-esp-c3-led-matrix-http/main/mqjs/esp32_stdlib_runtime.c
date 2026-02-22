#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_timer.h"

#include "mquickjs.h"

#include "matrix_engine.h"
#include "mqjs_timers.h"

static uint32_t s_next_timeout_id = 1;
static bool s_stop_requested = false;
static char s_last_text[65] = "HELLO";
static uint32_t s_frame_ms = 33;
static const uint32_t s_timeout_id_ring = 1024;

void mqjs_0067_runtime_request_stop(void) {
  s_stop_requested = true;
}

void mqjs_0067_runtime_clear_stop(void) {
  s_stop_requested = false;
}

static JSValue js_throw_err(JSContext *ctx, const char *msg) {
  return JS_ThrowTypeError(ctx, "%s", msg ? msg : "error");
}

static const char *mode_to_str(matrix_mode_t mode) {
  switch (mode) {
    case MATRIX_MODE_TEXT:
      return "text";
    case MATRIX_MODE_SCROLL:
      return "scroll";
    case MATRIX_MODE_DROP:
      return "drop";
    case MATRIX_MODE_SCRIPT:
      return "script";
    case MATRIX_MODE_IDLE:
    default:
      return "idle";
  }
}

static const char *scroll_loop_to_str(matrix_scroll_loop_t mode) {
  switch (mode) {
    case MATRIX_SCROLL_LOOP_WRAP:
      return "wrap";
    case MATRIX_SCROLL_LOOP_RIGHT_EXIT:
      return "right_exit";
    case MATRIX_SCROLL_LOOP_GAP:
    default:
      return "gap";
  }
}

static bool scroll_loop_from_str(const char *s, matrix_scroll_loop_t *out) {
  if (!s || !out) return false;
  if (strcmp(s, "gap") == 0) {
    *out = MATRIX_SCROLL_LOOP_GAP;
    return true;
  }
  if (strcmp(s, "wrap") == 0 || strcmp(s, "loop") == 0 || strcmp(s, "direct") == 0) {
    *out = MATRIX_SCROLL_LOOP_WRAP;
    return true;
  }
  if (strcmp(s, "right_exit") == 0 || strcmp(s, "exit_right") == 0) {
    *out = MATRIX_SCROLL_LOOP_RIGHT_EXIT;
    return true;
  }
  return false;
}

static int64_t now_us(void) {
  return esp_timer_get_time();
}

static int64_t now_ms(void) {
  return now_us() / 1000;
}

static bool value_to_int(JSContext *ctx, JSValue v, int *out, int def) {
  if (JS_IsUndefined(v) || JS_IsNull(v)) {
    *out = def;
    return true;
  }
  if (JS_ToInt32(ctx, out, v)) return false;
  return true;
}

static bool get_obj_int(JSContext *ctx, JSValue obj, const char *key, int *out, int def) {
  JSValue v = JS_GetPropertyStr(ctx, obj, key);
  return value_to_int(ctx, v, out, def);
}

static bool get_obj_bool(JSContext *ctx, JSValue obj, const char *key, bool *out, bool def) {
  int n = def ? 1 : 0;
  JSValue v = JS_GetPropertyStr(ctx, obj, key);
  if (!value_to_int(ctx, v, &n, n)) return false;
  *out = (n != 0);
  return true;
}

static bool get_obj_string(JSContext *ctx, JSValue obj, const char *key, char *out, size_t out_n) {
  if (!out || out_n == 0) return false;
  out[0] = '\0';
  JSValue v = JS_GetPropertyStr(ctx, obj, key);
  if (JS_IsUndefined(v) || JS_IsNull(v)) return true;

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t n = 0;
  const char *s = JS_ToCStringLen(ctx, &n, v, &sbuf);
  if (!s) return false;
  strlcpy(out, s, out_n);
  return true;
}

static JSValue matrix_status_to_js(JSContext *ctx) {
  matrix_status_t st = {0};
  if (matrix_engine_get_status(&st) != ESP_OK) return js_throw_err(ctx, "matrix status failed");

  JSValue root = JS_NewObject(ctx);
  (void)JS_SetPropertyStr(ctx, root, "ok", JS_NewBool(1));
  (void)JS_SetPropertyStr(ctx, root, "ready", JS_NewBool(st.ready ? 1 : 0));
  (void)JS_SetPropertyStr(ctx, root, "mode", JS_NewString(ctx, mode_to_str(st.mode)));
  (void)JS_SetPropertyStr(ctx, root, "text", JS_NewString(ctx, st.text));
  (void)JS_SetPropertyStr(ctx, root, "chain_len", JS_NewUint32(ctx, (uint32_t)st.chain_len));
  (void)JS_SetPropertyStr(ctx, root, "width", JS_NewUint32(ctx, (uint32_t)st.width));
  (void)JS_SetPropertyStr(ctx, root, "height", JS_NewUint32(ctx, 8));
  (void)JS_SetPropertyStr(ctx, root, "spi_hz", JS_NewUint32(ctx, (uint32_t)st.spi_hz));
  (void)JS_SetPropertyStr(ctx, root, "intensity", JS_NewUint32(ctx, (uint32_t)st.intensity));
  (void)JS_SetPropertyStr(ctx, root, "test_mode", JS_NewBool(st.test_mode ? 1 : 0));
  (void)JS_SetPropertyStr(ctx, root, "fps", JS_NewUint32(ctx, st.fps));
  (void)JS_SetPropertyStr(ctx, root, "pause_ms", JS_NewUint32(ctx, st.pause_ms));
  (void)JS_SetPropertyStr(ctx, root, "repeat_count", JS_NewUint32(ctx, st.repeat_count));
  (void)JS_SetPropertyStr(ctx, root, "loop_mode", JS_NewString(ctx, scroll_loop_to_str(st.scroll_loop)));
  (void)JS_SetPropertyStr(ctx, root, "reverse_modules", JS_NewBool(st.reverse_modules ? 1 : 0));
  (void)JS_SetPropertyStr(ctx, root, "flip_vertical", JS_NewBool(st.flip_vertical ? 1 : 0));
  (void)JS_SetPropertyStr(ctx, root, "stop_requested", JS_NewBool(s_stop_requested ? 1 : 0));
  return root;
}

static JSValue js_sim_status(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return matrix_status_to_js(ctx);
}

static JSValue js_sim_setFrameMs(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_ThrowTypeError(ctx, "sim.setFrameMs(ms) requires 1 argument");
  int ms = 0;
  if (JS_ToInt32(ctx, &ms, argv[0])) return JS_EXCEPTION;
  if (ms < 1 || ms > 2000) return JS_ThrowRangeError(ctx, "frame_ms out of range (1..2000)");
  s_frame_ms = (uint32_t)ms;
  return JS_NewUint32(ctx, s_frame_ms);
}

static JSValue js_sim_setBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_ThrowTypeError(ctx, "sim.setBrightness(pct) requires 1 argument");
  int pct = 0;
  if (JS_ToInt32(ctx, &pct, argv[0])) return JS_EXCEPTION;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  int intensity = (pct * 15 + 50) / 100;
  if (matrix_engine_set_intensity(intensity) != ESP_OK) return js_throw_err(ctx, "set intensity failed");
  return JS_NewUint32(ctx, (uint32_t)pct);
}

static JSValue js_sim_setPattern(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1 || !JS_IsString(ctx, argv[0])) {
    return JS_ThrowTypeError(ctx, "sim.setPattern(type) expects string");
  }

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t n = 0;
  const char *type = JS_ToCStringLen(ctx, &n, argv[0], &sbuf);
  if (!type || n == 0) return JS_ThrowTypeError(ctx, "invalid pattern type");

  esp_err_t err = ESP_ERR_INVALID_ARG;
  if (strcmp(type, "off") == 0) err = matrix_engine_stop();
  else if (strcmp(type, "scroll") == 0) err = matrix_engine_start_scroll(s_last_text, 15, 250, 0, false, MATRIX_SCROLL_LOOP_GAP);
  else if (strcmp(type, "wave") == 0) err = matrix_engine_start_scroll(s_last_text, 15, 250, 0, true, MATRIX_SCROLL_LOOP_GAP);
  else if (strcmp(type, "drop") == 0) err = matrix_engine_start_drop(s_last_text, 15, 250, 0);

  if (err != ESP_OK) return js_throw_err(ctx, "setPattern failed");
  return JS_NewString(ctx, type);
}

static JSValue js_sim_setRainbow(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  int fps = 15, pause_ms = 250, repeat = 0;
  if (argc >= 1 && JS_ToInt32(ctx, &fps, argv[0])) return JS_EXCEPTION;
  if (argc >= 2 && JS_ToInt32(ctx, &pause_ms, argv[1])) return JS_EXCEPTION;
  if (argc >= 3 && JS_ToInt32(ctx, &repeat, argv[2])) return JS_EXCEPTION;
  if (matrix_engine_start_scroll(s_last_text, (uint32_t)fps, (uint32_t)pause_ms, (uint32_t)repeat, true, MATRIX_SCROLL_LOOP_GAP) != ESP_OK) {
    return js_throw_err(ctx, "setRainbow failed");
  }
  return JS_UNDEFINED;
}

static JSValue js_sim_setChase(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  if (matrix_engine_start_scroll(s_last_text, 15, 250, 0, false, MATRIX_SCROLL_LOOP_GAP) != ESP_OK) return js_throw_err(ctx, "setChase failed");
  return JS_UNDEFINED;
}

static JSValue js_sim_setBreathing(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  if (matrix_engine_start_drop(s_last_text, 15, 250, 0) != ESP_OK) return js_throw_err(ctx, "setBreathing failed");
  return JS_UNDEFINED;
}

static JSValue js_sim_setSparkle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  if (matrix_engine_start_scroll(s_last_text, 15, 250, 0, true, MATRIX_SCROLL_LOOP_GAP) != ESP_OK) return js_throw_err(ctx, "setSparkle failed");
  return JS_UNDEFINED;
}

// Time functions for Date.now / performance.now.
static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, now_ms());
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, now_ms());
}

static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "load() not supported in 0067");
}

static JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_ThrowTypeError(ctx, "setTimeout(fn, ms) requires at least 1 argument");
  if (!JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "setTimeout: fn must be a function");

  int ms = 0;
  if (argc >= 2) {
    if (JS_ToInt32(ctx, &ms, argv[1])) return JS_EXCEPTION;
  }
  if (ms < 0) ms = 0;
  if (ms > 3600000) ms = 3600000;

  JSValue glob = JS_GetGlobalObject(ctx);
  JSValue ns = JS_GetPropertyStr(ctx, glob, "__0067");
  if (JS_IsUndefined(ns) || JS_IsNull(ns)) {
    ns = JS_NewObject(ctx);
    (void)JS_SetPropertyStr(ctx, glob, "__0067", ns);
    ns = JS_GetPropertyStr(ctx, glob, "__0067");
  }

  JSValue timers = JS_GetPropertyStr(ctx, ns, "timers");
  if (JS_IsUndefined(timers) || JS_IsNull(timers)) {
    timers = JS_NewObject(ctx);
    (void)JS_SetPropertyStr(ctx, ns, "timers", timers);
    timers = JS_GetPropertyStr(ctx, ns, "timers");
  }

  JSValue cb = JS_GetPropertyStr(ctx, timers, "cb");
  if (JS_IsUndefined(cb) || JS_IsNull(cb)) {
    cb = JS_NewObject(ctx);
    (void)JS_SetPropertyStr(ctx, timers, "cb", cb);
    cb = JS_GetPropertyStr(ctx, timers, "cb");
  }

  uint32_t id = 0;
  for (uint32_t tries = 0; tries < s_timeout_id_ring; tries++) {
    uint32_t cand = s_next_timeout_id++;
    if (cand == 0 || cand > s_timeout_id_ring) {
      s_next_timeout_id = 1;
      cand = s_next_timeout_id++;
    }

    JSValue existing = JS_GetPropertyUint32(ctx, cb, cand);
    if (JS_IsUndefined(existing) || JS_IsNull(existing)) {
      id = cand;
      break;
    }
  }

  if (id == 0) {
    return JS_ThrowInternalError(ctx, "setTimeout: no timer id slots available");
  }

  (void)JS_SetPropertyUint32(ctx, cb, id, argv[0]);

  const esp_err_t st = mqjs_0067_timers_schedule(id, (uint32_t)ms);
  if (st != ESP_OK) {
    (void)JS_SetPropertyUint32(ctx, cb, id, JS_NULL);
    return JS_ThrowInternalError(ctx, "setTimeout: schedule failed (%s)", esp_err_to_name(st));
  }

  return JS_NewUint32(ctx, id);
}

static JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_UNDEFINED;
  int id = 0;
  if (JS_ToInt32(ctx, &id, argv[0])) return JS_EXCEPTION;
  if (id <= 0) return JS_UNDEFINED;

  (void)mqjs_0067_timers_cancel((uint32_t)id);

  JSValue glob = JS_GetGlobalObject(ctx);
  JSValue ns = JS_GetPropertyStr(ctx, glob, "__0067");
  if (JS_IsUndefined(ns) || JS_IsNull(ns)) return JS_UNDEFINED;

  JSValue timers = JS_GetPropertyStr(ctx, ns, "timers");
  if (JS_IsUndefined(timers) || JS_IsNull(timers)) return JS_UNDEFINED;

  JSValue cb = JS_GetPropertyStr(ctx, timers, "cb");
  if (!JS_IsUndefined(cb) && !JS_IsNull(cb)) {
    (void)JS_SetPropertyUint32(ctx, cb, (uint32_t)id, JS_NULL);
  }
  return JS_UNDEFINED;
}

static JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  for (int i = 0; i < argc; i++) {
    if (i != 0) putchar(' ');
    JSValue v = argv[i];
    if (JS_IsString(ctx, v)) {
      JSCStringBuf buf;
      memset(&buf, 0, sizeof(buf));
      size_t len = 0;
      const char *str = JS_ToCStringLen(ctx, &len, v, &buf);
      if (str && len > 0) fwrite(str, 1, len, stdout);
    } else {
      JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
    }
  }
  putchar('\n');
  return JS_UNDEFINED;
}

static JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  JS_GC(ctx);
  return JS_UNDEFINED;
}

static JSValue js_i2c_tx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
static JSValue js_i2c_txrx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

static JSValue js_gpio_high(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  s_stop_requested = false;
  return JS_UNDEFINED;
}

static JSValue js_gpio_low(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  s_stop_requested = true;
  return JS_UNDEFINED;
}

static JSValue js_gpio_square(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_UNDEFINED;
  int ms = 0;
  if (JS_ToInt32(ctx, &ms, argv[0])) return JS_EXCEPTION;
  if (ms < 0) ms = 0;
  if (ms > 0) vTaskDelay(pdMS_TO_TICKS((uint32_t)ms));
  return JS_UNDEFINED;
}

static JSValue js_gpio_pulse(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_UNDEFINED;
  double us = 0;
  if (JS_ToNumber(ctx, &us, argv[0])) return JS_EXCEPTION;
  int64_t target = (int64_t)us;
  while (now_us() < target) {
    if (s_stop_requested) break;
    int64_t rem = target - now_us();
    if (rem > 4000) {
      vTaskDelay(pdMS_TO_TICKS((uint32_t)(rem / 1000) - 1));
    } else {
      taskYIELD();
    }
  }
  return JS_UNDEFINED;
}

static JSValue js_gpio_set_many(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_UNDEFINED;
}

static JSValue js_gpio_stop(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  s_stop_requested = true;
  (void)matrix_engine_stop();
  return JS_UNDEFINED;
}

static JSValue js_i2c_config(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  return js_i2c_tx(ctx, this_val, argc, argv);
}

static JSValue js_i2c_scan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  return js_i2c_txrx(ctx, this_val, argc, argv);
}

static JSValue js_i2c_write_reg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  return js_i2c_tx(ctx, this_val, argc, argv);
}

static JSValue js_i2c_read_reg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  return js_i2c_txrx(ctx, this_val, argc, argv);
}

static JSValue js_i2c_deconfig(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  s_stop_requested = true;
  (void)matrix_engine_stop();
  return JS_UNDEFINED;
}

static JSValue js_i2c_tx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) {
    return JS_ThrowTypeError(ctx, "i2c.tx expects command object");
  }

  JSValue cmd = argv[0];
  char op[32];
  if (!get_obj_string(ctx, cmd, "op", op, sizeof(op)) || op[0] == '\0') {
    return JS_ThrowTypeError(ctx, "command missing op");
  }

  if (strcmp(op, "clear") == 0) {
    return JS_NewBool(matrix_engine_frame_clear() == ESP_OK);
  }

  if (strcmp(op, "fill") == 0) {
    bool on = false;
    if (!get_obj_bool(ctx, cmd, "on", &on, false)) return JS_EXCEPTION;
    return JS_NewBool(matrix_engine_frame_fill(on) == ESP_OK);
  }

  if (strcmp(op, "setPixel") == 0) {
    int x = 0, y = 0;
    bool on = true;
    if (!get_obj_int(ctx, cmd, "x", &x, 0)) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "y", &y, 0)) return JS_EXCEPTION;
    if (!get_obj_bool(ctx, cmd, "on", &on, true)) return JS_EXCEPTION;
    return JS_NewBool(matrix_engine_frame_set_pixel(x, y, on) == ESP_OK);
  }

  if (strcmp(op, "present") == 0) {
    return JS_NewBool(matrix_engine_frame_present() == ESP_OK);
  }

  if (strcmp(op, "text") == 0) {
    char text[65] = {0};
    if (!get_obj_string(ctx, cmd, "text", text, sizeof(text))) return JS_EXCEPTION;
    strlcpy(s_last_text, text, sizeof(s_last_text));
    return JS_NewBool(matrix_engine_set_text(text) == ESP_OK);
  }

  if (strcmp(op, "scroll") == 0) {
    char text[65] = {0};
    int fps = 15, pause_ms = 250, repeat = 0;
    bool wave = false;
    char loop_mode[24] = {0};
    matrix_scroll_loop_t loop = MATRIX_SCROLL_LOOP_GAP;
    if (!get_obj_string(ctx, cmd, "text", text, sizeof(text))) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "fps", &fps, fps)) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "pause_ms", &pause_ms, pause_ms)) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "repeat", &repeat, repeat)) return JS_EXCEPTION;
    if (!get_obj_bool(ctx, cmd, "wave", &wave, false)) return JS_EXCEPTION;
    if (!get_obj_string(ctx, cmd, "loop_mode", loop_mode, sizeof(loop_mode))) return JS_EXCEPTION;
    if (loop_mode[0] != '\0' && !scroll_loop_from_str(loop_mode, &loop)) {
      return JS_ThrowTypeError(ctx, "invalid loop_mode");
    }
    strlcpy(s_last_text, text, sizeof(s_last_text));
    return JS_NewBool(matrix_engine_start_scroll(text, (uint32_t)fps, (uint32_t)pause_ms, (uint32_t)repeat, wave, loop) == ESP_OK);
  }

  if (strcmp(op, "drop") == 0) {
    char text[65] = {0};
    int fps = 15, pause_ms = 250, repeat = 0;
    if (!get_obj_string(ctx, cmd, "text", text, sizeof(text))) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "fps", &fps, fps)) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "pause_ms", &pause_ms, pause_ms)) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "repeat", &repeat, repeat)) return JS_EXCEPTION;
    strlcpy(s_last_text, text, sizeof(s_last_text));
    return JS_NewBool(matrix_engine_start_drop(text, (uint32_t)fps, (uint32_t)pause_ms, (uint32_t)repeat) == ESP_OK);
  }

  if (strcmp(op, "stop") == 0) {
    // Local matrix stop should not latch the cooperative stop flag.
    // Global script cancellation is driven by js_service_request_stop().
    (void)mqjs_0067_timers_cancel_all();
    return JS_NewBool(matrix_engine_stop() == ESP_OK);
  }

  if (strcmp(op, "intensity") == 0) {
    int value = 4;
    if (!get_obj_int(ctx, cmd, "value", &value, value)) return JS_EXCEPTION;
    return JS_NewBool(matrix_engine_set_intensity(value) == ESP_OK);
  }

  if (strcmp(op, "orientation") == 0) {
    bool reverse = false, flipv = true;
    if (!get_obj_bool(ctx, cmd, "reverse", &reverse, false)) return JS_EXCEPTION;
    if (!get_obj_bool(ctx, cmd, "flipv", &flipv, true)) return JS_EXCEPTION;
    return JS_NewBool(matrix_engine_set_orientation(reverse, flipv) == ESP_OK);
  }

  if (strcmp(op, "sleepMs") == 0) {
    int ms = 0;
    if (!get_obj_int(ctx, cmd, "ms", &ms, 0)) return JS_EXCEPTION;
    if (ms < 0) ms = 0;
    if (ms > 0) vTaskDelay(pdMS_TO_TICKS((uint32_t)ms));
    return JS_NewBool(1);
  }

  if (strcmp(op, "sleepUntilUs") == 0) {
    JSValue v = JS_GetPropertyStr(ctx, cmd, "us");
    double us = 0;
    if (JS_ToNumber(ctx, &us, v)) return JS_EXCEPTION;
    int64_t target = (int64_t)us;
    while (now_us() < target) {
      if (s_stop_requested) break;
      int64_t rem = target - now_us();
      if (rem > 4000) vTaskDelay(pdMS_TO_TICKS((uint32_t)(rem / 1000) - 1));
      else taskYIELD();
    }
    return JS_NewBool(1);
  }

  return JS_ThrowTypeError(ctx, "unknown matrix op: %s", op);
}

static JSValue js_i2c_txrx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) {
    return JS_ThrowTypeError(ctx, "i2c.txrx expects command object");
  }

  JSValue cmd = argv[0];
  char op[32];
  if (!get_obj_string(ctx, cmd, "op", op, sizeof(op)) || op[0] == '\0') {
    return JS_ThrowTypeError(ctx, "command missing op");
  }

  if (strcmp(op, "dim") == 0) {
    char key[8] = {0};
    if (argc >= 2 && JS_IsString(ctx, argv[1])) {
      JSCStringBuf sbuf;
      memset(&sbuf, 0, sizeof(sbuf));
      size_t n = 0;
      const char *s = JS_ToCStringLen(ctx, &n, argv[1], &sbuf);
      if (s) strlcpy(key, s, sizeof(key));
    }
    if (strcmp(key, "h") == 0) return JS_NewUint32(ctx, (uint32_t)matrix_engine_height());
    return JS_NewUint32(ctx, (uint32_t)matrix_engine_width());
  }

  if (strcmp(op, "getPixel") == 0) {
    int x = 0, y = 0;
    bool on = false;
    if (!get_obj_int(ctx, cmd, "x", &x, 0)) return JS_EXCEPTION;
    if (!get_obj_int(ctx, cmd, "y", &y, 0)) return JS_EXCEPTION;
    if (matrix_engine_frame_get_pixel(x, y, &on) != ESP_OK) return JS_NewBool(0);
    return JS_NewBool(on ? 1 : 0);
  }

  if (strcmp(op, "status") == 0) {
    return matrix_status_to_js(ctx);
  }

  if (strcmp(op, "shouldStop") == 0) {
    return JS_NewBool(s_stop_requested ? 1 : 0);
  }

  if (strcmp(op, "nowUs") == 0) {
    return JS_NewInt64(ctx, now_us());
  }

  if (strcmp(op, "nowMs") == 0) {
    return JS_NewInt64(ctx, now_ms());
  }

  return JS_ThrowTypeError(ctx, "unknown matrix query op: %s", op);
}

// Pull in generated ROM-style table.
#include "esp32_stdlib.h"
