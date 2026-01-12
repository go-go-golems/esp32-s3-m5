#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "esp_spiffs.h"
#include "esp_timer.h"

#include "exercizer/ControlPlaneC.h"
#include "exercizer/ControlPlaneTypes.h"
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

static JSValue js_throw_ctrl(JSContext* ctx, const char* msg) {
  return JS_ThrowTypeError(ctx, "%s", msg);
}

static bool js_read_u8_array(JSContext* ctx, JSValue val, uint8_t* out, size_t max_len, size_t* out_len) {
  if (!out || !out_len) {
    JS_ThrowTypeError(ctx, "internal: invalid buffer");
    return false;
  }
  *out_len = 0;
  if (JS_IsUndefined(val) || JS_IsNull(val)) {
    return true;
  }

  JSValue len_val = JS_GetPropertyStr(ctx, val, "length");
  if (JS_IsException(len_val)) {
    return false;
  }
  uint32_t len = 0;
  if (JS_ToUint32(ctx, &len, len_val)) {
    return false;
  }
  if (len > max_len) {
    JS_ThrowRangeError(ctx, "array too large (max %u)", (unsigned)max_len);
    return false;
  }
  for (uint32_t i = 0; i < len; i++) {
    JSValue v = JS_GetPropertyUint32(ctx, val, i);
    if (JS_IsException(v)) {
      return false;
    }
    uint32_t b = 0;
    if (JS_ToUint32(ctx, &b, v)) {
      return false;
    }
    out[i] = (uint8_t)b;
  }
  *out_len = len;
  return true;
}

static JSValue js_gpio_high(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 1) {
    return js_throw_ctrl(ctx, "gpio.high(pin) requires 1 argument");
  }
  int pin = 0;
  if (JS_ToInt32(ctx, &pin, argv[0])) {
    return JS_EXCEPTION;
  }

  exercizer_gpio_config_t cfg = {
      .pin = (uint8_t)pin,
      .mode = EXERCIZER_GPIO_MODE_HIGH,
      .hz = 0,
      .width_us = 0,
      .period_ms = 0,
  };
  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_GPIO_SET,
      .data_len = sizeof(cfg),
  };
  memcpy(ev.data, &cfg, sizeof(cfg));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "gpio.high: control plane not ready");
  }
  return JS_UNDEFINED;
}

static JSValue js_gpio_low(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 1) {
    return js_throw_ctrl(ctx, "gpio.low(pin) requires 1 argument");
  }
  int pin = 0;
  if (JS_ToInt32(ctx, &pin, argv[0])) {
    return JS_EXCEPTION;
  }

  exercizer_gpio_config_t cfg = {
      .pin = (uint8_t)pin,
      .mode = EXERCIZER_GPIO_MODE_LOW,
      .hz = 0,
      .width_us = 0,
      .period_ms = 0,
  };
  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_GPIO_SET,
      .data_len = sizeof(cfg),
  };
  memcpy(ev.data, &cfg, sizeof(cfg));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "gpio.low: control plane not ready");
  }
  return JS_UNDEFINED;
}

static JSValue js_gpio_square(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 2) {
    return js_throw_ctrl(ctx, "gpio.square(pin, hz) requires 2 arguments");
  }
  int pin = 0;
  int hz = 0;
  if (JS_ToInt32(ctx, &pin, argv[0]) || JS_ToInt32(ctx, &hz, argv[1])) {
    return JS_EXCEPTION;
  }
  if (hz <= 0) {
    return JS_ThrowRangeError(ctx, "gpio.square: hz must be > 0");
  }

  exercizer_gpio_config_t cfg = {
      .pin = (uint8_t)pin,
      .mode = EXERCIZER_GPIO_MODE_SQUARE,
      .hz = (uint32_t)hz,
      .width_us = 0,
      .period_ms = 0,
  };
  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_GPIO_SET,
      .data_len = sizeof(cfg),
  };
  memcpy(ev.data, &cfg, sizeof(cfg));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "gpio.square: control plane not ready");
  }
  return JS_UNDEFINED;
}

