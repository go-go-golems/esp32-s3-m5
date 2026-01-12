#pragma once

#include "eval/IEvaluator.h"

class RepeatEvaluator final : public IEvaluator {
 public:
  const char* Name() const override { return "repeat"; }
  const char* Prompt() const override { return "repeat> "; }
  bool Reset(std::string* error) override {
    (void)error;
    return true;
  }
  EvalResult EvalLine(std::string_view line) override;
};
