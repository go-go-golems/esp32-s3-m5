#include "js_runtime_bridge.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#include "sdkconfig.h"

extern "C" {
#include "matrix_engine.h"
}

#include "mqjs_service.h"
#include "mqjs_vm.h"

#include "mqjs_timers.h"

extern "C" void mqjs_0067_runtime_request_stop(void);
extern "C" void mqjs_0067_runtime_clear_stop(void);

static const char* TAG = "0067_js_service";

static mqjs_service_t* s_svc = nullptr;
static SemaphoreHandle_t s_mu = nullptr;
static js_service_status_t s_status = {};

namespace {

static void lock_mu() {
  if (s_mu) xSemaphoreTake(s_mu, portMAX_DELAY);
}

static void unlock_mu() {
  if (s_mu) xSemaphoreGive(s_mu);
}

static char* dup_cstr(const std::string& s) {
  char* out = static_cast<char*>(malloc(s.size() + 1));
  if (!out) return nullptr;
  memcpy(out, s.data(), s.size());
  out[s.size()] = 0;
  return out;
}

static void json_escape_append(std::string* out, const char* s) {
  if (!out || !s) return;
  for (const char* p = s; *p; ++p) {
    const char c = *p;
    if (c == '\\' || c == '"') out->push_back('\\');
    if (c == '\n') {
      out->append("\\n");
    } else if (c == '\r') {
      out->append("\\r");
    } else {
      out->push_back(c);
    }
  }
}

static void set_last_error_locked(const char* s) {
  memset(s_status.last_error, 0, sizeof(s_status.last_error));
  if (s && *s) strlcpy(s_status.last_error, s, sizeof(s_status.last_error));
}

static esp_err_t job_bootstrap(JSContext* ctx, void* user) {
  (void)user;

  static const char* kBootstrap =
      "var g = globalThis;\n"
      "g.__0067 = g.__0067 || {};\n"
      "var __0067 = g.__0067;\n"
      "__0067.timers = __0067.timers || {};\n"
      "__0067.timers.cb = __0067.timers.cb || {};\n"
      "g.cancel = function(h) {\n"
      "  if (typeof h === 'number') { clearTimeout(h); return; }\n"
      "  if (h && typeof h.cancel === 'function') { h.cancel(); return; }\n"
      "  throw new TypeError('cancel(handle): handle must be a number or {cancel()}');\n"
      "};\n"
      "g.every = function(ms, fn) {\n"
      "  ms = ms | 0;\n"
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
      "    try { fn(); } catch (__e_tick) { h.cancel(); throw __e_tick; }\n"
      "    if (h.cancelled) return;\n"
      "    h.id = setTimeout(tick, ms);\n"
      "  }\n"
      "  h.id = setTimeout(tick, ms);\n"
      "  return h;\n"
      "};\n"
      "g.matrix = g.matrix || {};\n"
      "var m = g.matrix;\n"
      "m.width = function() { return i2c.txrx({op:'dim'}, 'w'); };\n"
      "m.height = function() { return i2c.txrx({op:'dim'}, 'h'); };\n"
      "m.clear = function() { return !!i2c.tx({op:'clear'}); };\n"
      "m.fill = function(on) { return !!i2c.tx({op:'fill', on:(on ? 1 : 0)}); };\n"
      "m.setPixel = function(x, y, on) { return !!i2c.tx({op:'setPixel', x:(x|0), y:(y|0), on:(on ? 1 : 0)}); };\n"
      "m.getPixel = function(x, y) { return !!i2c.txrx({op:'getPixel', x:(x|0), y:(y|0)}); };\n"
      "m.present = function() { return !!i2c.tx({op:'present'}); };\n"
      "m.setText = function(text) { return !!i2c.tx({op:'text', text:String(text || '')}); };\n"
      "m.startScroll = function(text, opts) {\n"
      "  opts = opts || {};\n"
      "  return !!i2c.tx({op:'scroll', text:String(text || ''), fps:(opts.fps|0), pause_ms:(opts.pauseMs|0), repeat:(opts.repeat|0), wave:(opts.wave ? 1 : 0), loop_mode:String(opts.loopMode || 'gap')});\n"
      "};\n"
      "m.startDrop = function(text, opts) {\n"
      "  opts = opts || {};\n"
      "  return !!i2c.tx({op:'drop', text:String(text || ''), fps:(opts.fps|0), pause_ms:(opts.pauseMs|0), repeat:(opts.repeat|0)});\n"
      "};\n"
      "m._stopRaw = function() { return !!i2c.tx({op:'stop'}); };\n"
      "m.setIntensity = function(v) { return !!i2c.tx({op:'intensity', value:(v|0)}); };\n"
      "m.setOrientation = function(reverse, flipv) { return !!i2c.tx({op:'orientation', reverse:(reverse ? 1 : 0), flipv:(flipv ? 1 : 0)}); };\n"
      "m.setRotate180 = function(on) { return !!i2c.tx({op:'rotate180', on:(on ? 1 : 0)}); };\n"
      "m.status = function() { return i2c.txrx({op:'status'}); };\n"
      "m.statusJson = function() { return JSON.stringify(m.status()); };\n"
      "m.nowMs = function() { return i2c.txrx({op:'nowMs'}); };\n"
      "m.nowUs = function() { return i2c.txrx({op:'nowUs'}); };\n"
      "m.sleepMs = function(ms) { return !!i2c.tx({op:'sleepMs', ms:(ms|0)}); };\n"
      "m.sleepUntilUs = function(us) { return !!i2c.tx({op:'sleepUntilUs', us:Number(us)}); };\n"
      "m.shouldStop = function() { return !!i2c.txrx({op:'shouldStop'}); };\n"
      "m.anim = m.anim || {};\n"
      "var A = m.anim;\n"
      "A._registry = A._registry || {};\n"
      "A._current = A._current || null;\n"
      "A._cleanupCurrent = function() {\n"
      "  var c = A._current;\n"
      "  if (!c) return;\n"
      "  A._current = null;\n"
      "  if (c.handles) {\n"
      "    for (var i = 0; i < c.handles.length; i++) {\n"
      "      try { cancel(c.handles[i]); } catch (__e_cancel) {}\n"
      "    }\n"
      "  }\n"
      "  if (c.cleanups) {\n"
      "    for (var j = c.cleanups.length - 1; j >= 0; j--) {\n"
      "      try { c.cleanups[j](); } catch (__e_cleanup) { try { print('anim cleanup error', String(__e_cleanup)); } catch (__e_print) {} }\n"
      "    }\n"
      "  }\n"
      "};\n"
      "A.stop = function() {\n"
      "  A._cleanupCurrent();\n"
      "  return m._stopRaw();\n"
      "};\n"
      "A.register = function(name, spec) {\n"
      "  if (typeof name !== 'string' || !name) throw new TypeError('matrix.anim.register(name, spec)');\n"
      "  if (typeof spec === 'function') spec = { start: spec };\n"
      "  if (!spec || typeof spec.start !== 'function') throw new TypeError('spec.start must be a function');\n"
      "  A._registry[name] = spec;\n"
      "  return true;\n"
      "};\n"
      "A.unregister = function(name) {\n"
      "  if (A._current && A._current.name === name) A.stop();\n"
      "  delete A._registry[name];\n"
      "  return true;\n"
      "};\n"
      "A.list = function() { return Object.keys(A._registry); };\n"
      "A.current = function() { return A._current ? A._current.name : null; };\n"
      "A.status = function() {\n"
      "  var tracked = (A._current && A._current.handles) ? A._current.handles.length : 0;\n"
      "  return { current: A.current(), registered: A.list().length, tracked: tracked };\n"
      "};\n"
      "A.start = function(name, opts) {\n"
      "  if (typeof name !== 'string' || !name) throw new TypeError('matrix.anim.start(name, opts)');\n"
      "  var spec = A._registry[name];\n"
      "  if (!spec) throw new Error('unknown animation: ' + name);\n"
      "  A.stop();\n"
      "  var rec = { name: name, handles: [], cleanups: [], stopped: false };\n"
      "  A._current = rec;\n"
      "  var ctx = {\n"
      "    matrix: m,\n"
      "    opts: opts || {},\n"
      "    shouldStop: function() { return !!(m.shouldStop() || rec.stopped); },\n"
      "    every: function(ms, fn) { var h = every(ms, fn); rec.handles.push(h); return h; },\n"
      "    timeout: function(ms, fn) { var id = setTimeout(fn, ms); rec.handles.push(id); return id; },\n"
      "    track: function(h) { rec.handles.push(h); return h; },\n"
      "    onCleanup: function(fn) { if (typeof fn === 'function') rec.cleanups.push(fn); },\n"
      "    nowMs: function() { return m.nowMs(); },\n"
      "    nowUs: function() { return m.nowUs(); }\n"
      "  };\n"
      "  var ret;\n"
      "  try {\n"
      "    ret = spec.start(ctx);\n"
      "  } catch (__e_start) {\n"
      "    A._current = null;\n"
      "    throw __e_start;\n"
      "  }\n"
      "  if (typeof ret === 'function') rec.cleanups.push(ret);\n"
      "  else if (ret && typeof ret.cancel === 'function') rec.handles.push(ret);\n"
      "  return true;\n"
      "};\n"
      "A.clear = function() {\n"
      "  A.stop();\n"
      "  A._registry = {};\n"
      "  return true;\n"
      "};\n"
      "A._onRuntimeStop = function() {\n"
      "  if (A._current) A._current.stopped = true;\n"
      "  try { A.stop(); } catch (__e_stop) {}\n"
      "};\n"
      "m.stop = function() {\n"
      "  if (m.anim && typeof m.anim._onRuntimeStop === 'function') m.anim._onRuntimeStop();\n"
      "  else m._stopRaw();\n"
      "  return true;\n"
      "};\n";

  JSValue v = JS_Eval(ctx, kBootstrap, strlen(kBootstrap), "<boot:0067>", JS_EVAL_REPL);
  if (JS_IsException(v)) {
    MqjsVm* vm = MqjsVm::From(ctx);
    const std::string err = vm ? vm->GetExceptionString(JS_DUMP_LONG) : "bootstrap failed";
    ESP_LOGW(TAG, "bootstrap failed: %s", err.c_str());
    return ESP_FAIL;
  }
  return ESP_OK;
}

static esp_err_t job_clear_timer_callbacks(JSContext* ctx, void* user) {
  (void)user;
  const char* code =
      "var g = globalThis;\n"
      "if (g.matrix && g.matrix.anim && typeof g.matrix.anim._onRuntimeStop === 'function') {\n"
      "  try { g.matrix.anim._onRuntimeStop(); } catch (__e_stop) {}\n"
      "}\n"
      "if (g.__0067 && g.__0067.timers) { g.__0067.timers.cb = {}; }\n";
  JSValue v = JS_Eval(ctx, code, strlen(code), "<stop:0067>", JS_EVAL_REPL);
  if (JS_IsException(v)) return ESP_FAIL;
  return ESP_OK;
}

static esp_err_t job_prepare_soft_reset(JSContext* ctx, void* user) {
  (void)user;
  const char* code =
      "var g = globalThis;\n"
      "g.__0067 = {};\n";
  JSValue v = JS_Eval(ctx, code, strlen(code), "<reset-soft:0067>", JS_EVAL_REPL);
  if (JS_IsException(v)) return ESP_FAIL;
  return ESP_OK;
}

struct DumpArg {
  std::string* out = nullptr;
};

struct RuntimeStatsArg {
  uint32_t timer_keys = 0;
  uint32_t timer_active = 0;
  uint32_t animations_registered = 0;
  char active_animation[32] = {0};
};

static bool parse_runtime_stats(const char* s, RuntimeStatsArg* out) {
  if (!s || !out) return false;
  unsigned long keys = 0;
  unsigned long active = 0;
  unsigned long regs = 0;
  char anim[32] = {0};
  const int n = sscanf(s, "%lu\t%lu\t%lu\t%31[^\n]", &keys, &active, &regs, anim);
  if (n < 3) return false;
  out->timer_keys = (uint32_t)keys;
  out->timer_active = (uint32_t)active;
  out->animations_registered = (uint32_t)regs;
  if (n >= 4) strlcpy(out->active_animation, anim, sizeof(out->active_animation));
  return true;
}

static bool decode_eval_string_literal(const char* in, std::string* out) {
  if (!in || !out) return false;
  out->clear();

  size_t n = strlen(in);
  while (n > 0 && (in[n - 1] == '\n' || in[n - 1] == '\r')) n--;
  if (n == 0) return false;

  if (in[0] != '"' || in[n - 1] != '"') {
    out->assign(in, n);
    return true;
  }

  for (size_t i = 1; i + 1 < n; i++) {
    char c = in[i];
    if (c == '\\' && i + 1 < n) {
      char d = in[++i];
      if (d == 'n') out->push_back('\n');
      else if (d == 'r') out->push_back('\r');
      else if (d == 't') out->push_back('\t');
      else if (d == '\\') out->push_back('\\');
      else if (d == '"') out->push_back('"');
      else out->push_back(d);
    } else {
      out->push_back(c);
    }
  }
  return true;
}

static esp_err_t job_dump_memory(JSContext* ctx, void* user) {
  auto* a = static_cast<DumpArg*>(user);
  if (!a || !a->out) return ESP_ERR_INVALID_ARG;
  MqjsVm* vm = MqjsVm::From(ctx);
  if (!vm) return ESP_FAIL;
  *(a->out) = vm->DumpMemory(false);
  return ESP_OK;
}

static esp_err_t job_collect_runtime_stats(JSContext* ctx, void* user) {
  auto* a = static_cast<RuntimeStatsArg*>(user);
  if (!a) return ESP_ERR_INVALID_ARG;

  const char* code =
      "(function(){\n"
      "  var g = globalThis;\n"
      "  var ns = g.__0067 || {};\n"
      "  var cb = (ns.timers && ns.timers.cb) || {};\n"
      "  var keys = Object.keys(cb);\n"
      "  var active = 0;\n"
      "  for (var i = 0; i < keys.length; i++) {\n"
      "    var v = cb[keys[i]];\n"
      "    if (v !== null && v !== undefined) active++;\n"
      "  }\n"
      "  var anim = (g.matrix && g.matrix.anim) || {};\n"
      "  var regs = Object.keys(anim._registry || {}).length;\n"
      "  var cur = (anim._current && anim._current.name) ? String(anim._current.name) : '';\n"
      "  return String(keys.length) + '\\t' + String(active) + '\\t' + String(regs) + '\\t' + cur;\n"
      "})();\n";

  JSValue v = JS_Eval(ctx, code, strlen(code), "<status:0067>", JS_EVAL_REPL);
  if (JS_IsException(v)) return ESP_FAIL;
  if (!JS_IsString(ctx, v)) return ESP_FAIL;

  JSCStringBuf sbuf;
  memset(&sbuf, 0, sizeof(sbuf));
  size_t n = 0;
  const char* s = JS_ToCStringLen(ctx, &n, v, &sbuf);
  if (!s || n == 0) return ESP_FAIL;
  if (!parse_runtime_stats(s, a)) return ESP_FAIL;
  return ESP_OK;
}

}  // namespace

