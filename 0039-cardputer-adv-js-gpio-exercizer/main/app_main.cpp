#include <memory>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "exercizer/ControlPlane.h"
#include "exercizer/GpioEngine.h"
#include "exercizer/I2cEngine.h"

#include "console/UsbSerialJtagConsole.h"
#include "eval/ModeSwitchingEvaluator.h"
#include "js_bindings.h"
#include "repl/LineEditor.h"
#include "repl/ReplLoop.h"

namespace {

constexpr char kTag[] = "js_gpio_exercizer";

struct ReplTaskContext {
  std::unique_ptr<IConsole> console;
  std::unique_ptr<IEvaluator> evaluator;
  std::unique_ptr<LineEditor> editor;
  std::unique_ptr<ReplLoop> repl;
};

struct ControlTaskContext {
  ControlPlane *ctrl;
  GpioEngine *gpio;
  I2cEngine *i2c;
};

void repl_task(void *pvParameters) {
  std::unique_ptr<ReplTaskContext> ctx(static_cast<ReplTaskContext *>(pvParameters));

  ctx->console->WriteString(
      "\n"
      "========================================\n"
      "  Cardputer-ADV JS GPIO Exercizer\n"
      "========================================\n");

  ctx->editor->PrintPrompt(*ctx->console);
  ctx->repl->Run(*ctx->console, *ctx->editor, *ctx->evaluator);

  vTaskDelete(nullptr);
}

void control_task(void *arg) {
  auto *ctx = static_cast<ControlTaskContext *>(arg);
  exercizer_ctrl_event_t ev = {};

  while (true) {
    if (!ctx->ctrl->Receive(&ev, portMAX_DELAY)) {
      continue;
    }

    switch (ev.type) {
      case EXERCIZER_CTRL_GPIO_SET:
      case EXERCIZER_CTRL_GPIO_STOP:
        ctx->gpio->HandleEvent(ev);
        break;
      case EXERCIZER_CTRL_I2C_CONFIG:
      case EXERCIZER_CTRL_I2C_TXN:
        ctx->i2c->HandleEvent(ev);
        break;
      default:
        break;
    }
  }
}

}  // namespace

static ControlPlane s_ctrl;
static GpioEngine s_gpio;
static I2cEngine s_i2c;
static ControlTaskContext s_control_ctx = {};

extern "C" void app_main(void) {
  ESP_LOGI(kTag, "Starting JS GPIO exercizer");

  esp_err_t timer_err = esp_timer_init();
  if (timer_err != ESP_OK && timer_err != ESP_ERR_INVALID_STATE) {
    ESP_ERROR_CHECK(timer_err);
  }

  if (!s_ctrl.Start()) {
    ESP_LOGE(kTag, "control plane failed to start");
    return;
  }

  exercizer_js_bind(&s_ctrl);

  if (!s_i2c.Start()) {
    ESP_LOGW(kTag, "i2c engine failed to start");
  }

  s_control_ctx.ctrl = &s_ctrl;
  s_control_ctx.gpio = &s_gpio;
  s_control_ctx.i2c = &s_i2c;
  xTaskCreate(&control_task, "exercizer_ctrl", 4096, &s_control_ctx, 5, nullptr);

  auto ctx = std::make_unique<ReplTaskContext>();
  ctx->console = std::make_unique<UsbSerialJtagConsole>();
  auto evaluator = std::make_unique<ModeSwitchingEvaluator>();
  std::string error;
  if (!evaluator->SetMode("js", &error)) {
    ESP_LOGW(kTag, "failed to set js mode: %s", error.c_str());
  }
  ctx->evaluator = std::move(evaluator);

  const char *prompt = ctx->evaluator->Prompt();
  ctx->editor = std::make_unique<LineEditor>(prompt ? prompt : "js> ");
  ctx->repl = std::make_unique<ReplLoop>();

  xTaskCreate(repl_task, "repl_task", 8192, ctx.release(), 5, nullptr);
}
