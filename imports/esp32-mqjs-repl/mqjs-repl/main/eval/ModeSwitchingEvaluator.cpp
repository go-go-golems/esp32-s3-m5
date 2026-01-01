#include "eval/ModeSwitchingEvaluator.h"

#include <string>

namespace {

std::string_view trim(std::string_view s) {
  while (!s.empty()) {
    const char c = s.front();
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      break;
    }
    s.remove_prefix(1);
  }
  while (!s.empty()) {
    const char c = s.back();
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      break;
    }
    s.remove_suffix(1);
  }
  return s;
}

}  // namespace

ModeSwitchingEvaluator::ModeSwitchingEvaluator() = default;

const char* ModeSwitchingEvaluator::Name() const {
  return current_ ? current_->Name() : "unknown";
}

const char* ModeSwitchingEvaluator::Prompt() const {
  return current_ ? current_->Prompt() : nullptr;
}

EvalResult ModeSwitchingEvaluator::EvalLine(std::string_view line) {
  if (!current_) {
    EvalResult result;
    result.ok = false;
    result.output = "error: no evaluator selected\n";
    return result;
  }
  return current_->EvalLine(line);
}

bool ModeSwitchingEvaluator::SetMode(std::string_view mode, std::string* error) {
  mode = trim(mode);
  if (mode == "repeat") {
    current_ = &repeat_;
    if (error) {
      error->clear();
    }
    return true;
  }
  if (mode == "js") {
    if (!js_.has_value()) {
      js_.emplace();
    }
    current_ = &js_.value();
    if (error) {
      error->clear();
    }
    return true;
  }

  if (error) {
    *error = "unknown mode (expected: repeat|js)";
  }
  return false;
}

bool ModeSwitchingEvaluator::Reset(std::string* error) {
  if (!current_) {
    if (error) {
      *error = "no evaluator selected";
    }
    return false;
  }
  return current_->Reset(error);
}

bool ModeSwitchingEvaluator::GetStats(std::string* out) {
  if (!current_) {
    return false;
  }
  return current_->GetStats(out);
}

bool ModeSwitchingEvaluator::Autoload(bool format_if_mount_failed, std::string* out, std::string* error) {
  if (!current_) {
    if (error) {
      *error = "no evaluator selected";
    }
    return false;
  }
  return current_->Autoload(format_if_mount_failed, out, error);
}