extern "C" esp_err_t js_service_start(void) {
  if (s_svc) return ESP_OK;
  if (!s_mu) s_mu = xSemaphoreCreateMutex();
  if (!s_mu) return ESP_ERR_NO_MEM;

  mqjs_0067_runtime_clear_stop();

  mqjs_service_config_t cfg = {};
  cfg.task_name = "0067_js";
  cfg.task_stack_words = 6144;
  cfg.task_priority = 8;
  cfg.task_core_id = -1;
  cfg.queue_len = 16;
  cfg.arena_bytes = CONFIG_TUTORIAL_0067_JS_MEM_BYTES;
  cfg.stdlib = &js_stdlib;
  cfg.fix_global_this = true;

  esp_err_t st = mqjs_service_start(&cfg, &s_svc);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "mqjs_service_start failed: %s", esp_err_to_name(st));
    s_svc = nullptr;
    return st;
  }

  st = mqjs_0067_timers_start(s_svc);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "mqjs timers start failed: %s", esp_err_to_name(st));
    mqjs_service_stop(s_svc);
    s_svc = nullptr;
    return st;
  }

  mqjs_job_t boot = {};
  boot.fn = &job_bootstrap;
  boot.timeout_ms = 1000;
  st = mqjs_service_run(s_svc, &boot);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "bootstrap job failed: %s", esp_err_to_name(st));
  }

  lock_mu();
  memset(&s_status, 0, sizeof(s_status));
  s_status.started = true;
  unlock_mu();
  return ESP_OK;
}

