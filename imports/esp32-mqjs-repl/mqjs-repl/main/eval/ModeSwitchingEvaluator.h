#pragma once

#include <optional>
#include <string_view>

#include "eval/IEvaluator.h"
#include "eval/JsEvaluator.h"
#include "eval/RepeatEvaluator.h"

class ModeSwitchingEvaluator final : public IEvaluator {
 public:
  ModeSwitchingEvaluator();

  const char* Name() const override;
  const char* Prompt() const override;
  EvalResult EvalLine(std::string_view line) override;

  const char* ModeHelp() const override { return "repeat|js"; }
  bool SetMode(std::string_view mode, std::string* error) override;
  bool Reset(std::string* error) override;
  bool GetStats(std::string* out) override;

 private:
  RepeatEvaluator repeat_;
  std::optional<JsEvaluator> js_;

  IEvaluator* current_ = &repeat_;
};
