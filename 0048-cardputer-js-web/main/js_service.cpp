#include "js_service.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sdkconfig.h"

static const char* TAG = "js_service_0048";

namespace {

struct EvalDeadline {
  int64_t deadline_us = 0;
};

struct EncoderDeltaSnap {
  int32_t pos = 0;
  int32_t delta = 0;
  uint32_t ts_ms = 0;
  uint32_t seq = 0;
};

struct EvalReq {
  char* code = nullptr;
  size_t code_len = 0;
  uint32_t timeout_ms = 0;
  const char* filename = "<eval>";

  SemaphoreHandle_t done = nullptr;
  char* reply_json = nullptr;
};

enum MsgType : uint8_t {
  MSG_EVAL_REQ = 1,
  MSG_ENCODER_CLICK = 2,
  MSG_ENCODER_DELTA = 3,
};

struct Msg {
  MsgType type = MSG_EVAL_REQ;
  void* ptr = nullptr;  // EvalReq*
  int32_t a = 0;
  int32_t b = 0;
};

static TaskHandle_t s_task = nullptr;
static QueueHandle_t s_q = nullptr;
static SemaphoreHandle_t s_start_mu = nullptr;

static uint8_t* s_js_mem = nullptr;
static JSContext* s_ctx = nullptr;
static EvalDeadline s_deadline;

static portMUX_TYPE s_enc_mu = portMUX_INITIALIZER_UNLOCKED;
static EncoderDeltaSnap s_enc_delta = {};
static bool s_enc_delta_pending = false;

int interrupt_handler(JSContext* ctx, void* opaque) {
  (void)ctx;
  auto* d = static_cast<EvalDeadline*>(opaque);
  if (!d || d->deadline_us == 0) return 0;
  return esp_timer_get_time() > d->deadline_us;
}

void write_to_string(void* opaque, const void* buf, size_t buf_len) {
  if (!opaque || !buf || buf_len == 0) return;
  auto* out = static_cast<std::string*>(opaque);
  out->append(static_cast<const char*>(buf), buf_len);
}

static void json_escape_append(std::string* out, const std::string& s) {
  for (char c : s) {
    if (c == '\\' || c == '\"') out->push_back('\\');
    if (c == '\n') {
      *out += "\\n";
    } else {
      out->push_back(c);
    }
  }
}

std::string print_value(JSContext* ctx, JSValue v) {
  std::string out;

  // MicroQuickJS uses ctx->opaque as the log sink AND as the interrupt opaque.
  // Keep interrupts working by restoring ctx->opaque after printing.
  JS_SetContextOpaque(ctx, &out);
  JS_SetLogFunc(ctx, write_to_string);
  JS_PrintValueF(ctx, v, JS_DUMP_LONG);
  JS_SetContextOpaque(ctx, &s_deadline);

  return out;
}

static void log_js_exception(const char* what) {
  JSValue exc = JS_GetException(s_ctx);
  const std::string err = print_value(s_ctx, exc);
  ESP_LOGW(TAG, "%s: %s", what ? what : "js exception", err.c_str());
}

static void install_bootstrap_phase2b(void) {
  // Keep this minimal: registration helpers only. No native bindings required.
  static const char* kBootstrap =
      "globalThis.__0048 = globalThis.__0048 || {};\n"
      "__0048.encoder = __0048.encoder || { on_delta: null, on_click: null };\n"
      "globalThis.encoder = globalThis.encoder || {};\n"
      "encoder.on = (t, fn) => {\n"
      "  if (t !== 'delta' && t !== 'click') throw new TypeError('encoder.on: type must be \"delta\" or \"click\"');\n"
      "  if (typeof fn !== 'function') throw new TypeError('encoder.on: fn must be a function');\n"
      "  if (t === 'delta') __0048.encoder.on_delta = fn;\n"
      "  if (t === 'click') __0048.encoder.on_click = fn;\n"
      "};\n"
      "encoder.off = (t) => {\n"
      "  if (t !== 'delta' && t !== 'click') throw new TypeError('encoder.off: type must be \"delta\" or \"click\"');\n"
      "  if (t === 'delta') __0048.encoder.on_delta = null;\n"
      "  if (t === 'click') __0048.encoder.on_click = null;\n"
      "};\n";

  const int flags = JS_EVAL_REPL;
  JSValue v = JS_Eval(s_ctx, kBootstrap, strlen(kBootstrap), "<boot:2b>", flags);
  if (JS_IsException(v)) {
    log_js_exception("bootstrap 2B failed");
  }
}

static JSValue get_encoder_cb(const char* which) {
  if (!which) return JS_UNDEFINED;
  JSValue glob = JS_GetGlobalObject(s_ctx);
  JSValue ns = JS_GetPropertyStr(s_ctx, glob, "__0048");
  if (JS_IsException(ns)) return ns;
  JSValue enc = JS_GetPropertyStr(s_ctx, ns, "encoder");
  if (JS_IsException(enc)) return enc;
  JSValue cb = JS_GetPropertyStr(s_ctx, enc, which);
  return cb;
}

static void call_cb(JSValue cb, JSValue arg0, uint32_t timeout_ms) {
  if (JS_IsUndefined(cb) || JS_IsNull(cb)) return;
  if (!JS_IsFunction(s_ctx, cb)) return;
  if (JS_StackCheck(s_ctx, 3)) return;

  if (timeout_ms > 0) {
    s_deadline.deadline_us = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
  } else {
    s_deadline.deadline_us = 0;
  }

  JS_PushArg(s_ctx, arg0);
  JS_PushArg(s_ctx, cb);
  JS_PushArg(s_ctx, JS_NULL);
  JSValue ret = JS_Call(s_ctx, 1);
  const bool threw = JS_IsException(ret);
  s_deadline.deadline_us = 0;
  if (threw) {
    log_js_exception("encoder callback threw");
  }
}

static void handle_encoder_delta(void) {
  EncoderDeltaSnap snap;
  taskENTER_CRITICAL(&s_enc_mu);
  snap = s_enc_delta;
  s_enc_delta_pending = false;
  taskEXIT_CRITICAL(&s_enc_mu);

  JSValue cb = get_encoder_cb("on_delta");
  if (JS_IsException(cb)) {
    log_js_exception("get on_delta failed");
    return;
  }

  JSValue ev = JS_NewObject(s_ctx);
  JS_SetPropertyStr(s_ctx, ev, "pos", JS_NewInt32(s_ctx, snap.pos));
  JS_SetPropertyStr(s_ctx, ev, "delta", JS_NewInt32(s_ctx, snap.delta));
  JS_SetPropertyStr(s_ctx, ev, "ts_ms", JS_NewUint32(s_ctx, snap.ts_ms));
  JS_SetPropertyStr(s_ctx, ev, "seq", JS_NewUint32(s_ctx, snap.seq));
  call_cb(cb, ev, 25);
}

static void handle_encoder_click(int kind) {
  JSValue cb = get_encoder_cb("on_click");
  if (JS_IsException(cb)) {
    log_js_exception("get on_click failed");
    return;
  }

  JSValue ev = JS_NewObject(s_ctx);
  JS_SetPropertyStr(s_ctx, ev, "kind", JS_NewInt32(s_ctx, kind));
  JS_SetPropertyStr(s_ctx, ev, "ts_ms", JS_NewUint32(s_ctx, (uint32_t)esp_log_timestamp()));
  call_cb(cb, ev, 25);
}

static esp_err_t ensure_ctx(void) {
  if (s_ctx) return ESP_OK;

  const int mem_bytes = CONFIG_TUTORIAL_0048_JS_MEM_BYTES;
  if (mem_bytes <= 0) return ESP_ERR_INVALID_ARG;

  s_js_mem = static_cast<uint8_t*>(malloc(static_cast<size_t>(mem_bytes)));
  if (!s_js_mem) return ESP_ERR_NO_MEM;
  memset(s_js_mem, 0, static_cast<size_t>(mem_bytes));

  s_ctx = JS_NewContext(s_js_mem, static_cast<size_t>(mem_bytes), &js_stdlib);
  if (!s_ctx) {
    free(s_js_mem);
    s_js_mem = nullptr;
    return ESP_FAIL;
  }

  JS_SetContextOpaque(s_ctx, &s_deadline);
  JS_SetInterruptHandler(s_ctx, interrupt_handler);

  install_bootstrap_phase2b();

  ESP_LOGI(TAG, "js initialized (mem=%d)", mem_bytes);
  return ESP_OK;
}

static std::string eval_to_json(const char* code, size_t code_len, uint32_t timeout_ms, const char* filename) {
  if (!code || code_len == 0) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"empty body\",\"timed_out\":false}";
  }

