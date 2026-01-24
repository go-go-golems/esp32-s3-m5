#include "js_service.h"

#include <stdio.h>
#include <string.h>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include <string>

#include "esp_log.h"

#include "sdkconfig.h"

#include "http_server.h"

#include "mqjs_service.h"
#include "mqjs_vm.h"

#include "mqjs_timers.h"

// Provided by `mqjs/esp32_stdlib_runtime.c`.
extern "C" void mqjs_sim_set_engine(sim_engine_t* engine);

static const char* TAG = "0066_js_service";

static mqjs_service_t* s_svc = nullptr;
static sim_engine_t* s_engine = nullptr;

static uint32_t js_eval_timeout_ms(void) {
  // Conservative default for interactive console. Callers can pass a different timeout if desired.
  return 250;
}

namespace {

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
  snprintf(expr, sizeof(expr), "__0066_take_lines('%s')", src);

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

static esp_err_t job_bootstrap(JSContext* ctx, void* user) {
  (void)user;

  static const char* kBootstrap =
      // NOTE: MicroQuickJS stdlib in this repo doesn't support arrow functions; keep ES5 syntax.
      "var g = globalThis;\n"
      "g.__0066 = g.__0066 || {};\n"
      "var __0066 = g.__0066;\n"
      "__0066.timers = __0066.timers || {};\n"
      "__0066.timers.cb = __0066.timers.cb || {};\n"
      "__0066.gpio = __0066.gpio || {};\n"
      "__0066.gpio.state = __0066.gpio.state || { G3: 0, G4: 0 };\n"
      "__0066.maxEvents = __0066.maxEvents || 64;\n"
      "__0066.events = __0066.events || [];\n"
      "__0066.dropped = __0066.dropped || 0;\n"
      "__0066.ws_seq = __0066.ws_seq || 0;\n"
      "g.gpio = g.gpio || {};\n"
      "gpio.write = function(label, v) {\n"
      "  var k = String(label);\n"
      "  if (k !== 'G3' && k !== 'G4' && k !== 'g3' && k !== 'g4') throw new TypeError('gpio.write: label must be G3 or G4');\n"
      "  k = (k[0] === 'g') ? ('G' + k[1]) : k;\n"
      "  v = v ? 1 : 0;\n"
      "  __0066.gpio.state[k] = v;\n"
      "  if (v) gpio.high(k); else gpio.low(k);\n"
      "  return v;\n"
      "};\n"
      "gpio.toggle = function(label) {\n"
      "  var k = String(label);\n"
      "  if (k !== 'G3' && k !== 'G4' && k !== 'g3' && k !== 'g4') throw new TypeError('gpio.toggle: label must be G3 or G4');\n"
      "  k = (k[0] === 'g') ? ('G' + k[1]) : k;\n"
      "  var v = (__0066.gpio.state[k] ? 0 : 1);\n"
      "  return gpio.write(k, v);\n"
      "};\n"
      "g.emit = function(topic, payload) {\n"
      "  if (typeof topic !== 'string' || topic.length === 0) throw new TypeError('emit: topic must be non-empty string');\n"
      "  if (__0066.events.length >= __0066.maxEvents) { __0066.dropped = (__0066.dropped || 0) + 1; return; }\n"
      "  __0066.events.push({ topic: topic, payload: payload, ts_ms: Date.now() });\n"
      "};\n"
      "g.__0066_take_lines = function(source) {\n"
      "  var dropped = __0066.dropped || 0;\n"
      "  var ev = __0066.events || [];\n"
      "  if (ev.length === 0 && dropped === 0) return '';\n"
      "  __0066.events = [];\n"
      "  __0066.dropped = 0;\n"
      "  var src = String(source || 'eval');\n"
      "  var s = JSON.stringify({ type: 'js_events', seq: (__0066.ws_seq++), ts_ms: Date.now(), source: src, events: ev });\n"
      "  if (dropped) {\n"
      "    s += '\\n' + JSON.stringify({ type: 'js_events_dropped', seq: (__0066.ws_seq++), ts_ms: Date.now(), source: src, dropped: dropped });\n"
      "  }\n"
      "  return s;\n"
      "};\n"
      "if (typeof sim === 'object' && sim) {\n"
      "  sim.statusJson = function () {\n"
      "    if (typeof JSON === 'undefined' || !JSON || typeof JSON.stringify !== 'function') {\n"
      "      throw new Error('sim.statusJson: JSON.stringify not available');\n"
      "    }\n"
      "    return JSON.stringify(sim.status());\n"
      "  };\n"
      "}\n"
      "g.cancel = function(h) {\n"
      "  if (typeof h === 'number') { clearTimeout(h); return; }\n"
      "  if (h && typeof h.cancel === 'function') { h.cancel(); return; }\n"
      "  throw new TypeError('cancel(handle): handle must be a number or {cancel()}');\n"
      "};\n"
      "g.every = function(ms, fn) {\n"
      "  ms = ms|0;\n"
      "  if (ms < 0) throw new RangeError('every: ms must be >= 0');\n"
      "  if (typeof fn !== 'function') throw new TypeError('every: fn must be a function');\n"
      "  var h = { id: 0, cancelled: false };\n"
      "  h.cancel = function() {\n"
      "    if (h.cancelled) return;\n"
      "    h.cancelled = true;\n"
      "    if (h.id) clearTimeout(h.id);\n"
      "  };\n"
      "  function tick() {\n"
      "    if (h.cancelled) return;\n"
      "    try { fn(); } catch (e) { h.cancel(); throw e; }\n"
      "    if (h.cancelled) return;\n"
      "    h.id = setTimeout(tick, ms);\n"
      "  }\n"
      "  h.id = setTimeout(tick, ms);\n"
      "  return h;\n"
      "};\n";

  const int flags = JS_EVAL_REPL;
  JSValue v = JS_Eval(ctx, kBootstrap, strlen(kBootstrap), "<boot:0066>", flags);
  if (JS_IsException(v)) {
    log_js_exception(ctx, "bootstrap failed");
  }
  return ESP_OK;
}

static esp_err_t job_flush_ws(JSContext* ctx, void* user) {
  const char* src = user ? static_cast<const char*>(user) : "eval";
  flush_js_events_to_ws(ctx, src);
  return ESP_OK;
}

}  // namespace

