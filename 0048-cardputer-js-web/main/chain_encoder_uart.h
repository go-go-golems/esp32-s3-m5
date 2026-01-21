#pragma once

#include <stdint.h>

#include <atomic>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Minimal host-side driver for the M5Stack Chain Encoder (U207) UART protocol.
//
// Intended usage in 0048:
// - Create and init() the driver.
// - Periodically consume take_delta() for position updates.
// - Use has_click_pending()/clear_click_pending() to detect click events.
class ChainEncoderUart {
public:
  struct Config {
    int uart_num = 1;
    int baud = 115200;
    int tx_gpio = 2; // G2
    int rx_gpio = 1; // G1
    uint8_t index_id = 1;
    int poll_ms = 20;
  };

  explicit ChainEncoderUart(const Config& cfg);

  bool init();

  // Returns accumulated delta since last call (clears accumulator).
  int32_t take_delta();

  bool has_click_pending() const;
  void clear_click_pending();

private:
  struct Frame {
    uint8_t index = 0;
    uint8_t cmd = 0;
    std::vector<uint8_t> data;
  };

  static void task_trampoline(void* arg);
  void task_main();

  void build_frame(uint8_t cmd, const uint8_t* data, size_t data_len, std::vector<uint8_t>* out) const;

  bool read_frame(Frame* out, TickType_t deadline_ticks);
  void feed_bytes(const uint8_t* data, size_t n);
  bool try_extract_one(Frame* out);

  void handle_unsolicited(const Frame& f);

  bool cmd_get_inc(int16_t* out_delta);

  Config cfg_;
  TaskHandle_t task_ = nullptr;

  std::atomic<int32_t> delta_accum_{0};
  std::atomic<uint8_t> click_pending_{0}; // 0=no, else last trigger status (0 single,1 double,2 long)

  std::vector<uint8_t> buf_;
};

