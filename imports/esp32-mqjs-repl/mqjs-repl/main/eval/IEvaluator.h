#pragma once

#include <string>
#include <string_view>

struct EvalResult {
  bool ok = false;
  std::string output;
};

class IEvaluator {
 public:
  virtual ~IEvaluator() = default;

  virtual const char* Name() const = 0;
  virtual EvalResult EvalLine(std::string_view line) = 0;
};

