#include "js_service.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "cJSON.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "app_commands.h"
#include "mqjs_service.h"
#include "remote_client.h"

namespace {

static const char* TAG = "js_service_0074";

struct ServiceState {
  SemaphoreHandle_t mu = nullptr;
  mqjs_service_t* svc = nullptr;
  TaskHandle_t worker = nullptr;
  QueueHandle_t request_queue = nullptr;
  QueueHandle_t app_command_queue = nullptr;
  JsServiceStatus status = {};
};

struct BatchFlushResult {
  char* json = nullptr;
};

static ServiceState s_state = {};

uint64_t now_ms() {
  return static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
}

void lock() {
  xSemaphoreTake(s_state.mu, portMAX_DELAY);
}

void unlock() {
  xSemaphoreGive(s_state.mu);
}

void set_last_error_locked(const char* message) {
  std::snprintf(s_state.status.last_error, sizeof(s_state.status.last_error), "%s", message ? message : "");
}

esp_err_t send_script_console(uint32_t request_id, const char* level, const char* message) {
  return remote_client_send_script_console(request_id, level ? level : "info", message ? message : "", now_ms());
}

const char* result_status(bool ok, bool timed_out) {
  if (ok) {
    return "ok";
  }
  if (timed_out) {
    return "timed_out";
  }
  return "error";
}

esp_err_t job_bootstrap(JSContext* ctx, void* user) {
  (void)user;

  static const char* kBootstrap =
      "var g = globalThis;\n"
      "g.__lain = g.__lain || {};\n"
      "var __lain = g.__lain;\n"
      "__lain.cmds = [];\n"
      "__lain.logs = [];\n"
      "__lain.events = [];\n"
      "__lain.pushLog = function(level, argsLike) {\n"
      "  var parts = [];\n"
      "  for (var i = 0; i < argsLike.length; i++) parts.push(String(argsLike[i]));\n"
      "  __lain.logs.push({ level: String(level || 'info'), message: parts.join(' ') });\n"
      "};\n"
      "__lain.pushCmd = function(command, value, text) {\n"
      "  var entry = { command: String(command) };\n"
      "  if (value !== undefined && value !== null) entry.value = value | 0;\n"
      "  if (text !== undefined && text !== null) entry.text = String(text);\n"
      "  __lain.cmds.push(entry);\n"
      "  return entry;\n"
      "};\n"
      "__lain.typeToInt = function(type) {\n"
      "  if (typeof type === 'number') return type | 0;\n"
      "  type = String(type || '');\n"
      "  if (type === 'clear') return 1;\n"
      "  if (type === 'static') return 2;\n"
      "  if (type === 'hidden') return 3;\n"
      "  if (type === 'distorted') return 4;\n"
      "  return 0;\n"
      "};\n"
      "__lain.modeToInt = function(mode) {\n"
      "  if (typeof mode === 'number') return mode | 0;\n"
      "  mode = String(mode || 'debug');\n"
      "  return mode === 'radio' ? 1 : 0;\n"
      "};\n"
      "g.console = {\n"
      "  log: function() { __lain.pushLog('info', arguments); },\n"
      "  warn: function() { __lain.pushLog('warn', arguments); },\n"
      "  error: function() { __lain.pushLog('error', arguments); }\n"
      "};\n"
      "g.lain = {\n"
      "  message: function(text) { return __lain.pushCmd('show_message', null, text); },\n"
      "  position: function(value) { return __lain.pushCmd('set_position', value, null); },\n"
      "  mode: function(mode) { return __lain.pushCmd('set_mode', __lain.modeToInt(mode), null); },\n"
      "  band: function(name) { return __lain.pushCmd('set_band', null, name); },\n"
      "  station: function(pos, type, name) { return __lain.pushCmd('set_station', pos, __lain.typeToInt(type) + ':' + String(name)); },\n"
      "  reveal: function(text) { return __lain.pushCmd('show_reveal', null, text); },\n"
      "  emit: function(name, detail) { __lain.events.push({ name: String(name), detail: detail == null ? '' : String(detail) }); }\n"
      "};\n"
      "g.__lain_take_batches = function() {\n"
      "  var payload = JSON.stringify({ cmds: __lain.cmds, logs: __lain.logs, events: __lain.events });\n"
      "  __lain.cmds = [];\n"
      "  __lain.logs = [];\n"
      "  __lain.events = [];\n"
      "  return payload;\n"
      "};\n";

  JSValue value = JS_Eval(ctx, kBootstrap, std::strlen(kBootstrap), "<lain:bootstrap>", JS_EVAL_REPL);
  return JS_IsException(value) ? ESP_FAIL : ESP_OK;
}

esp_err_t job_take_batches(JSContext* ctx, void* user) {
  auto* result = static_cast<BatchFlushResult*>(user);
  if (!result) {
    return ESP_ERR_INVALID_ARG;
  }

  static const char* kExpr = "__lain_take_batches()";
  JSValue value = JS_Eval(ctx, kExpr, std::strlen(kExpr), "<lain:flush>", JS_EVAL_REPL | JS_EVAL_RETVAL);
  if (JS_IsException(value) || !JS_IsString(ctx, value)) {
    return ESP_FAIL;
  }

  JSCStringBuf buf = {};
  size_t len = 0;
  const char* str = JS_ToCStringLen(ctx, &len, value, &buf);
  if (!str) {
    return ESP_FAIL;
  }

  result->json = static_cast<char*>(malloc(len + 1));
  if (!result->json) {
    return ESP_ERR_NO_MEM;
  }
  std::memcpy(result->json, str, len);
  result->json[len] = '\0';
  return ESP_OK;
}

void process_batches(uint32_t request_id, const char* json) {
  if (!json || json[0] == '\0') {
    return;
  }

  cJSON* root = cJSON_Parse(json);
  if (!root) {
    (void)send_script_console(request_id, "error", "failed to parse lain batch payload");
    return;
  }

  const cJSON* logs = cJSON_GetObjectItemCaseSensitive(root, "logs");
  if (cJSON_IsArray(logs)) {
    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, logs) {
      const cJSON* level = cJSON_GetObjectItemCaseSensitive(item, "level");
      const cJSON* message = cJSON_GetObjectItemCaseSensitive(item, "message");
      (void)send_script_console(request_id,
                                cJSON_IsString(level) ? level->valuestring : "info",
                                cJSON_IsString(message) ? message->valuestring : "");
    }
  }