static bool js_read_gpio_mode(JSContext *ctx, JSValue obj, uint8_t *out_mode, uint32_t idx) {
  JSValue mode_val = JS_GetPropertyStr(ctx, obj, "mode");
  if (JS_IsException(mode_val)) {
    return false;
  }
  if (JS_IsUndefined(mode_val)) {
    JS_ThrowTypeError(ctx, "gpio.setMany[%u] missing mode", (unsigned)idx);
    return false;
  }
  if (JS_IsString(ctx, mode_val)) {
    JSCStringBuf sbuf;
    memset(&sbuf, 0, sizeof(sbuf));
    size_t mode_len = 0;
    const char *mode_str = JS_ToCStringLen(ctx, &mode_len, mode_val, &sbuf);
    if (!mode_str || mode_len == 0) {
      JS_ThrowTypeError(ctx, "gpio.setMany[%u] invalid mode", (unsigned)idx);
      return false;
    }
    if (strcmp(mode_str, "high") == 0) {
      *out_mode = EXERCIZER_GPIO_MODE_HIGH;
    } else if (strcmp(mode_str, "low") == 0) {
      *out_mode = EXERCIZER_GPIO_MODE_LOW;
    } else if (strcmp(mode_str, "square") == 0) {
      *out_mode = EXERCIZER_GPIO_MODE_SQUARE;
    } else if (strcmp(mode_str, "pulse") == 0) {
      *out_mode = EXERCIZER_GPIO_MODE_PULSE;
    } else {
      JS_ThrowTypeError(ctx, "gpio.setMany[%u] invalid mode", (unsigned)idx);
      return false;
    }
    return true;
  }
  if (JS_IsNumber(ctx, mode_val)) {
    uint32_t mode_num = 0;
    if (JS_ToUint32(ctx, &mode_num, mode_val)) {
      return false;
    }
    if (mode_num < EXERCIZER_GPIO_MODE_HIGH || mode_num > EXERCIZER_GPIO_MODE_PULSE) {
      JS_ThrowRangeError(ctx, "gpio.setMany[%u] mode out of range", (unsigned)idx);
      return false;
    }
    *out_mode = (uint8_t)mode_num;
    return true;
  }
  JS_ThrowTypeError(ctx, "gpio.setMany[%u] mode must be string or number", (unsigned)idx);
  return false;
}

static bool js_read_u32_prop(JSContext *ctx, JSValue obj, const char *name, uint32_t *out, bool required, uint32_t idx) {
  JSValue v = JS_GetPropertyStr(ctx, obj, name);
  if (JS_IsException(v)) {
    return false;
  }
  if (JS_IsUndefined(v)) {
    if (required) {
      JS_ThrowTypeError(ctx, "gpio.setMany[%u] missing %s", (unsigned)idx, name);
      return false;
    }
    *out = 0;
    return true;
  }
  if (JS_ToUint32(ctx, out, v)) {
    return false;
  }
  return true;
}

