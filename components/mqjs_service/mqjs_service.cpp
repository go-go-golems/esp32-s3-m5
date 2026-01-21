#include "mqjs_service.h"

#include <string.h>

extern "C" {
#include "mquickjs.h"
}

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_timer.h"

#include "mqjs_vm.h"

namespace {

enum MsgType : uint8_t {
  MSG_EVAL = 1,
  MSG_JOB = 2,
};

struct Pending {
  SemaphoreHandle_t done = nullptr;
  StaticSemaphore_t done_storage = {};
  bool done_static = false;
  esp_err_t status = ESP_OK;
};

struct EvalPending : Pending {
  const char* code = nullptr;
  size_t len = 0;
  uint32_t timeout_ms = 0;
  const char* filename = "<eval>";
  mqjs_eval_result_t* out = nullptr;
};

struct JobPending : Pending {
  mqjs_job_fn_t fn = nullptr;
  void* user = nullptr;
  uint32_t timeout_ms = 0;
  bool heap_owned = false;
};

struct Msg {
  MsgType type = MSG_EVAL;
  void* pending = nullptr;  // EvalPending* or JobPending*
};

static inline TickType_t ms_to_ticks(uint32_t ms) {
  if (ms == 0) return 0;
  return pdMS_TO_TICKS(ms);
}

static inline bool pending_init_static(Pending* p) {
  if (!p) return false;
  p->done = xSemaphoreCreateBinaryStatic(&p->done_storage);
  p->done_static = (p->done != nullptr);
  return p->done_static;
}

static inline char* dup_cstr(const std::string& s) {
  char* out = static_cast<char*>(malloc(s.size() + 1));
  if (!out) return nullptr;
  memcpy(out, s.data(), s.size());
  out[s.size()] = 0;
  return out;
}

struct Service {
  mqjs_service_config_t cfg = {};

  TaskHandle_t task = nullptr;
  QueueHandle_t q = nullptr;
  SemaphoreHandle_t ready = nullptr;

