#include "console/UartConsole.h"

UartConsole::UartConsole(uart_port_t uart_num, int baud_rate) : uart_num_(uart_num) {
  uart_config_t uart_config = {};
  uart_config.baud_rate = baud_rate;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 0;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  ESP_ERROR_CHECK(uart_param_config(uart_num_, &uart_config));
  ESP_ERROR_CHECK(uart_driver_install(uart_num_, 1024 * 2, 0, 0, nullptr, 0));
}

int UartConsole::Read(uint8_t* buffer, int max_len, TickType_t timeout) {
  return uart_read_bytes(uart_num_, buffer, max_len, timeout);
}

void UartConsole::Write(const uint8_t* buffer, int len) {
  if (!buffer || len <= 0) {
    return;
  }
  uart_write_bytes(uart_num_, reinterpret_cast<const char*>(buffer), len);
}
