#include "eval/RepeatEvaluator.h"

EvalResult RepeatEvaluator::EvalLine(std::string_view line) {
  EvalResult result;
  result.ok = true;
  result.output.assign(line.begin(), line.end());
  result.output.push_back('\n');
  return result;
}

