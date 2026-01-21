#include "js_runner.h"

#include "js_service.h"

esp_err_t js_runner_init(void) {
  return js_service_start();
}

std::string js_runner_eval_to_json(const char* code, size_t code_len) {
  return js_service_eval_to_json(code, code_len, 0, "<legacy>");
}