  uint8_t* arena = nullptr;
  MqjsVm* vm = nullptr;
  JSContext* ctx = nullptr;
};

static void eval_fill_error(mqjs_eval_result_t* out, const char* msg) {
  if (!out) return;
  out->ok = false;
  out->timed_out = false;
  out->output = dup_cstr(std::string());
  out->error = dup_cstr(std::string(msg ? msg : "error"));
}

static void service_ensure_ctx(Service* s) {
  if (!s || s->ctx) return;

  s->arena = static_cast<uint8_t*>(malloc(s->cfg.arena_bytes));
  if (!s->arena) return;
  memset(s->arena, 0, s->cfg.arena_bytes);

  MqjsVmConfig cfg = {};
  cfg.arena = s->arena;
  cfg.arena_bytes = s->cfg.arena_bytes;
  cfg.stdlib = s->cfg.stdlib;
  cfg.fix_global_this = s->cfg.fix_global_this;

  s->vm = MqjsVm::Create(cfg);
  if (!s->vm) {
    free(s->arena);
    s->arena = nullptr;
    return;
  }
  s->ctx = s->vm->ctx();
}

static void service_task(void* arg) {
  auto* s = static_cast<Service*>(arg);
  if (!s) {
    vTaskDelete(nullptr);
    return;
  }

  // Signal creator that the task started and is safe to interact with.
  if (s->ready) {
    xSemaphoreGive(s->ready);
  }
  if (!s->q) {
    vTaskDelete(nullptr);
    return;
  }

  for (;;) {
    Msg msg = {};
    if (xQueueReceive(s->q, &msg, portMAX_DELAY) != pdTRUE) continue;
    if (!msg.pending) continue;

    service_ensure_ctx(s);

    if (msg.type == MSG_EVAL) {
      auto* p = static_cast<EvalPending*>(msg.pending);
      if (p->out) {
        *(p->out) = {};
      }

      if (!s->ctx || !s->vm) {
        eval_fill_error(p->out, "js init failed");
        p->status = ESP_FAIL;
        xSemaphoreGive(p->done);
        continue;
      }

      s->vm->SetDeadlineMs(p->timeout_ms);
      const int64_t deadline_us =
          (p->timeout_ms > 0) ? (esp_timer_get_time() + (int64_t)p->timeout_ms * 1000) : 0;

      const int flags = JS_EVAL_REPL | JS_EVAL_RETVAL;
      JSValue val = JS_Eval(s->ctx, p->code, p->len, p->filename ? p->filename : "<eval>", flags);

      const bool timed_out = (deadline_us != 0) && (esp_timer_get_time() > deadline_us);
      s->vm->ClearDeadline();

      if (!p->out) {
        p->status = ESP_OK;
        xSemaphoreGive(p->done);
        continue;
      }

      p->out->timed_out = timed_out;

      if (JS_IsException(val)) {
        p->out->ok = false;
        p->out->output = dup_cstr(std::string());
        p->out->error = dup_cstr(s->vm->GetExceptionString(JS_DUMP_LONG));
        p->status = ESP_OK;
        xSemaphoreGive(p->done);
        continue;
      }

      p->out->ok = true;
      std::string out;
      if (!JS_IsUndefined(val)) {
        out = s->vm->PrintValue(val, JS_DUMP_LONG);
        out.push_back('\n');
      }
      p->out->output = dup_cstr(out);
      p->out->error = nullptr;
      p->status = ESP_OK;
      xSemaphoreGive(p->done);
      continue;
    }

    if (msg.type == MSG_JOB) {
      auto* p = static_cast<JobPending*>(msg.pending);
      if (!s->ctx || !s->vm) {
        p->status = ESP_FAIL;
        if (p->done) {
          xSemaphoreGive(p->done);
        }
        if (p->heap_owned) {
          if (p->done && !p->done_static) vSemaphoreDelete(p->done);
          free(p);
        }
        continue;
      }

      s->vm->SetDeadlineMs(p->timeout_ms);
      esp_err_t st = ESP_FAIL;
      if (p->fn) {
        st = p->fn(s->ctx, p->user);
      }
      s->vm->ClearDeadline();
      p->status = st;
      if (p->done) {
        xSemaphoreGive(p->done);
      }
      if (p->heap_owned) {
        if (p->done && !p->done_static) vSemaphoreDelete(p->done);
        free(p);
      }
      continue;
    }
  }
}

}  // namespace

esp_err_t mqjs_service_start(const mqjs_service_config_t* cfg, mqjs_service_t** out) {
  if (!out) return ESP_ERR_INVALID_ARG;
  *out = nullptr;
  if (!cfg || cfg->arena_bytes == 0 || !cfg->stdlib) return ESP_ERR_INVALID_ARG;

  auto* s = new Service();
  s->cfg = *cfg;
  if (!s->cfg.task_name) s->cfg.task_name = "mqjs_svc";
  if (s->cfg.task_stack_words == 0) s->cfg.task_stack_words = 6144 / 4;
  if (s->cfg.task_priority == 0) s->cfg.task_priority = 8;
  if (s->cfg.queue_len == 0) s->cfg.queue_len = 16;
  if (!s->cfg.fix_global_this) {
    // keep explicit bool; no-op
  }

  s->ready = xSemaphoreCreateBinary();
  if (!s->ready) {
    delete s;
    return ESP_ERR_NO_MEM;
  }

  s->q = xQueueCreate(static_cast<UBaseType_t>(s->cfg.queue_len), sizeof(Msg));
  if (!s->q) {
    vSemaphoreDelete(s->ready);
    delete s;
    return ESP_ERR_NO_MEM;
  }

  BaseType_t ok = pdFAIL;
  if (s->cfg.task_core_id >= 0) {
    ok = xTaskCreatePinnedToCore(&service_task,
                                s->cfg.task_name,
                                static_cast<uint32_t>(s->cfg.task_stack_words),
                                s,
                                static_cast<UBaseType_t>(s->cfg.task_priority),
                                &s->task,
                                static_cast<BaseType_t>(s->cfg.task_core_id));
  } else {
    ok = xTaskCreate(&service_task,
                    s->cfg.task_name,
                    static_cast<uint32_t>(s->cfg.task_stack_words),
                    s,
                    static_cast<UBaseType_t>(s->cfg.task_priority),
                    &s->task);
  }
  if (ok != pdPASS) {
    vQueueDelete(s->q);
    vSemaphoreDelete(s->ready);
    delete s;
    return ESP_ERR_NO_MEM;
  }

  // Ensure the created task is running before returning. If the task fails to start,
  // consumers could otherwise enqueue work and then deadlock waiting on completions.
  if (xSemaphoreTake(s->ready, pdMS_TO_TICKS(1000)) != pdTRUE) {
    vTaskDelete(s->task);
    vQueueDelete(s->q);
    vSemaphoreDelete(s->ready);
    delete s;
    return ESP_FAIL;
  }
  vSemaphoreDelete(s->ready);
  s->ready = nullptr;

  *out = reinterpret_cast<mqjs_service_t*>(s);
  return ESP_OK;
}

