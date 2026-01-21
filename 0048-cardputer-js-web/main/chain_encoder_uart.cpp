#include "chain_encoder_uart.h"

#include <algorithm>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "chain_enc_0048";

namespace {

constexpr uint8_t kHeadHi = 0xAA;
constexpr uint8_t kHeadLo = 0x55;
constexpr uint8_t kTailHi = 0x55;
constexpr uint8_t kTailLo = 0xAA;

// Protocol commands we care about.
constexpr uint8_t kCmdGetInc = 0x11;
constexpr uint8_t kCmdButtonEvent = 0xE0;
constexpr uint8_t kCmdSetButtonMode = 0xE4;
constexpr uint8_t kCmdGetFwVersion = 0xFA;
constexpr uint8_t kCmdGetDeviceType = 0xFB;

static uint8_t crc8_sum(const uint8_t* p, size_t n) {
  uint8_t s = 0;
  for (size_t i = 0; i < n; i++) s = (uint8_t)(s + p[i]);
  return s;
}

} // namespace

ChainEncoderUart::ChainEncoderUart(const Config& cfg) : cfg_(cfg) {}

bool ChainEncoderUart::init() {
  uart_config_t uart_config = {};
  uart_config.baud_rate = cfg_.baud;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 0;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  const esp_err_t err_cfg = uart_param_config((uart_port_t)cfg_.uart_num, &uart_config);
  if (err_cfg != ESP_OK) {
    ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err_cfg));
    return false;
  }

  const esp_err_t err_pin =
      uart_set_pin((uart_port_t)cfg_.uart_num, cfg_.tx_gpio, cfg_.rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (err_pin != ESP_OK) {
    ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err_pin));
    return false;
  }

  const int rx_buf = 2048;
  const int tx_buf = 2048;
  const esp_err_t err_inst = uart_driver_install((uart_port_t)cfg_.uart_num, rx_buf, tx_buf, 0, nullptr, 0);
  if (err_inst != ESP_OK && err_inst != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err_inst));
    return false;
  }

  if (!task_) {
    const BaseType_t ok = xTaskCreate(&ChainEncoderUart::task_trampoline, "chain_enc", 4096, this, 10, &task_);
    if (ok != pdPASS) {
      ESP_LOGE(TAG, "xTaskCreate failed");
      task_ = nullptr;
      return false;
    }
  }

  return true;
}

int32_t ChainEncoderUart::take_delta() {
  return delta_accum_.exchange(0);
}

bool ChainEncoderUart::has_click_pending() const {
  return click_pending_.load() != 0;
}

void ChainEncoderUart::clear_click_pending() {
  click_pending_.store(0);
}

void ChainEncoderUart::task_trampoline(void* arg) {
  static_cast<ChainEncoderUart*>(arg)->task_main();
}

