#include "qjs_service.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace {

constexpr const char* TAG = "qjs_service";
constexpr size_t kDefaultMemoryLimit = 2 * 1024 * 1024;
constexpr size_t kDefaultStackLimit = 64 * 1024;
constexpr int kMaxPendingJobsPerEval = 64;

enum MsgType : uint8_t {
  MSG_EVAL = 1,
  MSG_JOB = 2,
  MSG_RESET = 3,
  MSG_STATUS = 4,
  MSG_STOP = 5,
};

struct Service;

struct Pending {
  SemaphoreHandle_t done = nullptr;
  StaticSemaphore_t done_storage = {};
  esp_err_t status = ESP_OK;
};

struct EvalPending : Pending {
  const char* code = nullptr;
  size_t len = 0;
  uint32_t timeout_ms = 0;
  const char* filename = "<eval>";
  qjs_eval_result_t* out = nullptr;
};

struct JobPending : Pending {
  qjs_job_fn_t fn = nullptr;
  void* user = nullptr;
  uint32_t timeout_ms = 0;
  bool heap_owned = false;
};

struct StatusPending : Pending {
  qjs_service_status_t* out = nullptr;
};

struct Msg {
  MsgType type = MSG_EVAL;
  void* pending = nullptr;
};

struct Service {
  qjs_service_config_t cfg = {};
  TaskHandle_t task = nullptr;
  QueueHandle_t q = nullptr;
  SemaphoreHandle_t ready = nullptr;

  JSRuntime* rt = nullptr;
  JSContext* ctx = nullptr;
  std::string* capture = nullptr;
  int64_t deadline_us = 0;
  bool busy = false;
  uint32_t eval_count = 0;
  uint32_t reset_count = 0;
  uint32_t last_eval_ms = 0;
};

static TickType_t ms_to_ticks(uint32_t ms) {
  if (ms == 0) return 0;
  return pdMS_TO_TICKS(ms);
}

static bool pending_init(Pending* p) {
  if (!p) return false;
  p->done = xSemaphoreCreateBinaryStatic(&p->done_storage);
  return p->done != nullptr;
}

static char* dup_string(const std::string& s) {
  char* out = static_cast<char*>(malloc(s.size() + 1));
  if (!out) return nullptr;
  memcpy(out, s.data(), s.size());
  out[s.size()] = 0;
  return out;
}

static char* dup_cstr(const char* s) {
  if (!s) s = "";
  return dup_string(std::string(s));
}

static JSValue js_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  (void)this_val;
  auto* s = static_cast<Service*>(JS_GetContextOpaque(ctx));
  std::string line;
  for (int i = 0; i < argc; ++i) {
    const char* text = JS_ToCString(ctx, argv[i]);
    if (!text) return JS_EXCEPTION;
    if (i > 0) line.push_back(' ');
    line.append(text);
    JS_FreeCString(ctx, text);
  }
  line.push_back('\n');

  if (s && s->capture) {
    s->capture->append(line);
  } else {
    fwrite(line.data(), 1, line.size(), stdout);
    fflush(stdout);
  }
  return JS_UNDEFINED;
}

static JSValue js_millis(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, esp_timer_get_time() / 1000);
}

static JSValue js_gc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  JSRuntime* rt = JS_GetRuntime(ctx);
  JS_RunGC(rt);
  return JS_UNDEFINED;
}

static int interrupt_handler(JSRuntime* rt, void* opaque) {
  (void)rt;
  auto* s = static_cast<Service*>(opaque);
  if (!s || s->deadline_us == 0) return 0;
  return esp_timer_get_time() > s->deadline_us;
}

static bool set_global_function(JSContext* ctx, JSValue global, const char* name, JSCFunction* fn, int length) {
  JSValue f = JS_NewCFunction(ctx, fn, name, length);
  if (JS_IsException(f)) return false;
  return JS_SetPropertyStr(ctx, global, name, f) >= 0;
}