extern "C" void js_service_stop(void) {
  mqjs_0067_runtime_request_stop();
  mqjs_0067_timers_stop();

  if (s_svc) {
    mqjs_service_stop(s_svc);
    s_svc = nullptr;
  }

  lock_mu();
  s_status.started = false;
  s_status.busy = false;
  s_status.stop_requested = true;
  s_status.timer_cb_keys = 0;
  s_status.timer_cb_active = 0;
  s_status.animations_registered = 0;
  memset(s_status.active_animation, 0, sizeof(s_status.active_animation));
  unlock_mu();
}

extern "C" esp_err_t js_service_reset(void) {
  if (!s_svc) return js_service_start();

  mqjs_0067_runtime_request_stop();
  (void)mqjs_0067_timers_cancel_all();
  (void)matrix_engine_stop();

  mqjs_job_t prep = {};
  prep.fn = &job_prepare_soft_reset;
  prep.timeout_ms = 100;
  esp_err_t st = mqjs_service_run(s_svc, &prep);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "soft reset prep failed: %s", esp_err_to_name(st));
    return st;
  }

  mqjs_job_t boot = {};
  boot.fn = &job_bootstrap;
  boot.timeout_ms = 1000;
  st = mqjs_service_run(s_svc, &boot);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "soft reset bootstrap failed: %s", esp_err_to_name(st));
    return st;
  }

  mqjs_0067_runtime_clear_stop();
  lock_mu();
  s_status.busy = false;
  s_status.stop_requested = false;
  s_status.last_timed_out = false;
  set_last_error_locked("");
  unlock_mu();
  return ESP_OK;
}

