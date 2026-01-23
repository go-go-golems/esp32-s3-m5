#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"

#include "esp_timer.h"

#include "mquickjs.h"

#include "sdkconfig.h"

#include "sim_engine.h"

#include "mqjs_timers.h"

static sim_engine_t *s_engine = NULL;
static uint32_t s_next_timeout_id = 1;
static bool s_gpio_inited = false;

void mqjs_sim_set_engine(sim_engine_t *engine) {
  s_engine = engine;
}

static JSValue js_throw_sim(JSContext *ctx, const char *msg) {
  return JS_ThrowTypeError(ctx, "%s", msg ? msg : "sim error");
}

static bool parse_hex_color(const char *s, led_rgb8_t *out) {
  if (!s || !out) return false;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
  if (*s == '#') s++;
  if (strlen(s) != 6) return false;

  char *end = NULL;
  unsigned long v = strtoul(s, &end, 16);
  if (!end || *end != '\0' || v > 0xFFFFFFul) return false;
  out->r = (uint8_t)((v >> 16) & 0xFF);
  out->g = (uint8_t)((v >> 8) & 0xFF);
  out->b = (uint8_t)(v & 0xFF);
  return true;
}

static JSValue js_sim_status(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  if (!s_engine) {
    return js_throw_sim(ctx, "sim.status: engine not bound");
  }

  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);
  const uint32_t frame_ms = sim_engine_get_frame_ms(s_engine);
  const uint16_t led_count = s_engine->strip.cfg.led_count;

  JSValue root = JS_NewObject(ctx);
  (void)JS_SetPropertyStr(ctx, root, "ok", JS_NewBool(1));
  (void)JS_SetPropertyStr(ctx, root, "led_count", JS_NewUint32(ctx, (uint32_t)led_count));
  (void)JS_SetPropertyStr(ctx, root, "frame_ms", JS_NewUint32(ctx, frame_ms));
  (void)JS_SetPropertyStr(ctx, root, "brightness_pct", JS_NewUint32(ctx, (uint32_t)cfg.global_brightness_pct));

  const char *type = "off";
  switch (cfg.type) {
    case LED_PATTERN_OFF:
      type = "off";
      break;
    case LED_PATTERN_RAINBOW:
      type = "rainbow";
      break;
    case LED_PATTERN_CHASE:
      type = "chase";
      break;
    case LED_PATTERN_BREATHING:
      type = "breathing";
      break;
    case LED_PATTERN_SPARKLE:
      type = "sparkle";
      break;
    default:
      type = "off";
      break;
  }
  (void)JS_SetPropertyStr(ctx, root, "type", JS_NewString(ctx, type));

  // Always include per-pattern objects so callers can introspect without branching.
  JSValue rainbow = JS_NewObject(ctx);
  (void)JS_SetPropertyStr(ctx, rainbow, "speed", JS_NewUint32(ctx, (uint32_t)cfg.u.rainbow.speed));
  (void)JS_SetPropertyStr(ctx, rainbow, "sat", JS_NewUint32(ctx, (uint32_t)cfg.u.rainbow.saturation));
  (void)JS_SetPropertyStr(ctx, rainbow, "spread", JS_NewUint32(ctx, (uint32_t)cfg.u.rainbow.spread_x10));
  (void)JS_SetPropertyStr(ctx, root, "rainbow", rainbow);

  JSValue chase = JS_NewObject(ctx);
  (void)JS_SetPropertyStr(ctx, chase, "speed", JS_NewUint32(ctx, (uint32_t)cfg.u.chase.speed));
  (void)JS_SetPropertyStr(ctx, chase, "tail", JS_NewUint32(ctx, (uint32_t)cfg.u.chase.tail_len));
  (void)JS_SetPropertyStr(ctx, chase, "gap", JS_NewUint32(ctx, (uint32_t)cfg.u.chase.gap_len));
  (void)JS_SetPropertyStr(ctx, chase, "trains", JS_NewUint32(ctx, (uint32_t)cfg.u.chase.trains));
  (void)JS_SetPropertyStr(ctx, chase, "dir", JS_NewUint32(ctx, (uint32_t)cfg.u.chase.dir));
  (void)JS_SetPropertyStr(ctx, chase, "fade", JS_NewBool(cfg.u.chase.fade_tail ? 1 : 0));
  (void)JS_SetPropertyStr(ctx, root, "chase", chase);

  JSValue breathing = JS_NewObject(ctx);
  (void)JS_SetPropertyStr(ctx, breathing, "speed", JS_NewUint32(ctx, (uint32_t)cfg.u.breathing.speed));
  (void)JS_SetPropertyStr(ctx, breathing, "min", JS_NewUint32(ctx, (uint32_t)cfg.u.breathing.min_bri));
  (void)JS_SetPropertyStr(ctx, breathing, "max", JS_NewUint32(ctx, (uint32_t)cfg.u.breathing.max_bri));
  (void)JS_SetPropertyStr(ctx, breathing, "curve", JS_NewUint32(ctx, (uint32_t)cfg.u.breathing.curve));
  (void)JS_SetPropertyStr(ctx, root, "breathing", breathing);

  JSValue sparkle = JS_NewObject(ctx);
  (void)JS_SetPropertyStr(ctx, sparkle, "speed", JS_NewUint32(ctx, (uint32_t)cfg.u.sparkle.speed));
  (void)JS_SetPropertyStr(ctx, sparkle, "density", JS_NewUint32(ctx, (uint32_t)cfg.u.sparkle.density_pct));
  (void)JS_SetPropertyStr(ctx, sparkle, "fade", JS_NewUint32(ctx, (uint32_t)cfg.u.sparkle.fade_speed));
  (void)JS_SetPropertyStr(ctx, sparkle, "mode", JS_NewUint32(ctx, (uint32_t)cfg.u.sparkle.color_mode));
  (void)JS_SetPropertyStr(ctx, root, "sparkle", sparkle);

  return root;
}