static bool install_globals(Service* s) {
  JSValue global = JS_GetGlobalObject(s->ctx);
  if (JS_IsException(global)) return false;
  const bool ok = set_global_function(s->ctx, global, "print", js_print, 1) &&
                  set_global_function(s->ctx, global, "millis", js_millis, 0) &&
                  set_global_function(s->ctx, global, "gc", js_gc, 0);
  JS_FreeValue(s->ctx, global);
  return ok;
}

static void destroy_runtime(Service* s) {
  if (!s) return;
  if (s->ctx) {
    JS_SetContextOpaque(s->ctx, nullptr);
    JS_FreeContext(s->ctx);
    s->ctx = nullptr;
  }
  if (s->rt) {
    JS_FreeRuntime(s->rt);
    s->rt = nullptr;
  }
  s->capture = nullptr;
  s->deadline_us = 0;
}

static esp_err_t create_runtime(Service* s) {
  if (!s) return ESP_ERR_INVALID_ARG;
  destroy_runtime(s);

  s->rt = JS_NewRuntime();
  if (!s->rt) return ESP_ERR_NO_MEM;
  JS_SetRuntimeOpaque(s->rt, s);
  JS_SetMemoryLimit(s->rt, s->cfg.memory_limit_bytes);
  JS_SetMaxStackSize(s->rt, s->cfg.stack_limit_bytes);
  JS_SetCanBlock(s->rt, s->cfg.can_block ? 1 : 0);
  JS_SetInterruptHandler(s->rt, interrupt_handler, s);

  s->ctx = JS_NewContext(s->rt);
  if (!s->ctx) {
    destroy_runtime(s);
    return ESP_ERR_NO_MEM;
  }
  JS_SetContextOpaque(s->ctx, s);
  if (!install_globals(s)) {
    destroy_runtime(s);
    return ESP_FAIL;
  }
  return ESP_OK;
}

static void fill_exception_ctx(JSContext* ctx, qjs_eval_result_t* out) {
  if (!ctx || !out) return;
  JSValue exc = JS_GetException(ctx);
  const char* text = JS_ToCString(ctx, exc);
  out->ok = false;
  out->error = dup_cstr(text ? text : "<exception stringify failed>");
  if (text) JS_FreeCString(ctx, text);
  JS_FreeValue(ctx, exc);
}

static void fill_exception(Service* s, qjs_eval_result_t* out) {
  if (!s) return;
  fill_exception_ctx(s->ctx, out);
}

static bool drain_pending_jobs(Service* s, qjs_eval_result_t* out) {
  if (!s || !s->rt) return false;
  int jobs = 0;
  for (;;) {
    if (s->deadline_us != 0 && esp_timer_get_time() > s->deadline_us) {
      if (out) {
        out->ok = false;
        out->timed_out = true;
        out->error = dup_cstr("execution timed out while draining promise jobs");
      }
      return false;
    }

    JSContext* job_ctx = nullptr;
    int rc = JS_ExecutePendingJob(s->rt, &job_ctx);
    if (rc == 0) return true;
    if (rc < 0) {
      fill_exception_ctx(job_ctx ? job_ctx : s->ctx, out);
      return false;
    }
    jobs++;
    if (jobs >= kMaxPendingJobsPerEval) {
      if (out) {
        out->ok = false;
        out->error = dup_cstr("too many pending promise jobs");
      }
      return false;
    }
  }
}

static void fill_status(Service* s, qjs_service_status_t* out) {
  if (!out) return;
  *out = {};
  out->ready = s && s->ctx && s->rt;
  out->busy = s ? s->busy : false;
  out->eval_count = s ? s->eval_count : 0;
  out->reset_count = s ? s->reset_count : 0;
  out->last_eval_ms = s ? s->last_eval_ms : 0;
  out->memory_limit_bytes = s ? s->cfg.memory_limit_bytes : 0;
  out->stack_limit_bytes = s ? s->cfg.stack_limit_bytes : 0;
  out->esp_heap_internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  out->esp_heap_8bit_free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  out->esp_heap_psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  if (s && s->rt) {
    JSMemoryUsage mu = {};
    JS_ComputeMemoryUsage(s->rt, &mu);
    out->malloc_size = mu.malloc_size;
    out->memory_used_size = mu.memory_used_size;
    out->atom_count = mu.atom_count;
  }
}

