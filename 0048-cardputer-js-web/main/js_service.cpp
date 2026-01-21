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
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sdkconfig.h"

// Phase 2C: WS broadcast helper.
#include "http_server.h"

#include "mqjs_service.h"
#include "mqjs_vm.h"

static const char* TAG = "js_service_0048";

namespace {

struct EncoderDeltaSnap {
  int32_t pos = 0;
  int32_t delta = 0;
  uint32_t ts_ms = 0;
  uint32_t seq = 0;
};

struct ClickArg {
  int kind = 0;
};

static SemaphoreHandle_t s_start_mu = nullptr;
static mqjs_service_t* s_svc = nullptr;

static portMUX_TYPE s_enc_mu = portMUX_INITIALIZER_UNLOCKED;
static EncoderDeltaSnap s_enc_delta = {};
static bool s_enc_delta_pending = false;

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

static void log_js_exception(JSContext* ctx, const char* what) {
  if (!ctx) return;
  MqjsVm* vm = MqjsVm::From(ctx);
  const std::string err = vm ? vm->GetExceptionString(JS_DUMP_LONG) : "<exception>";
  ESP_LOGW(TAG, "%s: %s", what ? what : "js exception", err.c_str());
}

static void flush_js_events_to_ws(JSContext* ctx, const char* source) {
  if (!ctx) return;

  MqjsVm* vm = MqjsVm::From(ctx);
  if (!vm) return;

  const char* src = source ? source : "eval";
  char expr[96];
  snprintf(expr, sizeof(expr), "__0048_take_lines('%s')", src);

  vm->SetDeadlineMs(50);
  JSValue v = JS_Eval(ctx, expr, strlen(expr), "<flush>", JS_EVAL_REPL | JS_EVAL_RETVAL);
  vm->ClearDeadline();

  if (JS_IsException(v)) {
    log_js_exception(ctx, "flush js events failed");
    return;
  }

  if (!JS_IsString(ctx, v)) return;

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t n = 0;
  const char* s = JS_ToCStringLen(ctx, &n, v, &sbuf);
  if (!s || n == 0) return;

  const char* p = s;
  size_t remaining = n;
  while (remaining > 0) {
    const void* nl = memchr(p, '\n', remaining);
    const size_t len = nl ? (size_t)((const char*)nl - p) : remaining;
    if (len > 0) {
      std::string line(p, len);
      (void)http_server_ws_broadcast_text(line.c_str());
    }
    if (!nl) break;
    p += len + 1;
    remaining -= len + 1;
  }
}

static JSValue get_encoder_cb(JSContext* ctx, const char* which) {
  if (!ctx || !which) return JS_UNDEFINED;
  JSValue glob = JS_GetGlobalObject(ctx);
  JSValue ns = JS_GetPropertyStr(ctx, glob, "__0048");
  if (JS_IsException(ns)) return ns;
  JSValue enc = JS_GetPropertyStr(ctx, ns, "encoder");
  if (JS_IsException(enc)) return enc;
  return JS_GetPropertyStr(ctx, enc, which);
}

static void call_cb(JSContext* ctx, JSValue cb, JSValue arg0, uint32_t timeout_ms) {
  if (!ctx) return;
  if (JS_IsUndefined(cb) || JS_IsNull(cb)) return;
  if (!JS_IsFunction(ctx, cb)) return;
  if (JS_StackCheck(ctx, 3)) return;

  MqjsVm* vm = MqjsVm::From(ctx);
  if (vm) vm->SetDeadlineMs(timeout_ms);

  JS_PushArg(ctx, arg0);
  JS_PushArg(ctx, cb);
  JS_PushArg(ctx, JS_NULL);
  JSValue ret = JS_Call(ctx, 1);
  const bool threw = JS_IsException(ret);

  if (vm) vm->ClearDeadline();

  if (threw) {
    log_js_exception(ctx, "encoder callback threw");
  }
}

static esp_err_t job_bootstrap(JSContext* ctx, void* user) {
  (void)user;

  static const char* kBootstrap2b =
      // NOTE: MicroQuickJS stdlib in this repo doesn't support arrow functions; keep ES5 syntax.
      "var g = globalThis;\n"
      "g.__0048 = g.__0048 || {};\n"
      "var __0048 = g.__0048;\n"
      "__0048.encoder = __0048.encoder || { on_delta: null, on_click: null };\n"
      "g.encoder = g.encoder || {};\n"
      "var encoder = g.encoder;\n"
      "encoder.on = function(t, fn) {\n"
      "  if (t !== 'delta' && t !== 'click') throw new TypeError('encoder.on: type must be \"delta\" or \"click\"');\n"
      "  if (typeof fn !== 'function') throw new TypeError('encoder.on: fn must be a function');\n"
      "  if (t === 'delta') __0048.encoder.on_delta = fn;\n"
      "  if (t === 'click') __0048.encoder.on_click = fn;\n"
      "};\n"
      "encoder.off = function(t) {\n"
      "  if (t !== 'delta' && t !== 'click') throw new TypeError('encoder.off: type must be \"delta\" or \"click\"');\n"
      "  if (t === 'delta') __0048.encoder.on_delta = null;\n"
      "  if (t === 'click') __0048.encoder.on_click = null;\n"
      "};\n";

  static const char* kBootstrap2c =
      // NOTE: MicroQuickJS stdlib in this repo doesn't support arrow functions; keep ES5 syntax.
      "var g = globalThis;\n"
      "g.__0048 = g.__0048 || {};\n"
      "var __0048 = g.__0048;\n"
      "__0048.maxEvents = __0048.maxEvents || 64;\n"
      "__0048.events = __0048.events || [];\n"
      "__0048.dropped = __0048.dropped || 0;\n"
      "__0048.ws_seq = __0048.ws_seq || 0;\n"
      "g.emit = function(topic, payload) {\n"
      "  if (typeof topic !== 'string' || topic.length === 0) throw new TypeError('emit: topic must be non-empty string');\n"
      "  if (__0048.events.length >= __0048.maxEvents) { __0048.dropped = (__0048.dropped || 0) + 1; return; }\n"
      "  __0048.events.push({ topic: topic, payload: payload, ts_ms: Date.now() });\n"
      "};\n"
      "g.__0048_take_lines = function(source) {\n"
      "  var dropped = __0048.dropped || 0;\n"
      "  var ev = __0048.events || [];\n"
      "  if (ev.length === 0 && dropped === 0) return '';\n"
      "  __0048.events = [];\n"
      "  __0048.dropped = 0;\n"
      "  var src = String(source || 'eval');\n"
      "  var s = JSON.stringify({ type: 'js_events', seq: (__0048.ws_seq++), ts_ms: Date.now(), source: src, events: ev });\n"
      "  if (dropped) {\n"
      "    s += '\\n' + JSON.stringify({ type: 'js_events_dropped', seq: (__0048.ws_seq++), ts_ms: Date.now(), source: src, dropped: dropped });\n"
      "  }\n"
      "  return s;\n"
      "};\n";

  const int flags = JS_EVAL_REPL;
  JSValue v = JS_Eval(ctx, kBootstrap2b, strlen(kBootstrap2b), "<boot:2b>", flags);
  if (JS_IsException(v)) {
    log_js_exception(ctx, "bootstrap 2B failed");
  }
  v = JS_Eval(ctx, kBootstrap2c, strlen(kBootstrap2c), "<boot:2c>", flags);
  if (JS_IsException(v)) {
    log_js_exception(ctx, "bootstrap 2C failed");
  }

  return ESP_OK;
}

static esp_err_t job_handle_encoder_delta(JSContext* ctx, void* user) {
  (void)user;

  EncoderDeltaSnap snap;
  taskENTER_CRITICAL(&s_enc_mu);
  snap = s_enc_delta;
  s_enc_delta_pending = false;
  taskEXIT_CRITICAL(&s_enc_mu);

  JSValue cb = get_encoder_cb(ctx, "on_delta");
  if (JS_IsException(cb)) {
    log_js_exception(ctx, "get on_delta failed");
    return ESP_OK;
  }

  JSValue ev = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, ev, "pos", JS_NewInt32(ctx, snap.pos));
  JS_SetPropertyStr(ctx, ev, "delta", JS_NewInt32(ctx, snap.delta));
  JS_SetPropertyStr(ctx, ev, "ts_ms", JS_NewUint32(ctx, snap.ts_ms));
  JS_SetPropertyStr(ctx, ev, "seq", JS_NewUint32(ctx, snap.seq));
  call_cb(ctx, cb, ev, 25);