static bool js_read_gpio_config_obj(JSContext *ctx, JSValue obj, exercizer_gpio_config_t *out, uint32_t idx) {
  uint32_t pin = 0;
  uint32_t hz = 0;
  uint32_t width_us = 0;
  uint32_t period_ms = 0;
  uint8_t mode = 0;

  if (!js_read_u32_prop(ctx, obj, "pin", &pin, true, idx)) {
    return false;
  }
  if (!js_read_gpio_mode(ctx, obj, &mode, idx)) {
    return false;
  }

  if (mode == EXERCIZER_GPIO_MODE_SQUARE) {
    if (!js_read_u32_prop(ctx, obj, "hz", &hz, true, idx)) {
      return false;
    }
    if (hz == 0) {
      JS_ThrowRangeError(ctx, "gpio.setMany[%u] hz must be > 0", (unsigned)idx);
      return false;
    }
  } else if (mode == EXERCIZER_GPIO_MODE_PULSE) {
    if (!js_read_u32_prop(ctx, obj, "width_us", &width_us, true, idx) ||
        !js_read_u32_prop(ctx, obj, "period_ms", &period_ms, true, idx)) {
      return false;
    }
    if (width_us == 0 || period_ms == 0) {
      JS_ThrowRangeError(ctx, "gpio.setMany[%u] width_us/period_ms must be > 0", (unsigned)idx);
      return false;
    }
  }

  memset(out, 0, sizeof(*out));
  out->pin = (uint8_t)pin;
  out->mode = mode;
  out->hz = hz;
  out->width_us = width_us;
  out->period_ms = period_ms;
  return true;
}

static JSValue js_gpio_set_many(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
  (void)this_val;
  if (argc < 1) {
    return js_throw_ctrl(ctx, "gpio.setMany(list) requires 1 argument");
  }
  uint32_t len = 0;
  JSValue len_val = JS_GetPropertyStr(ctx, argv[0], "length");
  if (JS_IsException(len_val)) {
    return JS_EXCEPTION;
  }
  if (JS_ToUint32(ctx, &len, len_val)) {
    return JS_EXCEPTION;
  }
  for (uint32_t i = 0; i < len; i++) {
    JSValue item = JS_GetPropertyUint32(ctx, argv[0], i);
    if (JS_IsException(item)) {
      return JS_EXCEPTION;
    }
    if (JS_IsUndefined(item) || JS_IsNull(item)) {
      return JS_ThrowTypeError(ctx, "gpio.setMany[%u] must be object", (unsigned)i);
    }
    exercizer_gpio_config_t cfg = {};
    bool ok = js_read_gpio_config_obj(ctx, item, &cfg, i);
    if (!ok) {
      return JS_EXCEPTION;
    }

    exercizer_ctrl_event_t ev = {
        .type = EXERCIZER_CTRL_GPIO_SET,
        .data_len = sizeof(cfg),
    };
    memcpy(ev.data, &cfg, sizeof(cfg));
    if (!exercizer_control_send(&ev)) {
      return js_throw_ctrl(ctx, "gpio.setMany: control plane not ready");
    }
  }
  return JS_UNDEFINED;
}

static JSValue js_gpio_pulse(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 3) {
    return js_throw_ctrl(ctx, "gpio.pulse(pin, width_us, period_ms) requires 3 arguments");
  }
  int pin = 0;
  int width_us = 0;
  int period_ms = 0;
  if (JS_ToInt32(ctx, &pin, argv[0]) ||
      JS_ToInt32(ctx, &width_us, argv[1]) ||
      JS_ToInt32(ctx, &period_ms, argv[2])) {
    return JS_EXCEPTION;
  }
  if (width_us <= 0 || period_ms <= 0) {
    return JS_ThrowRangeError(ctx, "gpio.pulse: width_us and period_ms must be > 0");
  }

  exercizer_gpio_config_t cfg = {
      .pin = (uint8_t)pin,
      .mode = EXERCIZER_GPIO_MODE_PULSE,
      .hz = 0,
      .width_us = (uint32_t)width_us,
      .period_ms = (uint32_t)period_ms,
  };
  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_GPIO_SET,
      .data_len = sizeof(cfg),
  };
  memcpy(ev.data, &cfg, sizeof(cfg));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "gpio.pulse: control plane not ready");
  }
  return JS_UNDEFINED;
}

static JSValue js_gpio_stop(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc > 1) {
    return js_throw_ctrl(ctx, "gpio.stop([pin]) accepts at most 1 argument");
  }
  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_GPIO_STOP,
      .data_len = 0,
  };
  if (argc == 1) {
    int pin = 0;
    if (JS_ToInt32(ctx, &pin, argv[0])) {
      return JS_EXCEPTION;
    }
    exercizer_gpio_stop_t stop = {
        .pin = (uint8_t)pin,
    };
    ev.data_len = sizeof(stop);
    memcpy(ev.data, &stop, sizeof(stop));
  }
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "gpio.stop: control plane not ready");
  }
  return JS_UNDEFINED;
}

