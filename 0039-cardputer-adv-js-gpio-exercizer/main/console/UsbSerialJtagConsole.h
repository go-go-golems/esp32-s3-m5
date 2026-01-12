#pragma once

#include "console/IConsole.h"

class UsbSerialJtagConsole final : public IConsole {
 public:
  UsbSerialJtagConsole();
  ~UsbSerialJtagConsole() override = default;

  int Read(uint8_t* buffer, int max_len, TickType_t timeout) override;
  void Write(const uint8_t* buffer, int len) override;
};

