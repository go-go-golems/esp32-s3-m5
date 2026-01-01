#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "eval/IEvaluator.h"

struct JSContext;

class JsEvaluator final : public IEvaluator {
 public:
  JsEvaluator();
  ~JsEvaluator() override;

  const char* Name() const override { return "js"; }
  const char* Prompt() const override { return "js> "; }
  EvalResult EvalLine(std::string_view line) override;
  bool Reset(std::string* error) override;
  bool GetStats(std::string* out) override;

 private:
  static constexpr size_t kJsMemSize = 64 * 1024;
  static uint8_t js_mem_buf_[kJsMemSize];

  JSContext* ctx_ = nullptr;
};
