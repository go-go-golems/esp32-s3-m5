#include "m5dial_board.h"

#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>

namespace tutorial_0072 {

namespace {

static const char *TAG = "m5dial_board";

constexpr gpio_num_t kPinPowerHold = GPIO_NUM_46;
constexpr gpio_num_t kPinButton = GPIO_NUM_42;
constexpr gpio_num_t kPinEncoderA = GPIO_NUM_41;
constexpr gpio_num_t kPinEncoderB = GPIO_NUM_40;
constexpr gpio_num_t kPinTouchInt = GPIO_NUM_14;

constexpr int kPinLcdMosi = 5;
constexpr int kPinLcdMiso = -1;
constexpr int kPinLcdSclk = 6;
constexpr int kPinLcdDc = 4;
constexpr int kPinLcdCs = 7;
constexpr int kPinLcdRst = 8;
constexpr int kPinLcdBl = 9;
constexpr int kPinTouchSda = 11;
constexpr int kPinTouchScl = 12;

constexpr i2c_port_t kTouchI2cPort = I2C_NUM_0;
constexpr uint8_t kTouchI2cAddress = 0x38;
constexpr uint8_t kTouchRegPoints = 0x02;
constexpr uint8_t kTouchRegPoint1 = 0x03;

constexpr int64_t kButtonDebounceMs = 30;
constexpr int64_t kButtonLongPressMs = 700;
constexpr int kSwipeThresholdPx = 36;
constexpr int kSwipeAxisLeadPx = 14;

int64_t now_ms() {
  return esp_timer_get_time() / 1000LL;
}

esp_err_t touch_write_reg(uint8_t reg, uint8_t value) {
  uint8_t payload[2] = {reg, value};
  return i2c_master_write_to_device(kTouchI2cPort, kTouchI2cAddress, payload, sizeof(payload), pdMS_TO_TICKS(200));
}

esp_err_t touch_read_reg(uint8_t reg, uint8_t *data, size_t size) {
  return i2c_master_write_read_device(kTouchI2cPort, kTouchI2cAddress, &reg, 1, data, size, pdMS_TO_TICKS(200));
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
    cfg.bus_shared = true;
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

  touch_ready_ = init_touch_controller();

  encoder_.attachHalfQuad(static_cast<int>(kPinEncoderA), static_cast<int>(kPinEncoderB));
  encoder_.setFilter(1023);
  encoder_.clearCount();
  encoder_last_count_ = encoder_.readCount();

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
  input_cfg.pin_bit_mask = (1ULL << kPinButton);
  input_cfg.mode = GPIO_MODE_INPUT;
  input_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  input_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  input_cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&input_cfg);

  button_raw_state_ = gpio_get_level(kPinButton);
  button_stable_state_ = button_raw_state_;
  button_last_change_ms_ = now_ms();
  button_pressed_ms_ = button_last_change_ms_;
}

void M5DialBoard::poll() {
  const int encoder_now = encoder_.readCount();
  const int encoder_delta = encoder_now - encoder_last_count_;
  if (encoder_delta != 0) {
    encoder_steps_pending_ += encoder_delta;
    encoder_last_count_ = encoder_now;
  }

  const bool button_raw_now = gpio_get_level(kPinButton);
  if (button_raw_now != button_raw_state_) {
    button_raw_state_ = button_raw_now;
    button_last_change_ms_ = now_ms();
  }

  if ((now_ms() - button_last_change_ms_) >= kButtonDebounceMs && button_stable_state_ != button_raw_state_) {
    const int64_t stable_change_ms = now_ms();
    const bool previous = button_stable_state_;
    button_stable_state_ = button_raw_state_;
    if (previous && !button_stable_state_) {
      button_pressed_ms_ = stable_change_ms;
      button_long_press_reported_ = false;
    } else if (!previous && button_stable_state_ && !button_long_press_reported_) {
      button_press_pending_ = true;
    }
  }

  if (!button_stable_state_ && !button_long_press_reported_ && (now_ms() - button_pressed_ms_) >= kButtonLongPressMs) {
    button_long_press_pending_ = true;
    button_long_press_reported_ = true;
  }

  poll_touch();
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

bool M5DialBoard::take_button_long_press() {
  const bool pressed = button_long_press_pending_;
  button_long_press_pending_ = false;
  return pressed;
}

TouchState M5DialBoard::read_touch() {
  return touch_state_;
}

SwipeDirection M5DialBoard::take_swipe() {
  const SwipeDirection swipe = swipe_pending_;
  swipe_pending_ = SwipeDirection::kNone;
  return swipe;
}

bool M5DialBoard::init_touch_controller() {
  i2c_config_t cfg = {};
  cfg.mode = I2C_MODE_MASTER;
  cfg.sda_io_num = static_cast<gpio_num_t>(kPinTouchSda);
  cfg.scl_io_num = static_cast<gpio_num_t>(kPinTouchScl);
  cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
  cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
  cfg.master.clk_speed = 100000;
  cfg.clk_flags = 0;

  esp_err_t err = i2c_param_config(kTouchI2cPort, &cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "touch i2c config failed: %s", esp_err_to_name(err));
    return false;
  }

  err = i2c_driver_install(kTouchI2cPort, I2C_MODE_MASTER, 0, 0, 0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "touch i2c driver install failed: %s", esp_err_to_name(err));
    return false;
  }