void ChainEncoderUart::task_main() {
  ESP_LOGI(TAG,
           "start: uart=%d baud=%d tx=%d rx=%d index=%u poll_ms=%d",
           cfg_.uart_num,
           cfg_.baud,
           cfg_.tx_gpio,
           cfg_.rx_gpio,
           (unsigned)cfg_.index_id,
           cfg_.poll_ms);

  // Best-effort: enable active reporting so we get 0xE0 click events reliably.
  {
    const uint8_t mode = 1;
    std::vector<uint8_t> frame;
    build_frame(kCmdSetButtonMode, &mode, 1, &frame);
    (void)uart_write_bytes((uart_port_t)cfg_.uart_num, frame.data(), (uint32_t)frame.size());

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(100);
    Frame f;
    while (xTaskGetTickCount() < deadline) {
      if (!read_frame(&f, deadline)) continue;
      if (f.cmd == kCmdButtonEvent) {
        handle_unsolicited(f);
        continue;
      }
      if (f.cmd == kCmdSetButtonMode) {
        if (!f.data.empty() && f.data[0] == 1) {
          ESP_LOGI(TAG, "button mode set: active reporting");
        } else {
          ESP_LOGW(TAG, "button mode set response: %u", f.data.empty() ? 0U : (unsigned)f.data[0]);
        }
        break;
      }
    }
  }

  // Best-effort identification logs.
  {
    std::vector<uint8_t> frame;
    build_frame(kCmdGetDeviceType, nullptr, 0, &frame);
    (void)uart_write_bytes((uart_port_t)cfg_.uart_num, frame.data(), (uint32_t)frame.size());
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(100);
    Frame f;
    while (xTaskGetTickCount() < deadline) {
      if (!read_frame(&f, deadline)) continue;
      if (f.cmd == kCmdButtonEvent) {
        handle_unsolicited(f);
        continue;
      }
      if (f.cmd == kCmdGetDeviceType && f.data.size() >= 2) {
        const uint16_t t = (uint16_t)f.data[0] | ((uint16_t)f.data[1] << 8);
        ESP_LOGI(TAG, "device_type=0x%04x (expected encoder=0x0001)", (unsigned)t);
        break;
      }
    }
  }

  {
    std::vector<uint8_t> frame;
    build_frame(kCmdGetFwVersion, nullptr, 0, &frame);
    (void)uart_write_bytes((uart_port_t)cfg_.uart_num, frame.data(), (uint32_t)frame.size());
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(100);
    Frame f;
    while (xTaskGetTickCount() < deadline) {
      if (!read_frame(&f, deadline)) continue;
      if (f.cmd == kCmdButtonEvent) {
        handle_unsolicited(f);
        continue;
      }
      if (f.cmd == kCmdGetFwVersion && f.data.size() >= 1) {
        ESP_LOGI(TAG, "fw_version=0x%02x", (unsigned)f.data[0]);
        break;
      }
    }
  }

  int err_count = 0;

  while (true) {
    int16_t delta = 0;
    if (cmd_get_inc(&delta)) {
      if (delta != 0) {
        delta_accum_.fetch_add((int32_t)delta);
      }
      err_count = 0;
    } else {
      err_count++;
      if (err_count == 1 || (err_count % 50) == 0) {
        ESP_LOGW(TAG, "no response to get_inc (count=%d)", err_count);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(cfg_.poll_ms));
  }
}

void ChainEncoderUart::build_frame(uint8_t cmd, const uint8_t* data, size_t data_len, std::vector<uint8_t>* out) const {
  out->clear();
  out->reserve(2 + 2 + (3 + data_len) + 2);

  const uint16_t payload_len = (uint16_t)(3 + data_len); // Index + Cmd + Data... + CRC

  out->push_back(kHeadHi);
  out->push_back(kHeadLo);
  out->push_back((uint8_t)(payload_len & 0xFF));
  out->push_back((uint8_t)((payload_len >> 8) & 0xFF));

  out->push_back(cfg_.index_id);
  out->push_back(cmd);
  for (size_t i = 0; i < data_len; i++) out->push_back(data[i]);

  const uint8_t crc = crc8_sum(out->data() + 4, (size_t)(payload_len - 1)); // exclude CRC itself
  out->push_back(crc);

  out->push_back(kTailHi);
  out->push_back(kTailLo);
}

bool ChainEncoderUart::cmd_get_inc(int16_t* out_delta) {
  if (!out_delta) return false;

  std::vector<uint8_t> frame;
  build_frame(kCmdGetInc, nullptr, 0, &frame);

  const int written = uart_write_bytes((uart_port_t)cfg_.uart_num, frame.data(), (uint32_t)frame.size());
  if (written != (int)frame.size()) {
    ESP_LOGW(TAG, "uart_write_bytes short: %d/%u", written, (unsigned)frame.size());
  }

  // Wait for a matching response; ignore unsolicited packets (e.g. 0xE0) in the stream.
  const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(50);
  Frame f;
  while (xTaskGetTickCount() < deadline) {
    if (!read_frame(&f, deadline)) {
      continue;
    }
    if (f.cmd == kCmdButtonEvent) {
      handle_unsolicited(f);
      continue;
    }
    if (f.cmd != kCmdGetInc) {
      continue;
    }
    if (f.data.size() < 2) {
      return false;
    }
    const uint8_t lo = f.data[0];
    const uint8_t hi = f.data[1];
    *out_delta = (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
    return true;
  }

  return false;
}

bool ChainEncoderUart::read_frame(Frame* out, TickType_t deadline_ticks) {
  if (!out) return false;

  // Pull a few bytes at a time and feed the parser.
  uint8_t tmp[128];
  const TickType_t now = xTaskGetTickCount();
  const TickType_t remain = (deadline_ticks > now) ? (deadline_ticks - now) : 0;
  const int got = uart_read_bytes((uart_port_t)cfg_.uart_num, tmp, sizeof(tmp), remain);
  if (got <= 0) return false;

  feed_bytes(tmp, (size_t)got);
  return try_extract_one(out);
}

void ChainEncoderUart::feed_bytes(const uint8_t* data, size_t n) {
  if (!data || n == 0) return;
  buf_.insert(buf_.end(), data, data + n);

  // Prevent pathological growth if framing is lost.
  if (buf_.size() > 4096) {
    buf_.erase(buf_.begin(), buf_.begin() + (buf_.size() - 2048));
  }
}

bool ChainEncoderUart::try_extract_one(Frame* out) {
  if (!out) return false;

  // Find header.
  while (buf_.size() >= 2) {
    if (buf_[0] == kHeadHi && buf_[1] == kHeadLo) break;
    buf_.erase(buf_.begin());
  }
  if (buf_.size() < 4) return false;

  const uint16_t payload_len = (uint16_t)buf_[2] | ((uint16_t)buf_[3] << 8);
  const size_t frame_len = 2 + 2 + payload_len + 2;
  if (payload_len < 3) {
    buf_.erase(buf_.begin());
    return false;
  }
  if (frame_len > 8192) {
    buf_.clear();
    return false;
  }
  if (buf_.size() < frame_len) return false;

  // Validate tail.
  if (buf_[frame_len - 2] != kTailHi || buf_[frame_len - 1] != kTailLo) {
    buf_.erase(buf_.begin());
    return false;
  }

  const uint8_t crc = buf_[2 + 2 + payload_len - 1];
  const uint8_t crc_calc = crc8_sum(buf_.data() + 4, (size_t)(payload_len - 1));
  if (crc != crc_calc) {
    buf_.erase(buf_.begin());
    return false;
  }

  out->index = buf_[4];
  out->cmd = buf_[5];
  out->data.clear();
  if (payload_len > 3) {
    out->data.insert(out->data.end(), buf_.begin() + 6, buf_.begin() + (2 + 2 + payload_len - 1));
  }

  buf_.erase(buf_.begin(), buf_.begin() + frame_len);
  return true;
}

void ChainEncoderUart::handle_unsolicited(const Frame& f) {
  if (f.cmd != kCmdButtonEvent) return;
  if (f.data.size() < 1) return;
  // Protocol: 0=single, 1=double, 2=long
  click_pending_.store(f.data[0]);
}
