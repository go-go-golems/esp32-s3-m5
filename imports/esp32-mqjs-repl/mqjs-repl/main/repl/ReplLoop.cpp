#include "repl/ReplLoop.h"

#include <cstdarg>
#include <cstdio>
#include <cinttypes>
#include <string>

#include "esp_heap_caps.h"
#include "esp_system.h"

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

void console_printf(IConsole& console, const char* fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) {
    return;
  }
  console.Write(reinterpret_cast<const uint8_t*>(buf),
                (n < static_cast<int>(sizeof(buf))) ? n : static_cast<int>(sizeof(buf)));
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
    const char* mode_help = evaluator.ModeHelp();
    console.WriteString(
        "Commands:\n"
        "  :help          Show this help\n"
        "  :mode          Show evaluator\n");
    if (mode_help) {
      console.WriteString("  :mode NAME     Set evaluator (");
      console.WriteString(mode_help);
      console.WriteString(")\n");
    }
    console.WriteString(
        "  :reset         Reset current evaluator\n"
        "  :stats         Print memory stats\n"
        "  :prompt TEXT   Set prompt\n");
    return;
  }

  if (stripped == ":mode" || stripped.rfind(":mode ", 0) == 0) {
    if (stripped != ":mode") {
      const std::string mode = trim(stripped.substr(sizeof(":mode ") - 1));
      std::string error;
      if (!evaluator.SetMode(mode, &error)) {
        console.WriteString("error: ");
        console.WriteString(error.c_str());
        console.WriteString("\n");
        return;
      }

      if (const char* prompt = evaluator.Prompt()) {
        editor.SetPrompt(prompt);
      }
    }
    console.WriteString("mode: ");
    console.WriteString(evaluator.Name());
    console.WriteString("\n");
    return;
  }

  if (stripped == ":reset") {
    std::string error;
    if (!evaluator.Reset(&error)) {
      console.WriteString("error: ");
      console.WriteString(error.c_str());
      console.WriteString("\n");
      return;
    }
    console.WriteString("ok\n");
    return;
  }

  if (stripped == ":stats") {
    const uint32_t free_heap = esp_get_free_heap_size();
    const uint32_t min_free_heap = esp_get_minimum_free_heap_size();
    const size_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t min_free_8bit = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    const size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const size_t min_free_spiram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);

    console_printf(console, "heap_free=%" PRIu32 " heap_min_free=%" PRIu32 "\n", free_heap, min_free_heap);
    console_printf(console, "heap_8bit_free=%u heap_8bit_min_free=%u\n",
                   static_cast<unsigned>(free_8bit),
                   static_cast<unsigned>(min_free_8bit));
    console_printf(console, "heap_spiram_free=%u heap_spiram_min_free=%u\n",
                   static_cast<unsigned>(free_spiram),
                   static_cast<unsigned>(min_free_spiram));

    std::string stats;
    if (evaluator.GetStats(&stats) && !stats.empty()) {
      console.Write(reinterpret_cast<const uint8_t*>(stats.data()),
                    static_cast<int>(stats.size()));
      if (stats.back() != '\n') {
        console.WriteString("\n");
      }
    }

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
