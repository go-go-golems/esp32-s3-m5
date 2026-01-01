#include "repl/ReplLoop.h"

#include <string>

namespace {

bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string trim(const std::string& s) {
  size_t start = 0;
  while (start < s.size() && is_space(s[start])) {
    start++;
  }
  size_t end = s.size();
  while (end > start && is_space(s[end - 1])) {
    end--;
  }
  return s.substr(start, end - start);
}

}  // namespace

void ReplLoop::Run(IConsole& console, LineEditor& editor, IEvaluator& evaluator) {
  uint8_t buffer[128];

  while (true) {
    const int len = console.Read(buffer, sizeof(buffer), portMAX_DELAY);
    if (len <= 0) {
      continue;
    }

    for (int i = 0; i < len; i++) {
      const auto completed = editor.FeedByte(buffer[i], console);
      if (!completed.has_value()) {
        continue;
      }

      HandleLine(console, editor, evaluator, completed.value());
      editor.PrintPrompt(console);
    }
  }
}

void ReplLoop::HandleLine(IConsole& console, LineEditor& editor, IEvaluator& evaluator, const std::string& line) {
  const std::string stripped = trim(line);
  if (stripped.empty()) {
    return;
  }

  if (stripped == ":help") {
    console.WriteString(
        "Commands:\n"
        "  :help          Show this help\n"
        "  :mode          Show evaluator\n"
        "  :prompt TEXT   Set prompt\n");
    return;
  }

  if (stripped == ":mode") {
    console.WriteString("mode: ");
    console.WriteString(evaluator.Name());
    console.WriteString("\n");
    return;
  }

  constexpr char kPromptPrefix[] = ":prompt ";
  if (stripped.rfind(kPromptPrefix, 0) == 0) {
    editor.SetPrompt(stripped.substr(sizeof(kPromptPrefix) - 1));
    return;
  }

  const EvalResult result = evaluator.EvalLine(stripped);
  if (!result.output.empty()) {
    console.Write(reinterpret_cast<const uint8_t*>(result.output.data()),
                  static_cast<int>(result.output.size()));
  }
}