  const esp_err_t init_err = ensure_ctx();
  if (init_err != ESP_OK) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"js init failed\",\"timed_out\":false}";
  }

  if (!filename) filename = "<eval>";
  if (timeout_ms == 0) timeout_ms = CONFIG_TUTORIAL_0048_JS_TIMEOUT_MS;

  if (timeout_ms > 0) {
    s_deadline.deadline_us = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
  } else {
    s_deadline.deadline_us = 0;
  }

  const int flags = JS_EVAL_REPL | JS_EVAL_RETVAL;
  JSValue val = JS_Eval(s_ctx, code, code_len, filename, flags);

  const bool timed_out = (timeout_ms > 0) && (esp_timer_get_time() > s_deadline.deadline_us);
  s_deadline.deadline_us = 0;

  std::string json;
  if (JS_IsException(val)) {
    JSValue exc = JS_GetException(s_ctx);
    const std::string err = print_value(s_ctx, exc);
    json = "{\"ok\":false,\"output\":\"\",\"error\":\"";
    json_escape_append(&json, err);
    json += "\",\"timed_out\":";
    json += timed_out ? "true" : "false";
    json += "}";
    return json;
  }

  std::string out;
  if (!JS_IsUndefined(val)) {
    out = print_value(s_ctx, val);
    out.push_back('\n');
  }

  json = "{\"ok\":true,\"output\":\"";
  json_escape_append(&json, out);
  json += "\",\"error\":null,\"timed_out\":";
  json += timed_out ? "true" : "false";
  json += "}";
  return json;
}

