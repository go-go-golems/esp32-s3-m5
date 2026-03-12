#include "mqjs_timers.h"

extern "C" {
#include "mquickjs.h"
}

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "sdkconfig.h"

#include "js_service.h"
#include "mqjs_service.h"
#include "mqjs_vm.h"

namespace {

static const char* TAG = "0074_js_timers";
constexpr uint32_t kMaxTimers = CONFIG_TUTORIAL_0074_JS_MAX_TIMERS;

enum CmdType : uint8_t {
  kCmdSchedule = 1,
  kCmdCancel = 2,
  kCmdCancelAll = 3,
};

struct Cmd {
  CmdType type = kCmdSchedule;
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

mqjs_service_t* s_service = nullptr;
TaskHandle_t s_task = nullptr;
QueueHandle_t s_queue = nullptr;
TimerSlot s_timers[kMaxTimers];

void clear_slots() {
  for (uint32_t i = 0; i < kMaxTimers; ++i) {
    s_timers[i] = TimerSlot{};
  }
}

int64_t now_us() {
  return esp_timer_get_time();
}

int find_slot_by_id(uint32_t id) {
  for (uint32_t i = 0; i < kMaxTimers; ++i) {
    if (s_timers[i].active && s_timers[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int find_free_slot() {
  for (uint32_t i = 0; i < kMaxTimers; ++i) {
    if (!s_timers[i].active) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int64_t next_due_us() {
  int64_t best = 0;
  for (uint32_t i = 0; i < kMaxTimers; ++i) {
    if (!s_timers[i].active) {
      continue;
    }
    if (best == 0 || s_timers[i].due_us < best) {
      best = s_timers[i].due_us;
    }
  }
  return best;
}

void log_js_exception(JSContext* ctx, const char* what) {
  if (!ctx) {
    return;
  }
  MqjsVm* vm = MqjsVm::From(ctx);
  const std::string detail = vm ? vm->GetExceptionString(JS_DUMP_LONG) : "<exception>";
  ESP_LOGW(TAG, "%s: %s", what ? what : "js exception", detail.c_str());
}

JSValue get_timer_cb(JSContext* ctx, uint32_t id, JSValue* out_cb_table) {
  if (out_cb_table) {
    *out_cb_table = JS_UNDEFINED;
  }
  if (!ctx) {
    return JS_UNDEFINED;
  }

  JSValue glob = JS_GetGlobalObject(ctx);
  JSValue ns = JS_GetPropertyStr(ctx, glob, "__lain");
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

esp_err_t job_fire_timeout(JSContext* ctx, void* user) {
  auto* arg = static_cast<FireArg*>(user);
  const uint32_t id = arg ? arg->id : 0;
  free(arg);

  if (!ctx || id == 0) {
    return ESP_OK;
  }

  JSValue cb_table = JS_UNDEFINED;
  JSValue cb = get_timer_cb(ctx, id, &cb_table);
  if (JS_IsUndefined(cb) || JS_IsNull(cb) || !JS_IsFunction(ctx, cb)) {
    return ESP_OK;
  }

  JS_PushArg(ctx, cb);
  JS_PushArg(ctx, JS_NULL);
  JSValue ret = JS_Call(ctx, 0);
  const bool threw = JS_IsException(ret);

  (void)JS_SetPropertyUint32(ctx, cb_table, id, JS_NULL);
  (void)js_service_flush_batches_from_context(ctx, 0);
  if (threw) {
    log_js_exception(ctx, "timer callback threw");
  }

  return ESP_OK;
}

void post_fire(uint32_t id) {
  if (!s_service || id == 0) {
    return;
  }

  auto* arg = static_cast<FireArg*>(calloc(1, sizeof(FireArg)));
  if (!arg) {
    return;
  }
  arg->id = id;

  mqjs_job_t job = {};
  job.fn = &job_fire_timeout;
  job.user = arg;
  job.timeout_ms = CONFIG_TUTORIAL_0074_JS_TIMEOUT_MS;

  const esp_err_t err = mqjs_service_post(s_service, &job);
  if (err != ESP_OK) {
    free(arg);
    ESP_LOGW(TAG, "mqjs_service_post(timer) failed: %s", esp_err_to_name(err));
  }
}

void handle_cmd(const Cmd& cmd) {
  if (cmd.type == kCmdCancelAll) {
    clear_slots();
    return;
  }

  if (cmd.id == 0) {
    return;
  }

  if (cmd.type == kCmdCancel) {
    const int idx = find_slot_by_id(cmd.id);
    if (idx >= 0) {
      s_timers[idx].active = false;
    }
    return;
  }

  int idx = find_slot_by_id(cmd.id);
  if (idx < 0) {
    idx = find_free_slot();
  }
  if (idx < 0) {
    ESP_LOGW(TAG, "no timer slots left (max=%u)", static_cast<unsigned>(kMaxTimers));
    return;
  }

  s_timers[idx].active = true;
  s_timers[idx].id = cmd.id;
  s_timers[idx].due_us = now_us() + static_cast<int64_t>(cmd.delay_ms) * 1000;
}

void timers_task(void* arg) {
  (void)arg;
  ESP_LOGI(TAG, "task start (max_timers=%u)", static_cast<unsigned>(kMaxTimers));

  while (true) {
    const int64_t due = next_due_us();
    int64_t wait_ms = 1000;
    if (due != 0) {
      const int64_t now = now_us();
      wait_ms = (due <= now) ? 0 : ((due - now) / 1000);
      if (wait_ms < 0) {
        wait_ms = 0;
      }
      if (wait_ms > 1000) {
        wait_ms = 1000;
      }
    }

    Cmd cmd = {};
    const TickType_t timeout = (wait_ms == 0) ? 0 : pdMS_TO_TICKS(static_cast<uint32_t>(wait_ms));
    if (xQueueReceive(s_queue, &cmd, timeout) == pdTRUE) {
      handle_cmd(cmd);
      continue;
    }

    const int64_t now = now_us();
    for (uint32_t i = 0; i < kMaxTimers; ++i) {
      if (!s_timers[i].active || s_timers[i].due_us > now) {
        continue;
      }
      const uint32_t id = s_timers[i].id;
      s_timers[i].active = false;
      post_fire(id);
    }
  }
}

}  // namespace

esp_err_t mqjs_0074_timers_start(mqjs_service_t* svc) {
  if (s_task) {
    return ESP_OK;
  }
  if (!svc) {
    return ESP_ERR_INVALID_ARG;
  }

  s_service = svc;
  s_queue = xQueueCreate(16, sizeof(Cmd));
  if (!s_queue) {
    return ESP_ERR_NO_MEM;
  }

  clear_slots();
  if (xTaskCreate(&timers_task, "0074_js_tmr", 4096, nullptr, 8, &s_task) != pdPASS) {
    vQueueDelete(s_queue);
    s_queue = nullptr;
    s_service = nullptr;
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

void mqjs_0074_timers_stop(void) {
  if (s_task) {
    vTaskDelete(s_task);
    s_task = nullptr;
  }
  if (s_queue) {
    vQueueDelete(s_queue);
    s_queue = nullptr;
  }
  clear_slots();
  s_service = nullptr;
}

esp_err_t mqjs_0074_timers_schedule(uint32_t id, uint32_t delay_ms) {
  if (!s_queue) {
    return ESP_ERR_INVALID_STATE;
  }
  if (id == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  Cmd cmd = {};
  cmd.type = kCmdSchedule;
  cmd.id = id;
  cmd.delay_ms = delay_ms;
  if (xQueueSend(s_queue, &cmd, pdMS_TO_TICKS(50)) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}

esp_err_t mqjs_0074_timers_cancel(uint32_t id) {
  if (!s_queue) {
    return ESP_ERR_INVALID_STATE;
  }
  if (id == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  Cmd cmd = {};
  cmd.type = kCmdCancel;
  cmd.id = id;
  if (xQueueSend(s_queue, &cmd, pdMS_TO_TICKS(50)) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}

esp_err_t mqjs_0074_timers_cancel_all(void) {
  if (!s_queue) {
    return ESP_ERR_INVALID_STATE;
  }

  Cmd cmd = {};
  cmd.type = kCmdCancelAll;
  if (xQueueSend(s_queue, &cmd, pdMS_TO_TICKS(50)) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }
  return ESP_OK;
}
