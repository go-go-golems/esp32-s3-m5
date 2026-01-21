#include "js_runner.h"

#include <cstring>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "js_runner_0048";

namespace {

struct EvalDeadline {
  int64_t deadline_us = 0;
};

static uint8_t* s_js_mem = nullptr;
static JSContext* s_ctx = nullptr;
static SemaphoreHandle_t s_mu = nullptr;
static EvalDeadline s_deadline;

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

std::string print_value(JSContext* ctx, JSValue v) {
  std::string out;
  JS_SetContextOpaque(ctx, &out);
  JS_SetLogFunc(ctx, write_to_string);
  JS_PrintValueF(ctx, v, JS_DUMP_LONG);
  return out;
}

}  // namespace

esp_err_t js_runner_init(void) {
  if (s_ctx) return ESP_OK;

  if (!s_mu) {
    s_mu = xSemaphoreCreateMutex();
    if (!s_mu) return ESP_ERR_NO_MEM;
  }

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

  ESP_LOGI(TAG, "js initialized (mem=%d)", mem_bytes);
  return ESP_OK;
}

std::string js_runner_eval_to_json(const char* code, size_t code_len) {
  std::string json;

  if (!code || code_len == 0) {
    return "{\"ok\":false,\"output\":\"\",\"error\":\"empty body\",\"timed_out\":false}";
  }
  if (!s_ctx) {
    const esp_err_t err = js_runner_init();
    if (err != ESP_OK) {
      return "{\"ok\":false,\"output\":\"\",\"error\":\"js init failed\",\"timed_out\":false}";
    }
  }

  xSemaphoreTake(s_mu, portMAX_DELAY);

  const int timeout_ms = CONFIG_TUTORIAL_0048_JS_TIMEOUT_MS;
  if (timeout_ms > 0) {
    s_deadline.deadline_us = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
  } else {
    s_deadline.deadline_us = 0;
  }

  const int flags = JS_EVAL_REPL | JS_EVAL_RETVAL;
  JSValue val = JS_Eval(s_ctx, code, code_len, "<http>", flags);

  const bool timed_out = (timeout_ms > 0) && (esp_timer_get_time() > s_deadline.deadline_us);
  s_deadline.deadline_us = 0;

  if (JS_IsException(val)) {
    JSValue exc = JS_GetException(s_ctx);
    std::string err = print_value(s_ctx, exc);
    json = "{\"ok\":false,\"output\":\"\",\"error\":\"";
    for (char c : err) {
      if (c == '\\' || c == '\"') json.push_back('\\');
      if (c == '\n') {
        json += "\\n";
      } else {
        json.push_back(c);
      }
    }
    json += "\",\"timed_out\":";
    json += timed_out ? "true" : "false";
    json += "}";
    xSemaphoreGive(s_mu);
    return json;
  }

  std::string out;
  if (!JS_IsUndefined(val)) {
    out = print_value(s_ctx, val);
    out.push_back('\n');
  }

  json = "{\"ok\":true,\"output\":\"";
  for (char c : out) {
    if (c == '\\' || c == '\"') json.push_back('\\');
    if (c == '\n') {
      json += "\\n";
    } else {
      json.push_back(c);
    }
  }
  json += "\",\"error\":null,\"timed_out\":";
  json += timed_out ? "true" : "false";
  json += "}";

  xSemaphoreGive(s_mu);
  return json;
}