static void service_task(void* arg) {
  (void)arg;
  ESP_LOGI(TAG, "task start");

  for (;;) {
    Msg msg = {};
    if (xQueueReceive(s_q, &msg, portMAX_DELAY) != pdTRUE) continue;

    if (msg.type == MSG_EVAL_REQ) {
      auto* req = static_cast<EvalReq*>(msg.ptr);
      if (!req) continue;
      const std::string json = eval_to_json(req->code, req->code_len, req->timeout_ms, req->filename);
      req->reply_json = static_cast<char*>(malloc(json.size() + 1));
      if (req->reply_json) {
        memcpy(req->reply_json, json.data(), json.size());
        req->reply_json[json.size()] = 0;
      }
      free(req->code);
      req->code = nullptr;
      xSemaphoreGive(req->done);
      continue;
    }

    const esp_err_t init_err = ensure_ctx();
    if (init_err != ESP_OK) continue;

    if (msg.type == MSG_ENCODER_DELTA) {
      handle_encoder_delta();
      continue;
    }

    if (msg.type == MSG_ENCODER_CLICK) {
      handle_encoder_click(msg.a);
      continue;
    }
  }
}

}  // namespace

esp_err_t js_service_start(void) {
  if (!s_start_mu) s_start_mu = xSemaphoreCreateMutex();
  if (!s_start_mu) return ESP_ERR_NO_MEM;

  xSemaphoreTake(s_start_mu, portMAX_DELAY);
  if (s_task) {
    xSemaphoreGive(s_start_mu);
    return ESP_OK;
  }

  s_q = xQueueCreate(16, sizeof(Msg));
  if (!s_q) {
    xSemaphoreGive(s_start_mu);
    return ESP_ERR_NO_MEM;
  }

  BaseType_t ok = xTaskCreate(&service_task, "js_svc", 6144, nullptr, 8, &s_task);
  if (ok != pdPASS) {
    vQueueDelete(s_q);
    s_q = nullptr;
    s_task = nullptr;
    xSemaphoreGive(s_start_mu);
    return ESP_ERR_NO_MEM;
  }

  xSemaphoreGive(s_start_mu);
  return ESP_OK;
}