static JSValue js_sim_setFrameMs(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setFrameMs: engine not bound");
  if (argc < 1) return JS_ThrowTypeError(ctx, "sim.setFrameMs(ms) requires 1 argument");
  int ms = 0;
  if (JS_ToInt32(ctx, &ms, argv[0])) return JS_EXCEPTION;
  if (ms <= 0 || ms > 1000) return JS_ThrowRangeError(ctx, "frame_ms out of range (1..1000)");
  sim_engine_set_frame_ms(s_engine, (uint32_t)ms);
  return JS_NewUint32(ctx, sim_engine_get_frame_ms(s_engine));
}

static JSValue js_sim_setBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setBrightness: engine not bound");
  if (argc < 1) return JS_ThrowTypeError(ctx, "sim.setBrightness(pct) requires 1 argument");
  int pct = 0;
  if (JS_ToInt32(ctx, &pct, argv[0])) return JS_EXCEPTION;
  if (pct < 1 || pct > 100) return JS_ThrowRangeError(ctx, "brightness out of range (1..100)");
  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);
  cfg.global_brightness_pct = (uint8_t)pct;
  sim_engine_set_cfg(s_engine, &cfg);
  return JS_NewUint32(ctx, (uint32_t)pct);
}

static JSValue js_sim_setPattern(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setPattern: engine not bound");
  if (argc < 1) return JS_ThrowTypeError(ctx, "sim.setPattern(type) requires 1 argument");
  if (!JS_IsString(ctx, argv[0])) return JS_ThrowTypeError(ctx, "sim.setPattern expects a string");

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t len = 0;
  const char *s = JS_ToCStringLen(ctx, &len, argv[0], &sbuf);
  if (!s || len == 0) return JS_ThrowTypeError(ctx, "sim.setPattern: invalid string");

  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);

  if (strcmp(s, "off") == 0) {
    cfg.type = LED_PATTERN_OFF;
  } else if (strcmp(s, "rainbow") == 0) {
    cfg.type = LED_PATTERN_RAINBOW;
  } else if (strcmp(s, "chase") == 0) {
    cfg.type = LED_PATTERN_CHASE;
  } else if (strcmp(s, "breathing") == 0) {
    cfg.type = LED_PATTERN_BREATHING;
  } else if (strcmp(s, "sparkle") == 0) {
    cfg.type = LED_PATTERN_SPARKLE;
  } else {
    return JS_ThrowTypeError(ctx, "sim.setPattern: invalid type (off|rainbow|chase|breathing|sparkle)");
  }

  sim_engine_set_cfg(s_engine, &cfg);
  return JS_NewString(ctx, s);
}

