#pragma once

#include "console/IConsole.h"
#include "eval/IEvaluator.h"
#include "repl/LineEditor.h"

class ReplLoop {
 public:
  void Run(IConsole& console, LineEditor& editor, IEvaluator& evaluator);

 private:
  void HandleLine(IConsole& console, LineEditor& editor, IEvaluator& evaluator, const std::string& line);
};