esp_err_t js_service_start(sim_engine_t* engine) {
  if (s_svc) return ESP_OK;
  if (!engine) return ESP_ERR_INVALID_ARG;

  s_engine = engine;
  mqjs_sim_set_engine(engine);

  mqjs_service_config_t cfg = {};
  cfg.task_name = "0066_js";
  cfg.task_stack_words = 6144;
  cfg.task_priority = 8;
  cfg.task_core_id = -1;
  cfg.queue_len = 16;
  cfg.arena_bytes = CONFIG_TUTORIAL_0066_JS_MEM_BYTES;
  cfg.stdlib = &js_stdlib;
  cfg.fix_global_this = true;

  esp_err_t st = mqjs_service_start(&cfg, &s_svc);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "mqjs_service_start failed: %s", esp_err_to_name(st));
    s_svc = nullptr;
    return st;
  }

  st = mqjs_0066_timers_start(s_svc);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "mqjs timers start failed: %s", esp_err_to_name(st));
    js_service_stop();
    return st;
  }

  mqjs_job_t boot = {};
  boot.fn = &job_bootstrap;
  boot.user = nullptr;
  boot.timeout_ms = 200;
  (void)mqjs_service_run(s_svc, &boot);

  return ESP_OK;
}

void js_service_stop(void) {
  mqjs_0066_timers_stop();
  if (!s_svc) return;
  mqjs_service_stop(s_svc);
  s_svc = nullptr;
}

esp_err_t js_service_reset(sim_engine_t* engine) {
  js_service_stop();
  return js_service_start(engine ? engine : s_engine);
}

esp_err_t js_service_eval(const char* code,
                          size_t code_len,
                          uint32_t timeout_ms,
                          const char* filename,
                          std::string* out,
                          std::string* err) {
  if (out) out->clear();
  if (err) err->clear();
  if (!s_svc) return ESP_ERR_INVALID_STATE;
  if (!code || code_len == 0) return ESP_ERR_INVALID_ARG;

  mqjs_eval_result_t r = {};
  const uint32_t tmo = (timeout_ms == 0) ? js_eval_timeout_ms() : timeout_ms;
  const esp_err_t st = mqjs_service_eval(s_svc, code, code_len, tmo, filename ? filename : "<eval>", &r);

  if (st != ESP_OK) {
    mqjs_eval_result_free(&r);
    return st;
  }

  if (r.error && err) {
    err->assign(r.error);
    if (!err->empty() && (*err)[err->size() - 1] != '\n') err->push_back('\n');
  }
  if (r.output && out) {
    out->assign(r.output);
    if (!out->empty() && (*out)[out->size() - 1] != '\n') out->push_back('\n');
  }

  mqjs_eval_result_free(&r);
  return ESP_OK;
}

namespace {

struct DumpArg {
  std::string* out = nullptr;
};

static esp_err_t job_dump_memory(JSContext* ctx, void* user) {
  auto* a = static_cast<DumpArg*>(user);
  if (!a || !a->out) return ESP_ERR_INVALID_ARG;
  MqjsVm* vm = MqjsVm::From(ctx);
  if (!vm) return ESP_FAIL;
  *(a->out) = vm->DumpMemory(false);
  if (!a->out->empty() && (*a->out)[a->out->size() - 1] != '\n') a->out->push_back('\n');
  return ESP_OK;
}

}  // namespace

esp_err_t js_service_dump_memory(std::string* out) {
  if (!out) return ESP_ERR_INVALID_ARG;
  out->clear();
  if (!s_svc) return ESP_ERR_INVALID_STATE;

  DumpArg a = {.out = out};
  mqjs_job_t job = {};
  job.fn = &job_dump_memory;
  job.user = &a;
  job.timeout_ms = 100;
  return mqjs_service_run(s_svc, &job);
}

std::string js_service_eval_to_json(const char* code,
                                   size_t code_len,
                                   uint32_t timeout_ms,
                                   const char* filename) {
  if (!s_svc) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"js service unavailable\",\"timed_out\":false}";
  }

  if (!code || code_len == 0) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"empty body\",\"timed_out\":false}";
  }

  const uint32_t tmo = (timeout_ms == 0) ? js_eval_timeout_ms() : timeout_ms;
  mqjs_eval_result_t r = {};
  const esp_err_t st = mqjs_service_eval(s_svc, code, code_len, tmo, filename ? filename : "<eval>", &r);
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

  const mqjs_job_t flush = {.fn = &job_flush_ws, .user = (void*)"eval", .timeout_ms = 0};
  (void)mqjs_service_post(s_svc, &flush);

  return json;
}

extern "C" void mqjs_0066_after_js_callback(JSContext* ctx, const char* source) {
  flush_js_events_to_ws(ctx, source ? source : "callback");
}