void mqjs_service_stop(mqjs_service_t* s_) {
  auto* s = reinterpret_cast<Service*>(s_);
  if (!s) return;
  if (s->task) {
    vTaskDelete(s->task);
    s->task = nullptr;
  }
  if (s->q) {
    vQueueDelete(s->q);
    s->q = nullptr;
  }
  if (s->vm) {
    MqjsVm::DestroyContext(s->ctx);
    s->vm = nullptr;
    s->ctx = nullptr;
  }
  if (s->arena) {
    free(s->arena);
    s->arena = nullptr;
  }
  delete s;
}

void mqjs_eval_result_free(mqjs_eval_result_t* r) {
  if (!r) return;
  if (r->output) free(r->output);
  if (r->error) free(r->error);
  *r = {};
}

esp_err_t mqjs_service_eval(mqjs_service_t* s_,
                            const char* code,
                            size_t len,
                            uint32_t timeout_ms,
                            const char* filename,
                            mqjs_eval_result_t* out) {
  auto* s = reinterpret_cast<Service*>(s_);
  if (!s || !code || len == 0 || !out) return ESP_ERR_INVALID_ARG;

  auto* p = static_cast<EvalPending*>(calloc(1, sizeof(EvalPending)));
  if (!p) return ESP_ERR_NO_MEM;
  if (!pending_init_static(p)) {
    free(p);
    return ESP_ERR_NO_MEM;
  }
  p->code = code;
  p->len = len;
  p->timeout_ms = timeout_ms;
  p->filename = filename ? filename : "<eval>";
  p->out = out;

  Msg msg = {};
  msg.type = MSG_EVAL;
  msg.pending = p;
  if (xQueueSend(s->q, &msg, ms_to_ticks(100)) != pdTRUE) {
    free(p);
    return ESP_ERR_TIMEOUT;
  }
  xSemaphoreTake(p->done, portMAX_DELAY);
  const esp_err_t status = p->status;
  free(p);
  return status;
}

esp_err_t mqjs_service_run(mqjs_service_t* s_, const mqjs_job_t* job) {
  auto* s = reinterpret_cast<Service*>(s_);
  if (!s || !job || !job->fn) return ESP_ERR_INVALID_ARG;

  auto* p = static_cast<JobPending*>(calloc(1, sizeof(JobPending)));
  if (!p) return ESP_ERR_NO_MEM;
  if (!pending_init_static(p)) {
    free(p);
    return ESP_ERR_NO_MEM;
  }
  p->fn = job->fn;
  p->user = job->user;
  p->timeout_ms = job->timeout_ms;

  Msg msg = {};
  msg.type = MSG_JOB;
  msg.pending = p;
  if (xQueueSend(s->q, &msg, ms_to_ticks(100)) != pdTRUE) {
    free(p);
    return ESP_ERR_TIMEOUT;
  }
  xSemaphoreTake(p->done, portMAX_DELAY);
  const esp_err_t status = p->status;
  free(p);
  return status;
}

esp_err_t mqjs_service_post(mqjs_service_t* s_, const mqjs_job_t* job) {
  auto* s = reinterpret_cast<Service*>(s_);
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
