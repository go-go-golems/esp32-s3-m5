#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_err.h"

#include "sdkconfig.h"

struct ScriptEvalRequest {
  uint32_t request_id = 0;
  uint32_t timeout_ms = 0;
  char filename[32] = {0};
  char code[CONFIG_TUTORIAL_0074_JS_MAX_BODY + 1] = {0};
};

struct JsServiceStatus {
  bool started = false;
  bool remote_enabled = false;
  uint32_t queued_requests = 0;
  uint32_t completed_requests = 0;
  uint32_t dropped_requests = 0;
  uint32_t last_request_id = 0;
  char last_error[96] = {0};
};

esp_err_t js_service_start(QueueHandle_t app_command_queue);
esp_err_t js_service_submit(const ScriptEvalRequest* request);
void js_service_set_remote_enabled(bool enabled);
bool js_service_remote_enabled();
void js_service_get_status(JsServiceStatus* out);