  gpio_reset_pin(kPinTouchInt);
  gpio_set_direction(kPinTouchInt, GPIO_MODE_INPUT);
  gpio_set_pull_mode(kPinTouchInt, GPIO_PULLUP_ONLY);

  // Mirror the working FT3267 initialization sequence from M5Dial-UserDemo.
  touch_write_reg(0x80, 70);
  touch_write_reg(0x81, 60);
  touch_write_reg(0x82, 16);
  touch_write_reg(0x83, 60);
  touch_write_reg(0x84, 10);
  touch_write_reg(0x85, 20);
  touch_write_reg(0x87, 2);
  touch_write_reg(0x88, 12);
  touch_write_reg(0x89, 40);

  uint8_t points = 0;
  err = touch_read_reg(kTouchRegPoints, &points, 1);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "touch probe failed: %s", esp_err_to_name(err));
    return false;
  }

  ESP_LOGI(TAG, "touch controller ready");
  return true;
}

void M5DialBoard::poll_touch() {
  if (!touch_ready_) {
    touch_state_ = {};
    return;
  }

  const TouchState sample = sample_touch();
  touch_state_ = sample;

  if (sample.pressed) {
    if (!touch_was_pressed_) {
      touch_start_ = sample;
      touch_last_ = sample;
      touch_was_pressed_ = true;
    } else {
      touch_last_ = sample;
    }
    return;
  }

  if (!touch_was_pressed_) {
    return;
  }

  touch_was_pressed_ = false;
  const int dx = static_cast<int>(touch_last_.x) - static_cast<int>(touch_start_.x);
  const int dy = static_cast<int>(touch_last_.y) - static_cast<int>(touch_start_.y);
  const int abs_dx = dx < 0 ? -dx : dx;
  const int abs_dy = dy < 0 ? -dy : dy;

  SwipeDirection swipe = SwipeDirection::kNone;
  if (abs_dx >= kSwipeThresholdPx && abs_dx >= abs_dy + kSwipeAxisLeadPx) {
    swipe = dx > 0 ? SwipeDirection::kRight : SwipeDirection::kLeft;
  } else if (abs_dy >= kSwipeThresholdPx && abs_dy >= abs_dx + kSwipeAxisLeadPx) {
    swipe = dy > 0 ? SwipeDirection::kDown : SwipeDirection::kUp;
  }

  if (swipe != SwipeDirection::kNone) {
    swipe_pending_ = swipe;
    ESP_LOGI(TAG, "touch swipe %d", static_cast<int>(swipe));
  }
}

TouchState M5DialBoard::sample_touch() {
  TouchState out;

  uint8_t points = 0;
  if (touch_read_reg(kTouchRegPoints, &points, 1) != ESP_OK) {
    return out;
  }

  points &= 0x0F;
  if (points == 0) {
    return out;
  }

  uint8_t xy[4] = {};
  if (touch_read_reg(kTouchRegPoint1, xy, sizeof(xy)) != ESP_OK) {
    return out;
  }

  out.pressed = true;
  out.x = static_cast<int16_t>(((xy[0] & 0x0F) << 8) | xy[1]);
  out.y = static_cast<int16_t>(((xy[2] & 0x0F) << 8) | xy[3]);
  return out;
}

}  // namespace tutorial_0072
