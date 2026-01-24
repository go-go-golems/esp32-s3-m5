#include "js_service.h"

#include <string.h>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include <string>

#include "esp_log.h"

#include "sdkconfig.h"

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

static void log_js_exception(JSContext* ctx, const char* what) {
  if (!ctx) return;
  MqjsVm* vm = MqjsVm::From(ctx);
  const std::string err = vm ? vm->GetExceptionString(JS_DUMP_LONG) : "<exception>";
  ESP_LOGW(TAG, "%s: %s", what ? what : "js exception", err.c_str());
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
