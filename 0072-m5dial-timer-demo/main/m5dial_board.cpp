#include "m5dial_board.h"

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_timer.h>

namespace tutorial_0072 {

namespace {

static const char *TAG = "m5dial_board";

constexpr gpio_num_t kPinPowerHold = GPIO_NUM_46;
constexpr gpio_num_t kPinButton = GPIO_NUM_42;
constexpr gpio_num_t kPinEncoderA = GPIO_NUM_41;
constexpr gpio_num_t kPinEncoderB = GPIO_NUM_40;

constexpr int kPinLcdMosi = 5;
constexpr int kPinLcdMiso = -1;
constexpr int kPinLcdSclk = 6;
constexpr int kPinLcdDc = 4;
constexpr int kPinLcdCs = 7;
constexpr int kPinLcdRst = 8;
constexpr int kPinLcdBl = 9;

constexpr int kPinTouchSda = 11;
constexpr int kPinTouchScl = 12;
constexpr int kPinTouchInt = 14;
constexpr int kTouchAddress = 0x38;

constexpr int64_t kButtonDebounceMs = 30;

int64_t now_ms() {
  return esp_timer_get_time() / 1000LL;
}

}  // namespace

LGFX_M5Dial::LGFX_M5Dial() {
  {
    auto cfg = bus_.config();
    cfg.spi_host = SPI3_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 80000000;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = true;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = kPinLcdSclk;
    cfg.pin_mosi = kPinLcdMosi;
    cfg.pin_miso = kPinLcdMiso;
    cfg.pin_dc = kPinLcdDc;
    bus_.config(cfg);
    panel_.setBus(&bus_);
  }

  {
    auto cfg = panel_.config();
    cfg.pin_cs = kPinLcdCs;
    cfg.pin_rst = kPinLcdRst;
    cfg.pin_busy = -1;
    cfg.panel_width = 240;
    cfg.panel_height = 240;
    cfg.memory_width = 240;
    cfg.memory_height = 240;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = true;
    cfg.invert = true;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    panel_.config(cfg);
  }

  {
    auto cfg = light_.config();
    cfg.pin_bl = kPinLcdBl;
    cfg.invert = false;
    cfg.freq = 44100;
    cfg.pwm_channel = 7;
    light_.config(cfg);
    panel_.setLight(&light_);
  }

  {
    auto cfg = touch_.config();
    cfg.x_min = 0;
    cfg.x_max = 239;
    cfg.y_min = 0;
    cfg.y_max = 239;
    cfg.pin_int = kPinTouchInt;
    cfg.pin_rst = kPinLcdRst;
    cfg.offset_rotation = 0;
    cfg.bus_shared = false;
    cfg.i2c_port = I2C_NUM_0;
    cfg.i2c_addr = kTouchAddress;
    cfg.pin_sda = kPinTouchSda;
    cfg.pin_scl = kPinTouchScl;
    cfg.freq = 400000;
    touch_.config(cfg);
    panel_.setTouch(&touch_);
  }

  setPanel(&panel_);
}

bool M5DialBoard::init() {
  init_power_hold();
  init_encoder_button_gpio();

  if (!display_.init()) {
    ESP_LOGE(TAG, "display init failed");
    return false;
  }

  display_.setColorDepth(16);
  display_.setRotation(0);
  display_.setBrightness(255);
  display_.fillScreen(TFT_BLACK);

  encoder_prev_state_ =
      (uint8_t)((gpio_get_level(kPinEncoderA) ? 0x2 : 0) | (gpio_get_level(kPinEncoderB) ? 0x1 : 0));

  ESP_LOGI(TAG, "board init complete");
  return true;
}

void M5DialBoard::init_power_hold() {
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = 1ULL << kPinPowerHold;
  cfg.mode = GPIO_MODE_OUTPUT;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);
  gpio_set_level(kPinPowerHold, 1);
}

void M5DialBoard::init_encoder_button_gpio() {
  gpio_config_t input_cfg = {};
  input_cfg.pin_bit_mask = (1ULL << kPinButton) | (1ULL << kPinEncoderA) | (1ULL << kPinEncoderB);
  input_cfg.mode = GPIO_MODE_INPUT;
  input_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  input_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  input_cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&input_cfg);

  button_raw_state_ = gpio_get_level(kPinButton);
  button_stable_state_ = button_raw_state_;
  button_last_change_ms_ = now_ms();
}

void M5DialBoard::poll() {
  static constexpr int8_t kTransitionTable[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0,
  };

  const uint8_t encoder_state =
      (uint8_t)((gpio_get_level(kPinEncoderA) ? 0x2 : 0) | (gpio_get_level(kPinEncoderB) ? 0x1 : 0));
  const uint8_t transition = (uint8_t)((encoder_prev_state_ << 2) | encoder_state);
  encoder_prev_state_ = encoder_state;

  const int8_t transition_delta = kTransitionTable[transition];
  if (transition_delta != 0) {
    encoder_substep_accum_ += transition_delta;
    while (encoder_substep_accum_ >= 2) {
      encoder_steps_pending_++;
      encoder_substep_accum_ -= 2;
    }
    while (encoder_substep_accum_ <= -2) {
      encoder_steps_pending_--;
      encoder_substep_accum_ += 2;
    }
  }

  const bool button_raw_now = gpio_get_level(kPinButton);
  if (button_raw_now != button_raw_state_) {
    button_raw_state_ = button_raw_now;
    button_last_change_ms_ = now_ms();
  }

  if ((now_ms() - button_last_change_ms_) >= kButtonDebounceMs && button_stable_state_ != button_raw_state_) {
    const bool previous = button_stable_state_;
    button_stable_state_ = button_raw_state_;
    if (previous && !button_stable_state_) {
      button_press_pending_ = true;
    }
  }
}

int M5DialBoard::take_encoder_steps() {
  const int steps = encoder_steps_pending_;
  encoder_steps_pending_ = 0;
  return steps;
}

bool M5DialBoard::take_button_press() {
  const bool pressed = button_press_pending_;
  button_press_pending_ = false;
  return pressed;
}

TouchState M5DialBoard::read_touch() {
  TouchState state = {};
  uint16_t x = 0;
  uint16_t y = 0;
  if (display_.getTouch(&x, &y)) {
    state.pressed = true;
    state.x = static_cast<int16_t>(x);
    state.y = static_cast<int16_t>(y);
  }
  return state;
}

}  // namespace tutorial_0072