static esp_err_t ensure_runtime(Service* s) {
  if (!s) return ESP_ERR_INVALID_ARG;
  if (s->rt && s->ctx) return ESP_OK;
  return create_runtime(s);
}

static void service_task(void* arg) {
  auto* s = static_cast<Service*>(arg);
  if (!s) {
    vTaskDelete(nullptr);
    return;
  }

  ESP_LOGI(TAG, "task start name=%s prio=%u core=%d", pcTaskGetName(nullptr),
           (unsigned)uxTaskPriorityGet(nullptr), (int)xPortGetCoreID());

  const int64_t t0 = esp_timer_get_time();
  esp_err_t init_st = create_runtime(s);
  ESP_LOGI(TAG, "runtime init status=%s elapsed=%lld ms", esp_err_to_name(init_st),
           (long long)((esp_timer_get_time() - t0) / 1000));

  if (s->ready) xSemaphoreGive(s->ready);

  for (;;) {
    Msg msg = {};
    if (xQueueReceive(s->q, &msg, portMAX_DELAY) != pdTRUE) continue;

    if (msg.type == MSG_STOP) {
      auto* p = static_cast<Pending*>(msg.pending);
      destroy_runtime(s);
      if (p && p->done) xSemaphoreGive(p->done);
      vTaskDelete(nullptr);
      return;
    }

    if (msg.type == MSG_RESET) {
      auto* p = static_cast<Pending*>(msg.pending);
      s->busy = true;
      p->status = create_runtime(s);
      if (p->status == ESP_OK) s->reset_count++;
      s->busy = false;
      xSemaphoreGive(p->done);
      continue;
    }

    if (msg.type == MSG_STATUS) {
      auto* p = static_cast<StatusPending*>(msg.pending);
      fill_status(s, p ? p->out : nullptr);
      if (p) {
        p->status = ESP_OK;
        xSemaphoreGive(p->done);
      }
      continue;
    }

    esp_err_t st = ensure_runtime(s);
    if (st != ESP_OK) {
      auto* p = static_cast<Pending*>(msg.pending);
      if (p) {
        p->status = st;
        xSemaphoreGive(p->done);
      }
      continue;
    }

    if (msg.type == MSG_EVAL) {
      auto* p = static_cast<EvalPending*>(msg.pending);
      if (!p || !p->out) continue;
      *(p->out) = {};
      std::string printed;
      s->capture = &printed;
      s->busy = true;
      s->deadline_us = p->timeout_ms ? (esp_timer_get_time() + (int64_t)p->timeout_ms * 1000) : 0;
      const int64_t t_eval = esp_timer_get_time();
      JSValue val = JS_Eval(s->ctx, p->code, p->len, p->filename ? p->filename : "<eval>", JS_EVAL_TYPE_GLOBAL);
      const bool eval_exception = JS_IsException(val);
      if (eval_exception) {
        fill_exception(s, p->out);
      } else {
        p->out->ok = drain_pending_jobs(s, p->out);
      }
      const int64_t elapsed_us = esp_timer_get_time() - t_eval;
      const bool timed_out = p->out->timed_out || ((s->deadline_us != 0) && (esp_timer_get_time() > s->deadline_us));
      s->deadline_us = 0;
      s->capture = nullptr;
      s->busy = false;
      s->last_eval_ms = (uint32_t)(elapsed_us / 1000);
      s->eval_count++;

      p->out->timed_out = timed_out;
      p->out->elapsed_ms = s->last_eval_ms;
      p->out->output = dup_string(printed);
      if (!eval_exception && p->out->ok) {
        if (!JS_IsUndefined(val)) {
          const char* text = JS_ToCString(s->ctx, val);
          if (text) {
            if (!p->out->output) p->out->output = dup_cstr("");
            std::string merged(p->out->output ? p->out->output : "");
            merged.append(text);
            merged.push_back('\n');
            free(p->out->output);
            p->out->output = dup_string(merged);
            JS_FreeCString(s->ctx, text);
          }
        }
      }
      JS_FreeValue(s->ctx, val);
      p->status = ESP_OK;
      xSemaphoreGive(p->done);
      continue;
    }

    if (msg.type == MSG_JOB) {
      auto* p = static_cast<JobPending*>(msg.pending);
      if (!p) continue;
      s->busy = true;
      s->deadline_us = p->timeout_ms ? (esp_timer_get_time() + (int64_t)p->timeout_ms * 1000) : 0;
      p->status = p->fn ? p->fn(s->ctx, p->user) : ESP_ERR_INVALID_ARG;
      s->deadline_us = 0;
      s->busy = false;
      if (p->done) xSemaphoreGive(p->done);
      if (p->heap_owned) free(p);
      continue;
    }
  }
}

