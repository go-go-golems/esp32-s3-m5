#include "mqjs_timers.h"

#include <string.h>

extern "C" {
#include "mquickjs.h"
}

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "mqjs_vm.h"

static const char* TAG = "0066_mqjs_timers";

namespace {

constexpr uint32_t kMaxTimers = 32;

enum CmdType : uint8_t {
  CMD_SCHEDULE = 1,
  CMD_CANCEL = 2,
};

struct Cmd {
  CmdType type = CMD_SCHEDULE;
  uint32_t id = 0;
  uint32_t delay_ms = 0;
};

struct TimerSlot {
  bool active = false;
  uint32_t id = 0;
  int64_t due_us = 0;
};

struct FireArg {
  uint32_t id = 0;
};

static mqjs_service_t* s_svc = nullptr;
static TaskHandle_t s_task = nullptr;
static QueueHandle_t s_q = nullptr;
static TimerSlot s_timers[kMaxTimers];

extern "C" void __attribute__((weak)) mqjs_0066_after_js_callback(JSContext* ctx, const char* source) {
  (void)ctx;
  (void)source;
}

static void log_js_exception(JSContext* ctx, const char* what) {
  if (!ctx) return;
  MqjsVm* vm = MqjsVm::From(ctx);
  const std::string err = vm ? vm->GetExceptionString(JS_DUMP_LONG) : "<exception>";
  ESP_LOGW(TAG, "%s: %s", what ? what : "js exception", err.c_str());
}

static JSValue get_timer_cb(JSContext* ctx, uint32_t id, JSValue* out_cb_table) {
  if (out_cb_table) *out_cb_table = JS_UNDEFINED;
  if (!ctx) return JS_UNDEFINED;

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
  if (JS_IsUndefined(cb) || JS_IsNull(cb)) {
    return JS_UNDEFINED;
  }

  JSValue fn = JS_GetPropertyUint32(ctx, cb, id);
  if (out_cb_table) {
    *out_cb_table = cb;
  }
  return fn;
}

static esp_err_t job_fire_timeout(JSContext* ctx, void* user) {
  auto* a = static_cast<FireArg*>(user);
  const uint32_t id = a ? a->id : 0;
  free(a);

  if (!ctx || id == 0) return ESP_OK;

  JSValue cb_table = JS_UNDEFINED;
  JSValue cb = get_timer_cb(ctx, id, &cb_table);
  if (JS_IsUndefined(cb) || JS_IsNull(cb) || !JS_IsFunction(ctx, cb)) {
    return ESP_OK;
  }

  // Call cb() with no args.
  JS_PushArg(ctx, cb);
  JS_PushArg(ctx, JS_NULL);
  JSValue ret = JS_Call(ctx, 0);
  const bool threw = JS_IsException(ret);

  // One-shot timeout: drop the reference so it can be GC'ed.
  (void)JS_SetPropertyUint32(ctx, cb_table, id, JS_NULL);

  if (threw) {
    log_js_exception(ctx, "timeout callback threw");
  }

  mqjs_0066_after_js_callback(ctx, "timer");
  return ESP_OK;
}

static void post_fire(uint32_t id) {
  if (!s_svc || id == 0) return;

  auto* a = static_cast<FireArg*>(calloc(1, sizeof(FireArg)));
  if (!a) return;
  a->id = id;

  mqjs_job_t job = {};
  job.fn = &job_fire_timeout;
  job.user = a;
  job.timeout_ms = 50;

  const esp_err_t st = mqjs_service_post(s_svc, &job);
  if (st != ESP_OK) {
    free(a);
    ESP_LOGW(TAG, "mqjs_service_post(timeout) failed: %s", esp_err_to_name(st));
  }
}

static int64_t now_us() { return esp_timer_get_time(); }

static int find_slot_by_id(uint32_t id) {
  for (uint32_t i = 0; i < kMaxTimers; i++) {
    if (s_timers[i].active && s_timers[i].id == id) return (int)i;
  }
  return -1;
}

static int find_free_slot() {
  for (uint32_t i = 0; i < kMaxTimers; i++) {
    if (!s_timers[i].active) return (int)i;
  }
  return -1;
}

static int64_t next_due_us() {
  int64_t best = 0;
  for (uint32_t i = 0; i < kMaxTimers; i++) {
    if (!s_timers[i].active) continue;
    if (best == 0 || s_timers[i].due_us < best) best = s_timers[i].due_us;
  }
  return best;
}

static void handle_cmd(const Cmd& c) {
  if (c.id == 0) return;
  if (c.type == CMD_CANCEL) {
    const int idx = find_slot_by_id(c.id);
    if (idx >= 0) {
      s_timers[idx].active = false;
      s_timers[idx].id = 0;
      s_timers[idx].due_us = 0;
    }
    return;
  }

  if (c.type == CMD_SCHEDULE) {
    int idx = find_slot_by_id(c.id);
    if (idx < 0) idx = find_free_slot();
    if (idx < 0) {
      ESP_LOGW(TAG, "no timer slots left (max=%u)", (unsigned)kMaxTimers);
      return;
    }

    s_timers[idx].active = true;
    s_timers[idx].id = c.id;
    s_timers[idx].due_us = now_us() + (int64_t)c.delay_ms * 1000;
    return;
  }
}

static void timers_task(void* arg) {
  (void)arg;
  ESP_LOGI(TAG, "task start");

  for (;;) {
    const int64_t due = next_due_us();
    int64_t wait_ms = 1000;
    if (due != 0) {
      const int64_t now = now_us();
      wait_ms = (due <= now) ? 0 : ((due - now) / 1000);
      if (wait_ms < 0) wait_ms = 0;
      if (wait_ms > 1000) wait_ms = 1000;
    }

    Cmd cmd = {};
    const TickType_t to = (wait_ms == 0) ? 0 : pdMS_TO_TICKS((uint32_t)wait_ms);
    if (xQueueReceive(s_q, &cmd, to) == pdTRUE) {
      handle_cmd(cmd);
      continue;
    }

    // Timeout expired: fire any due timers.
    const int64_t now = now_us();
    for (uint32_t i = 0; i < kMaxTimers; i++) {
      if (!s_timers[i].active) continue;
      if (s_timers[i].due_us > now) continue;
      const uint32_t id = s_timers[i].id;
      s_timers[i].active = false;
      s_timers[i].id = 0;
      s_timers[i].due_us = 0;
      post_fire(id);
    }
  }
}

}  // namespace