static JSValue js_i2c_config(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 5) {
    return js_throw_ctrl(ctx, "i2c.config(port, sda, scl, addr, hz) requires 5 arguments");
  }
  int port = 0;
  int sda = 0;
  int scl = 0;
  int addr = 0;
  int hz = 0;
  if (JS_ToInt32(ctx, &port, argv[0]) ||
      JS_ToInt32(ctx, &sda, argv[1]) ||
      JS_ToInt32(ctx, &scl, argv[2]) ||
      JS_ToInt32(ctx, &addr, argv[3]) ||
      JS_ToInt32(ctx, &hz, argv[4])) {
    return JS_EXCEPTION;
  }
  if (addr < 0 || addr > 0x7f) {
    return JS_ThrowRangeError(ctx, "i2c.config: addr must be 0..0x7f");
  }
  if (hz <= 0) {
    return JS_ThrowRangeError(ctx, "i2c.config: hz must be > 0");
  }

  exercizer_i2c_config_t cfg = {
      .port = (int8_t)port,
      .sda = (uint8_t)sda,
      .scl = (uint8_t)scl,
      .addr = (uint8_t)addr,
      .hz = (uint32_t)hz,
  };
  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_I2C_CONFIG,
      .data_len = sizeof(cfg),
  };
  memcpy(ev.data, &cfg, sizeof(cfg));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "i2c.config: control plane not ready");
  }
  return JS_UNDEFINED;
}

static JSValue js_i2c_tx(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 1) {
    return js_throw_ctrl(ctx, "i2c.tx(bytes) requires 1 argument");
  }
  exercizer_i2c_txn_t txn = {};
  size_t write_len = 0;
  if (!js_read_u8_array(ctx, argv[0], txn.write, EXERCIZER_I2C_WRITE_MAX, &write_len)) {
    return JS_EXCEPTION;
  }
  txn.write_len = (uint8_t)write_len;
  txn.read_len = 0;

  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_I2C_TXN,
      .data_len = sizeof(txn),
  };
  memcpy(ev.data, &txn, sizeof(txn));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "i2c.tx: control plane not ready");
  }
  return JS_UNDEFINED;
}

static JSValue js_i2c_txrx(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  (void)this_val;
  if (argc < 2) {
    return js_throw_ctrl(ctx, "i2c.txrx(bytes, readLen) requires 2 arguments");
  }
  exercizer_i2c_txn_t txn = {};
  size_t write_len = 0;
  if (!js_read_u8_array(ctx, argv[0], txn.write, EXERCIZER_I2C_WRITE_MAX, &write_len)) {
    return JS_EXCEPTION;
  }
  int read_len = 0;
  if (JS_ToInt32(ctx, &read_len, argv[1])) {
    return JS_EXCEPTION;
  }
  if (read_len < 0 || read_len > EXERCIZER_I2C_WRITE_MAX) {
    return JS_ThrowRangeError(ctx, "i2c.txrx: readLen must be 0..%u", EXERCIZER_I2C_WRITE_MAX);
  }
  txn.write_len = (uint8_t)write_len;
  txn.read_len = (uint8_t)read_len;

  exercizer_ctrl_event_t ev = {
      .type = EXERCIZER_CTRL_I2C_TXN,
      .data_len = sizeof(txn),
  };
  memcpy(ev.data, &txn, sizeof(txn));
  if (!exercizer_control_send(&ev)) {
    return js_throw_ctrl(ctx, "i2c.txrx: control plane not ready");
  }
  return JS_UNDEFINED;
}

#include "esp32_stdlib.h"
