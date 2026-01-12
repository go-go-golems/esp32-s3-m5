#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"

class IConsole {
 public:
  virtual ~IConsole() = default;

  virtual int Read(uint8_t* buffer, int max_len, TickType_t timeout) = 0;
  virtual void Write(const uint8_t* buffer, int len) = 0;

  void WriteString(const char* s);
};