static Service* as_service(qjs_service_t* s) {
  return reinterpret_cast<Service*>(s);
}

}  // namespace

esp_err_t qjs_service_start(const qjs_service_config_t* cfg, qjs_service_t** out) {
  if (!out) return ESP_ERR_INVALID_ARG;
  *out = nullptr;

  auto* s = new Service();
  if (!s) return ESP_ERR_NO_MEM;
  if (cfg) s->cfg = *cfg;
  if (!s->cfg.task_name) s->cfg.task_name = "qjs_svc";
  if (s->cfg.task_stack_words == 0) s->cfg.task_stack_words = 8192;
  if (s->cfg.task_priority == 0) s->cfg.task_priority = 8;
  if (s->cfg.queue_len == 0) s->cfg.queue_len = 16;
  if (s->cfg.memory_limit_bytes == 0) s->cfg.memory_limit_bytes = kDefaultMemoryLimit;
  if (s->cfg.stack_limit_bytes == 0) s->cfg.stack_limit_bytes = kDefaultStackLimit;

  s->ready = xSemaphoreCreateBinaryWithCaps(MALLOC_CAP_INTERNAL);
  if (!s->ready) {
    delete s;
    return ESP_ERR_NO_MEM;
  }
  s->q = xQueueCreateWithCaps((UBaseType_t)s->cfg.queue_len, sizeof(Msg), MALLOC_CAP_INTERNAL);
  if (!s->q) {
    vSemaphoreDeleteWithCaps(s->ready);
    delete s;
    return ESP_ERR_NO_MEM;
  }

  BaseType_t ok = pdFAIL;
  if (s->cfg.task_core_id >= 0) {
    ok = xTaskCreatePinnedToCore(service_task, s->cfg.task_name, s->cfg.task_stack_words, s,
                                 (UBaseType_t)s->cfg.task_priority, &s->task,
                                 (BaseType_t)s->cfg.task_core_id);
  } else {
    ok = xTaskCreate(service_task, s->cfg.task_name, s->cfg.task_stack_words, s,
                     (UBaseType_t)s->cfg.task_priority, &s->task);
  }
  if (ok != pdPASS) {
    vQueueDeleteWithCaps(s->q);
    vSemaphoreDeleteWithCaps(s->ready);
    delete s;
    return ESP_ERR_NO_MEM;
  }

  if (xSemaphoreTake(s->ready, pdMS_TO_TICKS(2000)) != pdTRUE) {
    vTaskDelete(s->task);
    vQueueDeleteWithCaps(s->q);
    vSemaphoreDeleteWithCaps(s->ready);
    delete s;
    return ESP_ERR_TIMEOUT;
  }
  vSemaphoreDeleteWithCaps(s->ready);
  s->ready = nullptr;

  *out = reinterpret_cast<qjs_service_t*>(s);
  return ESP_OK;
}