std::string js_service_eval_to_json(const char* code, size_t code_len, uint32_t timeout_ms, const char* filename) {
  const esp_err_t err = js_service_start();
  if (err != ESP_OK) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"js service unavailable\",\"timed_out\":false}";
  }

  if (!code || code_len == 0) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"empty body\",\"timed_out\":false}";
  }

  EvalReq req = {};
  req.timeout_ms = timeout_ms;
  req.filename = filename ? filename : "<eval>";
  req.done = xSemaphoreCreateBinary();
  if (!req.done) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"oom\",\"timed_out\":false}";
  }

  req.code = static_cast<char*>(malloc(code_len + 1));
  if (!req.code) {
    vSemaphoreDelete(req.done);
    return "{\"ok\":false,\"output\":\"\",\"error\":\"oom\",\"timed_out\":false}";
  }
  memcpy(req.code, code, code_len);
  req.code[code_len] = 0;
  req.code_len = code_len;

  Msg msg = {};
  msg.type = MSG_EVAL_REQ;
  msg.ptr = &req;
  if (xQueueSend(s_q, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
    free(req.code);
    vSemaphoreDelete(req.done);
    return "{\"ok\":false,\"output\":\"\",\"error\":\"busy\",\"timed_out\":false}";
  }

  // We rely on the VM-level timeout; the service must always respond to avoid UAF on `req`.
  xSemaphoreTake(req.done, portMAX_DELAY);

  std::string out = req.reply_json ? req.reply_json : "{\"ok\":false,\"output\":\"\",\"error\":\"oom\",\"timed_out\":false}";
  if (req.reply_json) free(req.reply_json);
  vSemaphoreDelete(req.done);
  return out;
}

esp_err_t js_service_post_encoder_click(int kind) {
  const esp_err_t err = js_service_start();
  if (err != ESP_OK) return err;
  Msg msg = {};
  msg.type = MSG_ENCODER_CLICK;
  msg.a = kind;
  if (xQueueSend(s_q, &msg, 0) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}

esp_err_t js_service_update_encoder_delta(int32_t pos, int32_t delta) {
  const esp_err_t err = js_service_start();
  if (err != ESP_OK) return err;

  bool need_wakeup = false;
  taskENTER_CRITICAL(&s_enc_mu);
  s_enc_delta.pos = pos;
  s_enc_delta.delta = delta;
  s_enc_delta.ts_ms = (uint32_t)esp_log_timestamp();
  s_enc_delta.seq++;
  if (!s_enc_delta_pending) {
    s_enc_delta_pending = true;
    need_wakeup = true;
  }
  taskEXIT_CRITICAL(&s_enc_mu);

  if (need_wakeup) {
    Msg msg = {};
    msg.type = MSG_ENCODER_DELTA;
    if (xQueueSend(s_q, &msg, 0) != pdTRUE) {
      taskENTER_CRITICAL(&s_enc_mu);
      s_enc_delta_pending = false;
      taskEXIT_CRITICAL(&s_enc_mu);
      return ESP_ERR_TIMEOUT;
    }
  }
  return ESP_OK;
}