static JSValue js_sim_setRainbow(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setRainbow: engine not bound");
  if (argc < 3) return JS_ThrowTypeError(ctx, "sim.setRainbow(speed,sat,spread) requires 3 arguments");
  int speed = 0, sat = 0, spread = 0;
  if (JS_ToInt32(ctx, &speed, argv[0]) || JS_ToInt32(ctx, &sat, argv[1]) || JS_ToInt32(ctx, &spread, argv[2])) {
    return JS_EXCEPTION;
  }
  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);
  cfg.type = LED_PATTERN_RAINBOW;
  cfg.u.rainbow.speed = (uint8_t)speed;
  cfg.u.rainbow.saturation = (uint8_t)sat;
  cfg.u.rainbow.spread_x10 = (uint8_t)spread;
  sim_engine_set_cfg(s_engine, &cfg);
  return JS_UNDEFINED;
}

static JSValue js_sim_setChase(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setChase: engine not bound");
  if (argc < 9) {
    return JS_ThrowTypeError(ctx,
                            "sim.setChase(speed,tail,gap,trains,fgHex,bgHex,dir,fade,briPct) requires 9 args");
  }
  int speed = 0, tail = 0, gap = 0, trains = 0, dir = 0, fade = 0, bri = 0;
  if (JS_ToInt32(ctx, &speed, argv[0]) || JS_ToInt32(ctx, &tail, argv[1]) || JS_ToInt32(ctx, &gap, argv[2]) ||
      JS_ToInt32(ctx, &trains, argv[3]) || JS_ToInt32(ctx, &dir, argv[6]) || JS_ToInt32(ctx, &fade, argv[7]) ||
      JS_ToInt32(ctx, &bri, argv[8])) {
    return JS_EXCEPTION;
  }
  if (!JS_IsString(ctx, argv[4]) || !JS_IsString(ctx, argv[5])) {
    return JS_ThrowTypeError(ctx, "sim.setChase: fgHex/bgHex must be strings like #RRGGBB");
  }

  JSCStringBuf sbuf_fg, sbuf_bg;
  memset(&sbuf_fg, 0, sizeof(sbuf_fg));
  memset(&sbuf_bg, 0, sizeof(sbuf_bg));
  size_t fg_len = 0, bg_len = 0;
  const char *fg = JS_ToCStringLen(ctx, &fg_len, argv[4], &sbuf_fg);
  const char *bg = JS_ToCStringLen(ctx, &bg_len, argv[5], &sbuf_bg);
  led_rgb8_t fg_c = {0}, bg_c = {0};
  if (!parse_hex_color(fg, &fg_c) || !parse_hex_color(bg, &bg_c)) {
    return JS_ThrowTypeError(ctx, "sim.setChase: invalid color");
  }

  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);
  cfg.type = LED_PATTERN_CHASE;
  cfg.global_brightness_pct = (uint8_t)bri;
  cfg.u.chase.speed = (uint8_t)speed;
  cfg.u.chase.tail_len = (uint8_t)tail;
  cfg.u.chase.gap_len = (uint8_t)gap;
  cfg.u.chase.trains = (uint8_t)trains;
  cfg.u.chase.fg = fg_c;
  cfg.u.chase.bg = bg_c;
  cfg.u.chase.dir = (led_direction_t)(uint8_t)dir;
  cfg.u.chase.fade_tail = (fade != 0);
  sim_engine_set_cfg(s_engine, &cfg);
  return JS_UNDEFINED;
}

static JSValue js_sim_setBreathing(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setBreathing: engine not bound");
  if (argc < 5) {
    return JS_ThrowTypeError(ctx, "sim.setBreathing(speed,colorHex,min,max,curve) requires 5 args");
  }
  int speed = 0, min = 0, max = 0, curve = 0;
  if (JS_ToInt32(ctx, &speed, argv[0]) || JS_ToInt32(ctx, &min, argv[2]) || JS_ToInt32(ctx, &max, argv[3]) ||
      JS_ToInt32(ctx, &curve, argv[4])) {
    return JS_EXCEPTION;
  }
  if (!JS_IsString(ctx, argv[1])) return JS_ThrowTypeError(ctx, "sim.setBreathing: colorHex must be string");

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t len = 0;
  const char *s = JS_ToCStringLen(ctx, &len, argv[1], &sbuf);
  led_rgb8_t c = {0};
  if (!parse_hex_color(s, &c)) return JS_ThrowTypeError(ctx, "sim.setBreathing: invalid color");

  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);
  cfg.type = LED_PATTERN_BREATHING;
  cfg.u.breathing.speed = (uint8_t)speed;
  cfg.u.breathing.color = c;
  cfg.u.breathing.min_bri = (uint8_t)min;
  cfg.u.breathing.max_bri = (uint8_t)max;
  cfg.u.breathing.curve = (led_curve_t)(uint8_t)curve;
  sim_engine_set_cfg(s_engine, &cfg);
  return JS_UNDEFINED;
}