extern "C" esp_err_t js_service_hard_reset(void) {
  js_service_stop();
  return js_service_start();
}

extern "C" esp_err_t js_service_request_stop(void) {
  mqjs_0067_runtime_request_stop();
  (void)mqjs_0067_timers_cancel_all();
  (void)matrix_engine_stop();

  if (s_svc) {
    mqjs_job_t job = {};
    job.fn = &job_clear_timer_callbacks;
    job.timeout_ms = 100;
    (void)mqjs_service_post(s_svc, &job);
  }

  lock_mu();
  s_status.stop_requested = true;
  unlock_mu();
  return ESP_OK;
}

extern "C" esp_err_t js_service_eval_json(const char* code,
                                            size_t code_len,
                                            uint32_t timeout_ms,
                                            const char* filename,
                                            char** out_json) {
  if (!out_json) return ESP_ERR_INVALID_ARG;
  *out_json = nullptr;
  if (!s_svc) return ESP_ERR_INVALID_STATE;
  if (!code || code_len == 0) return ESP_ERR_INVALID_ARG;

  lock_mu();
  s_status.busy = true;
  s_status.stop_requested = false;
  unlock_mu();
  mqjs_0067_runtime_clear_stop();

  const uint64_t t0 = (uint64_t)(esp_timer_get_time() / 1000);
  const uint32_t tmo = timeout_ms ? timeout_ms : CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS;

  mqjs_eval_result_t r = {};
  const esp_err_t st = mqjs_service_eval(s_svc,
                                         code,
                                         code_len,
                                         tmo,
                                         filename ? filename : "<eval>",
                                         &r);

  const uint64_t t1 = (uint64_t)(esp_timer_get_time() / 1000);
  const uint32_t elapsed = (uint32_t)((t1 >= t0) ? (t1 - t0) : 0);

  lock_mu();
  s_status.busy = false;
  s_status.last_eval_ms = elapsed;
  s_status.eval_count++;
  if (st != ESP_OK) {
    set_last_error_locked((r.error && r.error[0]) ? r.error : esp_err_to_name(st));
    s_status.last_timed_out = false;
  } else {
    s_status.last_timed_out = r.timed_out;
    set_last_error_locked(r.error ? r.error : "");
  }
  unlock_mu();

  std::string json;
  if (st != ESP_OK) {
    json = "{\"ok\":false,\"output\":\"\",\"error\":\"";
    json_escape_append(&json, (r.error && r.error[0]) ? r.error : esp_err_to_name(st));
    json += "\",\"timed_out\":false}";
  } else if (!r.ok) {
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

  *out_json = dup_cstr(json);
  if (!*out_json) return ESP_ERR_NO_MEM;
  return ESP_OK;
}

extern "C" esp_err_t js_service_dump_memory_text(char** out_text) {
  if (!out_text) return ESP_ERR_INVALID_ARG;
  *out_text = nullptr;
  if (!s_svc) return ESP_ERR_INVALID_STATE;

  std::string out;
  DumpArg a = {.out = &out};
  mqjs_job_t job = {};
  job.fn = &job_dump_memory;
  job.user = &a;
  job.timeout_ms = 100;
  const esp_err_t st = mqjs_service_run(s_svc, &job);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "dump memory job failed: %s", esp_err_to_name(st));
    return st;
  }

  if (!out.empty() && out.back() != '\n') out.push_back('\n');
  *out_text = dup_cstr(out);
  if (!*out_text) return ESP_ERR_NO_MEM;
  return ESP_OK;
}