  flush_js_events_to_ws(ctx, "callback");
  return ESP_OK;
}

static esp_err_t job_handle_encoder_click(JSContext* ctx, void* user) {
  int kind = 0;
  if (user) {
    auto* a = static_cast<ClickArg*>(user);
    kind = a->kind;
    free(a);
  }

  JSValue cb = get_encoder_cb(ctx, "on_click");
  if (JS_IsException(cb)) {
    log_js_exception(ctx, "get on_click failed");
    return ESP_OK;
  }

  JSValue ev = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, ev, "kind", JS_NewInt32(ctx, kind));
  JS_SetPropertyStr(ctx, ev, "ts_ms", JS_NewUint32(ctx, (uint32_t)esp_log_timestamp()));
  call_cb(ctx, cb, ev, 25);

  flush_js_events_to_ws(ctx, "callback");
  return ESP_OK;
}

static esp_err_t job_flush_eval(JSContext* ctx, void* user) {
  (void)user;
  flush_js_events_to_ws(ctx, "eval");
  return ESP_OK;
}

}  // namespace

esp_err_t js_service_start(void) {
  if (!s_start_mu) s_start_mu = xSemaphoreCreateMutex();
  if (!s_start_mu) return ESP_ERR_NO_MEM;

  xSemaphoreTake(s_start_mu, portMAX_DELAY);
  if (s_svc) {
    xSemaphoreGive(s_start_mu);
    return ESP_OK;
  }

  mqjs_service_config_t cfg = {};
  cfg.task_name = "js_svc";
  cfg.task_stack_words = 6144 / 4;
  cfg.task_priority = 8;
  cfg.task_core_id = -1;
  cfg.queue_len = 16;
  cfg.arena_bytes = (size_t)CONFIG_TUTORIAL_0048_JS_MEM_BYTES;
  cfg.stdlib = &js_stdlib;
  cfg.fix_global_this = true;

  esp_err_t err = mqjs_service_start(&cfg, &s_svc);
  if (err != ESP_OK) {
    s_svc = nullptr;
    xSemaphoreGive(s_start_mu);
    return err;
  }

  const mqjs_job_t boot = {.fn = &job_bootstrap, .user = nullptr, .timeout_ms = 250};
  (void)mqjs_service_run(s_svc, &boot);

  ESP_LOGI(TAG, "js service started (mem=%u)", (unsigned)cfg.arena_bytes);

  xSemaphoreGive(s_start_mu);
  return ESP_OK;
}