esp_err_t mqjs_0066_timers_start(mqjs_service_t* svc) {
  if (s_task) return ESP_OK;
  if (!svc) return ESP_ERR_INVALID_ARG;
  s_svc = svc;

  s_q = xQueueCreate(16, sizeof(Cmd));
  if (!s_q) return ESP_ERR_NO_MEM;

  memset(s_timers, 0, sizeof(s_timers));

  BaseType_t ok = xTaskCreate(&timers_task, "0066_js_tmr", 4096, nullptr, 8, &s_task);
  if (ok != pdPASS) {
    vQueueDelete(s_q);
    s_q = nullptr;
    s_task = nullptr;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

void mqjs_0066_timers_stop(void) {
  if (s_task) {
    vTaskDelete(s_task);
    s_task = nullptr;
  }
  if (s_q) {
    vQueueDelete(s_q);
    s_q = nullptr;
  }
  memset(s_timers, 0, sizeof(s_timers));
  s_svc = nullptr;
}

esp_err_t mqjs_0066_timers_schedule(uint32_t id, uint32_t delay_ms) {
  if (!s_q) return ESP_ERR_INVALID_STATE;
  if (id == 0) return ESP_ERR_INVALID_ARG;
  Cmd c = {};
  c.type = CMD_SCHEDULE;
  c.id = id;
  c.delay_ms = delay_ms;
  if (xQueueSend(s_q, &c, pdMS_TO_TICKS(50)) != pdTRUE) return ESP_ERR_TIMEOUT;
  return ESP_OK;
}

esp_err_t mqjs_0066_timers_cancel(uint32_t id) {
  if (!s_q) return ESP_ERR_INVALID_STATE;
  if (id == 0) return ESP_ERR_INVALID_ARG;
  Cmd c = {};
  c.type = CMD_CANCEL;
  c.id = id;
  if (xQueueSend(s_q, &c, pdMS_TO_TICKS(50)) != pdTRUE) return ESP_ERR_TIMEOUT;
  return ESP_OK;
}
