#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "console/UartConsole.h"
#include "console/UsbSerialJtagConsole.h"
#include "eval/ModeSwitchingEvaluator.h"
#include "repl/LineEditor.h"
#include "repl/ReplLoop.h"

namespace {

constexpr char kTag[] = "mqjs-repl";

struct ReplTaskContext {
  std::unique_ptr<IConsole> console;
  std::unique_ptr<IEvaluator> evaluator;
  std::unique_ptr<LineEditor> editor;
  std::unique_ptr<ReplLoop> repl;
};

void repl_task(void* pvParameters) {
  std::unique_ptr<ReplTaskContext> ctx(static_cast<ReplTaskContext*>(pvParameters));

  ctx->console->WriteString(
      "\n"
      "========================================\n"
      "  ESP32-S3 REPL (bring-up)\n"
      "========================================\n");

  ctx->editor->PrintPrompt(*ctx->console);
  ctx->repl->Run(*ctx->console, *ctx->editor, *ctx->evaluator);

  vTaskDelete(nullptr);
}

}  // namespace

extern "C" void app_main(void) {
  ESP_LOGI(kTag, "Starting REPL firmware (mode switching)");

  auto ctx = std::make_unique<ReplTaskContext>();
#if CONFIG_MQJS_REPL_CONSOLE_USB_SERIAL_JTAG
  ctx->console = std::make_unique<UsbSerialJtagConsole>();
#else
  ctx->console = std::make_unique<UartConsole>(UART_NUM_0, 115200);
#endif
  ctx->evaluator = std::make_unique<ModeSwitchingEvaluator>();
  const char* prompt = ctx->evaluator->Prompt();
  ctx->editor = std::make_unique<LineEditor>(prompt ? prompt : "repl> ");
  ctx->repl = std::make_unique<ReplLoop>();

  xTaskCreate(
      repl_task,
      "repl_task",
      8192,
      ctx.release(),
      5,
      nullptr);
}