static JSValue js_sim_setSparkle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (!s_engine) return js_throw_sim(ctx, "sim.setSparkle: engine not bound");
  if (argc < 6) {
    return JS_ThrowTypeError(ctx, "sim.setSparkle(speed,colorHex,density,fade,mode,bgHex) requires 6 args");
  }
  int speed = 0, density = 0, fade = 0, mode = 0;
  if (JS_ToInt32(ctx, &speed, argv[0]) || JS_ToInt32(ctx, &density, argv[2]) || JS_ToInt32(ctx, &fade, argv[3]) ||
      JS_ToInt32(ctx, &mode, argv[4])) {
    return JS_EXCEPTION;
  }
  if (!JS_IsString(ctx, argv[1]) || !JS_IsString(ctx, argv[5])) {
    return JS_ThrowTypeError(ctx, "sim.setSparkle: colorHex/bgHex must be strings");
  }
  JSCStringBuf sbuf_c, sbuf_bg;
  memset(&sbuf_c, 0, sizeof(sbuf_c));
  memset(&sbuf_bg, 0, sizeof(sbuf_bg));
  size_t c_len = 0, bg_len = 0;
  const char *c_s = JS_ToCStringLen(ctx, &c_len, argv[1], &sbuf_c);
  const char *bg_s = JS_ToCStringLen(ctx, &bg_len, argv[5], &sbuf_bg);
  led_rgb8_t c = {0}, bg = {0};
  if (!parse_hex_color(c_s, &c) || !parse_hex_color(bg_s, &bg)) {
    return JS_ThrowTypeError(ctx, "sim.setSparkle: invalid color");
  }

  led_pattern_cfg_t cfg = {};
  sim_engine_get_cfg(s_engine, &cfg);
  cfg.type = LED_PATTERN_SPARKLE;
  cfg.u.sparkle.speed = (uint8_t)speed;
  cfg.u.sparkle.color = c;
  cfg.u.sparkle.density_pct = (uint8_t)density;
  cfg.u.sparkle.fade_speed = (uint8_t)fade;
  cfg.u.sparkle.color_mode = (led_sparkle_color_mode_t)(uint8_t)mode;
  cfg.u.sparkle.background = bg;
  sim_engine_set_cfg(s_engine, &cfg);
  return JS_UNDEFINED;
}

// Time functions for Date.now / performance.now.
static int64_t get_time_ms(void) {
  return (int64_t)(esp_timer_get_time() / 1000);
}