void qjs_service_stop(qjs_service_t* s_) {
  auto* s = as_service(s_);
  if (!s) return;
  Pending p = {};
  if (pending_init(&p) && s->q) {
    Msg msg = {};
    msg.type = MSG_STOP;
    msg.pending = &p;
    if (xQueueSend(s->q, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
      xSemaphoreTake(p.done, pdMS_TO_TICKS(1000));
    }
  } else {
    destroy_runtime(s);
    if (s->task) vTaskDelete(s->task);
  }
  if (s->q) vQueueDeleteWithCaps(s->q);
  delete s;
}

void qjs_eval_result_free(qjs_eval_result_t* r) {
  if (!r) return;
  if (r->output) free(r->output);
  if (r->error) free(r->error);
  *r = {};
}

esp_err_t qjs_service_eval(qjs_service_t* s_, const char* code, size_t len, uint32_t timeout_ms,
                           const char* filename, qjs_eval_result_t* out) {
  auto* s = as_service(s_);
  if (!s || !code || len == 0 || !out) return ESP_ERR_INVALID_ARG;

  EvalPending p = {};
  if (!pending_init(&p)) return ESP_ERR_NO_MEM;
  p.code = code;
  p.len = len;
  p.timeout_ms = timeout_ms;
  p.filename = filename ? filename : "<eval>";
  p.out = out;

  Msg msg = {};
  msg.type = MSG_EVAL;
  msg.pending = &p;
  if (xQueueSend(s->q, &msg, pdMS_TO_TICKS(100)) != pdTRUE) return ESP_ERR_TIMEOUT;
  xSemaphoreTake(p.done, portMAX_DELAY);
  return p.status;
}

esp_err_t qjs_service_run(qjs_service_t* s_, const qjs_job_t* job) {
  auto* s = as_service(s_);
  if (!s || !job || !job->fn) return ESP_ERR_INVALID_ARG;

  JobPending p = {};
  if (!pending_init(&p)) return ESP_ERR_NO_MEM;
  p.fn = job->fn;
  p.user = job->user;
  p.timeout_ms = job->timeout_ms;

  Msg msg = {};
  msg.type = MSG_JOB;
  msg.pending = &p;
  if (xQueueSend(s->q, &msg, pdMS_TO_TICKS(100)) != pdTRUE) return ESP_ERR_TIMEOUT;
  xSemaphoreTake(p.done, portMAX_DELAY);
  return p.status;
}

esp_err_t qjs_service_post(qjs_service_t* s_, const qjs_job_t* job) {
  auto* s = as_service(s_);
  if (!s || !job || !job->fn) return ESP_ERR_INVALID_ARG;

  auto* p = static_cast<JobPending*>(calloc(1, sizeof(JobPending)));
  if (!p) return ESP_ERR_NO_MEM;
  p->fn = job->fn;
  p->user = job->user;
  p->timeout_ms = job->timeout_ms;
  p->heap_owned = true;

  Msg msg = {};
  msg.type = MSG_JOB;
  msg.pending = p;
  if (xQueueSend(s->q, &msg, 0) != pdTRUE) {
    free(p);
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}

esp_err_t qjs_service_reset(qjs_service_t* s_, uint32_t timeout_ms) {
  auto* s = as_service(s_);
  if (!s) return ESP_ERR_INVALID_ARG;

  Pending p = {};
  if (!pending_init(&p)) return ESP_ERR_NO_MEM;
  Msg msg = {};
  msg.type = MSG_RESET;
  msg.pending = &p;
  if (xQueueSend(s->q, &msg, ms_to_ticks(timeout_ms ? timeout_ms : 100)) != pdTRUE) return ESP_ERR_TIMEOUT;
  if (xSemaphoreTake(p.done, timeout_ms ? ms_to_ticks(timeout_ms) : portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;
  return p.status;
}

esp_err_t qjs_service_get_status(qjs_service_t* s_, qjs_service_status_t* out, uint32_t timeout_ms) {
  auto* s = as_service(s_);
  if (!s || !out) return ESP_ERR_INVALID_ARG;

  StatusPending p = {};
  if (!pending_init(&p)) return ESP_ERR_NO_MEM;
  p.out = out;
  Msg msg = {};
  msg.type = MSG_STATUS;
  msg.pending = &p;
  if (xQueueSend(s->q, &msg, ms_to_ticks(timeout_ms ? timeout_ms : 100)) != pdTRUE) return ESP_ERR_TIMEOUT;
  if (xSemaphoreTake(p.done, timeout_ms ? ms_to_ticks(timeout_ms) : portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;
  return p.status;
}