  const cJSON* events = cJSON_GetObjectItemCaseSensitive(root, "events");
  if (cJSON_IsArray(events)) {
    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, events) {
      const cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
      const cJSON* detail = cJSON_GetObjectItemCaseSensitive(item, "detail");
      (void)remote_client_send_script_event(request_id,
                                            cJSON_IsString(name) ? name->valuestring : "event",
                                            cJSON_IsString(detail) ? detail->valuestring : "",
                                            now_ms());
    }
  }

  const cJSON* cmds = cJSON_GetObjectItemCaseSensitive(root, "cmds");
  if (cJSON_IsArray(cmds) && s_state.app_command_queue) {
    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, cmds) {
      AppCommand command = {};
      if (!app_command_parse_js_batch_item(item, request_id, &command)) {
        (void)send_script_console(request_id, "warn", "ignored invalid lain command");
        continue;
      }
      if (xQueueSend(s_state.app_command_queue, &command, 0) != pdTRUE) {
        (void)send_script_console(request_id, "error", "app command queue full");
      }
    }
  }

  cJSON_Delete(root);
}

esp_err_t ensure_service_started_locked() {
  if (s_state.svc) {
    return ESP_OK;
  }
  if (!s_state.app_command_queue) {
    set_last_error_locked("app queue unset");
    return ESP_ERR_INVALID_STATE;
  }

  mqjs_service_config_t cfg = {};
  cfg.task_name = "lain_js_svc";
  cfg.task_stack_words = 6144;
  cfg.task_priority = 8;
  cfg.task_core_id = -1;
  cfg.queue_len = 16;
  cfg.arena_bytes = static_cast<size_t>(CONFIG_TUTORIAL_0074_JS_MEM_BYTES);
  cfg.stdlib = &js_stdlib;
  cfg.fix_global_this = true;

  esp_err_t err = mqjs_service_start(&cfg, &s_state.svc);
  if (err != ESP_OK) {
    set_last_error_locked(esp_err_to_name(err));
    return err;
  }

  const mqjs_job_t boot = {.fn = &job_bootstrap, .user = nullptr, .timeout_ms = 250};
  err = mqjs_service_run(s_state.svc, &boot);
  if (err != ESP_OK) {
    set_last_error_locked("bootstrap failed");
    return err;
  }

  s_state.status.started = true;
  set_last_error_locked("");
  return ESP_OK;
}