static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "load() not supported in 0066 (no SPIFFS JS libs yet)");
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
  JSValue ns = JS_GetPropertyStr(ctx, glob, "__0066");
  if (JS_IsUndefined(ns) || JS_IsNull(ns)) {
    ns = JS_NewObject(ctx);
    (void)JS_SetPropertyStr(ctx, glob, "__0066", ns);
    ns = JS_GetPropertyStr(ctx, glob, "__0066");
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

  uint32_t id = s_next_timeout_id++;
  if (id == 0) id = s_next_timeout_id++;

  (void)JS_SetPropertyUint32(ctx, cb, id, argv[0]);

  const esp_err_t st = mqjs_0066_timers_schedule(id, (uint32_t)ms);
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

  // Best-effort cancel; ignore errors (e.g. not scheduled).
  (void)mqjs_0066_timers_cancel((uint32_t)id);

  // Drop JS-side callback reference if it exists.
  JSValue glob = JS_GetGlobalObject(ctx);
  JSValue ns = JS_GetPropertyStr(ctx, glob, "__0066");
  if (JS_IsUndefined(ns) || JS_IsNull(ns)) {
    return JS_UNDEFINED;
  }

  JSValue timers = JS_GetPropertyStr(ctx, ns, "timers");
  if (JS_IsUndefined(timers) || JS_IsNull(timers)) {
    return JS_UNDEFINED;
  }

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

// Stubs for the generator's default stdlib (we don't expose these in 0066, but the table may contain them).
static bool gpio_parse_label(JSContext *ctx, JSValue v, gpio_num_t *out_gpio) {
  if (!out_gpio) return false;
  *out_gpio = GPIO_NUM_NC;
  if (!JS_IsString(ctx, v)) {
    (void)JS_ThrowTypeError(ctx, "gpio: expected a string label (\"G3\" or \"G4\")");
    return false;
  }

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t len = 0;
  const char *s = JS_ToCStringLen(ctx, &len, v, &sbuf);
  if (!s || len == 0) {
    (void)JS_ThrowTypeError(ctx, "gpio: invalid label");
    return false;
  }

  // Accept "G3"/"g3" and "G4"/"g4".
  if ((len == 2 || len == 3) && (s[0] == 'G' || s[0] == 'g')) {
    if (s[1] == '3') {
      *out_gpio = (gpio_num_t)CONFIG_TUTORIAL_0066_G3_GPIO;
      return true;
    }
    if (s[1] == '4') {
      *out_gpio = (gpio_num_t)CONFIG_TUTORIAL_0066_G4_GPIO;
      return true;
    }
  }

  (void)JS_ThrowTypeError(ctx, "gpio: unsupported label (expected \"G3\" or \"G4\")");
  return false;
}

static void gpio_init_once(void) {
  if (s_gpio_inited) return;
  s_gpio_inited = true;

  const gpio_num_t g3 = (gpio_num_t)CONFIG_TUTORIAL_0066_G3_GPIO;
  const gpio_num_t g4 = (gpio_num_t)CONFIG_TUTORIAL_0066_G4_GPIO;

  gpio_config_t cfg = {};
  cfg.intr_type = GPIO_INTR_DISABLE;
  cfg.mode = GPIO_MODE_OUTPUT;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pin_bit_mask = (1ULL << (uint32_t)g3) | (1ULL << (uint32_t)g4);
  (void)gpio_config(&cfg);

  (void)gpio_set_level(g3, 0);
  (void)gpio_set_level(g4, 0);
}

static JSValue js_gpio_high(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_ThrowTypeError(ctx, "gpio.high(label) requires 1 argument");
  gpio_num_t gpio = GPIO_NUM_NC;
  if (!gpio_parse_label(ctx, argv[0], &gpio)) return JS_EXCEPTION;
  gpio_init_once();
  (void)gpio_set_level(gpio, 1);
  return JS_UNDEFINED;
}
static JSValue js_gpio_low(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) return JS_ThrowTypeError(ctx, "gpio.low(label) requires 1 argument");
  gpio_num_t gpio = GPIO_NUM_NC;
  if (!gpio_parse_label(ctx, argv[0], &gpio)) return JS_EXCEPTION;
  gpio_init_once();
  (void)gpio_set_level(gpio, 0);
  return JS_UNDEFINED;
}
static JSValue js_gpio_square(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "gpio.square() not supported in 0066");
}
static JSValue js_gpio_pulse(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "gpio.pulse() not supported in 0066");
}
static JSValue js_gpio_set_many(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "gpio.setMany() not supported in 0066");
}
static JSValue js_gpio_stop(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "gpio.stop() not supported in 0066");
}

static JSValue js_i2c_config(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.config() not supported in 0066");
}
static JSValue js_i2c_scan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.scan() not supported in 0066");
}
static JSValue js_i2c_write_reg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.writeReg() not supported in 0066");
}
static JSValue js_i2c_read_reg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.readReg() not supported in 0066");
}
static JSValue js_i2c_deconfig(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.deconfig() not supported in 0066");
}
static JSValue js_i2c_tx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.tx() not supported in 0066");
}
static JSValue js_i2c_txrx(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)ctx;
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_ThrowTypeError(ctx, "i2c.txrx() not supported in 0066");
}

// Pull in the generated ROM-style table, which references the symbols above.
#include "esp32_stdlib.h"
