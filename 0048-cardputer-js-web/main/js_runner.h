#pragma once

#include <string>

#include "esp_err.h"

esp_err_t js_runner_init(void);
std::string js_runner_eval_to_json(const char* code, size_t code_len);