void js_worker_task(void* arg) {
  (void)arg;

  while (true) {
    ScriptEvalRequest request = {};
    if (xQueueReceive(s_state.request_queue, &request, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    lock();
    const bool remote_enabled = s_state.status.remote_enabled;
    s_state.status.last_request_id = request.request_id;
    unlock();

    if (!remote_enabled) {
      (void)remote_client_send_script_result(
          request.request_id, "rejected", false, false, "", "remote script execution is disabled", now_ms());
      continue;
    }

    mqjs_eval_result_t result = {};
    const uint32_t timeout_ms =
        request.timeout_ms == 0 ? static_cast<uint32_t>(CONFIG_TUTORIAL_0074_JS_TIMEOUT_MS) : request.timeout_ms;

    const esp_err_t eval_err = mqjs_service_eval(s_state.svc,
                                                 request.code,
                                                 std::strlen(request.code),
                                                 timeout_ms,
                                                 request.filename[0] ? request.filename : "<remote>",
                                                 &result);

    BatchFlushResult batches = {};
    const mqjs_job_t flush = {.fn = &job_take_batches, .user = &batches, .timeout_ms = 100};
    const esp_err_t flush_err = mqjs_service_run(s_state.svc, &flush);
    if (flush_err == ESP_OK && batches.json) {
      process_batches(request.request_id, batches.json);
      free(batches.json);
    }

    if (eval_err != ESP_OK) {
      (void)remote_client_send_script_result(
          request.request_id, "rejected", false, false, "", esp_err_to_name(eval_err), now_ms());
      lock();
      s_state.status.dropped_requests++;
      set_last_error_locked(esp_err_to_name(eval_err));
      unlock();
      continue;
    }

    (void)remote_client_send_script_result(request.request_id,
                                           result_status(result.ok, result.timed_out),
                                           result.ok,
                                           result.timed_out,
                                           result.output ? result.output : "",
                                           result.error ? result.error : "",
                                           now_ms());
    mqjs_eval_result_free(&result);

    lock();
    s_state.status.completed_requests++;
    set_last_error_locked("");
    unlock();
  }
}

}  // namespace

esp_err_t js_service_start(QueueHandle_t app_command_queue) {
  if (!app_command_queue) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!s_state.mu) {
    s_state.mu = xSemaphoreCreateMutex();
    if (!s_state.mu) {
      return ESP_ERR_NO_MEM;
    }
  }

  lock();
  s_state.app_command_queue = app_command_queue;

  if (!s_state.request_queue) {
    s_state.request_queue = xQueueCreate(CONFIG_TUTORIAL_0074_JS_QUEUE_LEN, sizeof(ScriptEvalRequest));
    if (!s_state.request_queue) {
      unlock();
      return ESP_ERR_NO_MEM;
    }
  }

  esp_err_t err = ensure_service_started_locked();
  if (err != ESP_OK) {
    unlock();
    return err;
  }

  if (!s_state.worker) {
    if (xTaskCreatePinnedToCore(js_worker_task, "lain_js_worker", 6144, nullptr, 5, &s_state.worker, 0) != pdPASS) {
      unlock();
      return ESP_ERR_NO_MEM;
    }
  }

  unlock();
  ESP_LOGI(TAG, "js service ready");
  return ESP_OK;
}

esp_err_t js_service_submit(const ScriptEvalRequest* request) {
  if (!request) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!s_state.mu) {
    return ESP_ERR_INVALID_STATE;
  }

  lock();
  const bool ready = s_state.request_queue != nullptr && s_state.svc != nullptr;
  if (!ready) {
    unlock();
    return ESP_ERR_INVALID_STATE;
  }
  if (xQueueSend(s_state.request_queue, request, 0) != pdTRUE) {
    s_state.status.dropped_requests++;
    set_last_error_locked("script queue full");
    unlock();
    return ESP_ERR_NO_MEM;
  }
  s_state.status.queued_requests++;
  unlock();
  return ESP_OK;
}

void js_service_set_remote_enabled(bool enabled) {
  if (!s_state.mu) {
    return;
  }
  lock();
  s_state.status.remote_enabled = enabled;
  unlock();
}

bool js_service_remote_enabled() {
  if (!s_state.mu) {
    return false;
  }
  lock();
  const bool enabled = s_state.status.remote_enabled;
  unlock();
  return enabled;
}

void js_service_get_status(JsServiceStatus* out) {
  if (!out) {
    return;
  }
  if (!s_state.mu) {
    *out = {};
    return;
  }
  lock();
  *out = s_state.status;
  unlock();
}