std::string js_service_eval_to_json(const char* code, size_t code_len, uint32_t timeout_ms, const char* filename) {
  const esp_err_t err = js_service_start();
  if (err != ESP_OK || !s_svc) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"js service unavailable\",\"timed_out\":false}";
  }

  if (!code || code_len == 0) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"empty body\",\"timed_out\":false}";
  }

  const uint32_t effective_timeout_ms =
      (timeout_ms == 0) ? (uint32_t)CONFIG_TUTORIAL_0048_JS_TIMEOUT_MS : timeout_ms;

  mqjs_eval_result_t r = {};
  const esp_err_t st = mqjs_service_eval(s_svc,
                                        code,
                                        code_len,
                                        effective_timeout_ms,
                                        filename ? filename : "<eval>",
                                        &r);
  if (st != ESP_OK) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"busy\",\"timed_out\":false}";
  }

  std::string json;
  if (!r.ok) {
    json = "{\"ok\":false,\"output\":\"";
    json_escape_append(&json, r.output ? r.output : "");
    json += "\",\"error\":\"";
    json_escape_append(&json, r.error ? r.error : "error");
    json += "\",\"timed_out\":";
    json += r.timed_out ? "true" : "false";
    json += "}";
  } else {
    json = "{\"ok\":true,\"output\":\"";
    json_escape_append(&json, r.output ? r.output : "");
    json += "\",\"error\":null,\"timed_out\":";
    json += r.timed_out ? "true" : "false";
    json += "}";
  }

  mqjs_eval_result_free(&r);

  const mqjs_job_t flush = {.fn = &job_flush_eval, .user = nullptr, .timeout_ms = 0};
  (void)mqjs_service_post(s_svc, &flush);

  return json;
}

esp_err_t js_service_post_encoder_click(int kind) {
  const esp_err_t err = js_service_start();
  if (err != ESP_OK || !s_svc) return err;

  auto* a = static_cast<ClickArg*>(malloc(sizeof(ClickArg)));
  if (!a) return ESP_ERR_NO_MEM;
  a->kind = kind;

  const mqjs_job_t job = {.fn = &job_handle_encoder_click, .user = a, .timeout_ms = 0};
  esp_err_t st = mqjs_service_post(s_svc, &job);
  if (st != ESP_OK) {
    free(a);
  }
  return st;
}

esp_err_t js_service_update_encoder_delta(int32_t pos, int32_t delta) {
  const esp_err_t err = js_service_start();
  if (err != ESP_OK || !s_svc) return err;

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

  if (!need_wakeup) return ESP_OK;

  const mqjs_job_t job = {.fn = &job_handle_encoder_delta, .user = nullptr, .timeout_ms = 0};
  esp_err_t st = mqjs_service_post(s_svc, &job);
  if (st != ESP_OK) {
    taskENTER_CRITICAL(&s_enc_mu);
    s_enc_delta_pending = false;
    taskEXIT_CRITICAL(&s_enc_mu);
  }
  return st;
}
