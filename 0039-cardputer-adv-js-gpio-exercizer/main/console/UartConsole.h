#pragma once

#include "driver/uart.h"

#include "console/IConsole.h"

class UartConsole final : public IConsole {
 public:
  UartConsole(uart_port_t uart_num, int baud_rate);
  ~UartConsole() override = default;

  int Read(uint8_t* buffer, int max_len, TickType_t timeout) override;
  void Write(const uint8_t* buffer, int len) override;

 private:
  uart_port_t uart_num_;
};

