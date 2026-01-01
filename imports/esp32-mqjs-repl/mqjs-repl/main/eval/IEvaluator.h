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
  virtual const char* Prompt() const { return nullptr; }
  virtual EvalResult EvalLine(std::string_view line) = 0;

  virtual const char* ModeHelp() const { return nullptr; }
  virtual bool SetMode(std::string_view mode, std::string* error) {
    (void)mode;
    if (error) {
      *error = "mode switching is not supported";
    }
    return false;
  }

  virtual bool Reset(std::string* error) {
    if (error) {
      *error = "reset is not supported";
    }
    return false;
  }

  virtual bool GetStats(std::string* out) {
    (void)out;
    return false;
  }
};