extern "C" esp_err_t js_service_get_status(js_service_status_t* out) {
  if (!out) return ESP_ERR_INVALID_ARG;
  RuntimeStatsArg rs = {};
  bool have_stats = false;
  if (s_svc) {
    mqjs_job_t job = {};
    job.fn = &job_collect_runtime_stats;
    job.user = &rs;
    job.timeout_ms = 500;
    const esp_err_t st = mqjs_service_run(s_svc, &job);
    if (st == ESP_OK) {
      have_stats = true;
    } else {
      static const char* kProbeCode =
          "(function(){\n"
          "  var g = globalThis;\n"
          "  var ns = g.__0067 || {};\n"
          "  var cb = (ns.timers && ns.timers.cb) || {};\n"
          "  var keys = Object.keys(cb);\n"
          "  var active = 0;\n"
          "  for (var i = 0; i < keys.length; i++) {\n"
          "    var v = cb[keys[i]];\n"
          "    if (v !== null && v !== undefined) active++;\n"
          "  }\n"
          "  var anim = (g.matrix && g.matrix.anim) || {};\n"
          "  var regs = Object.keys(anim._registry || {}).length;\n"
          "  var cur = (anim._current && anim._current.name) ? String(anim._current.name) : '';\n"
          "  return String(keys.length) + '\\t' + String(active) + '\\t' + String(regs) + '\\t' + cur;\n"
          "})();\n";

      mqjs_eval_result_t r = {};
      const esp_err_t est = mqjs_service_eval(s_svc,
                                              kProbeCode,
                                              strlen(kProbeCode),
                                              500,
                                              "<status-probe>",
                                              &r);
      if (est == ESP_OK && r.ok && r.output) {
        std::string decoded;
        if (decode_eval_string_literal(r.output, &decoded) && parse_runtime_stats(decoded.c_str(), &rs)) {
          have_stats = true;
        }
      }
      mqjs_eval_result_free(&r);
    }

    if (have_stats) {
      lock_mu();
      s_status.timer_cb_keys = rs.timer_keys;
      s_status.timer_cb_active = rs.timer_active;
      s_status.animations_registered = rs.animations_registered;
      strlcpy(s_status.active_animation, rs.active_animation, sizeof(s_status.active_animation));
      if (s_status.timer_cb_keys > s_status.timer_cb_keys_high_water) {
        s_status.timer_cb_keys_high_water = s_status.timer_cb_keys;
      }
      unlock_mu();
    }
  }

  lock_mu();
  s_status.heap_free_8bit = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_8BIT);
  s_status.heap_largest_free_8bit = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  s_status.heap_min_free_8bit = (uint32_t)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  *out = s_status;
  unlock_mu();
  return ESP_OK;
}

extern "C" void js_service_free(char* p) {
  if (p) free(p);
}
