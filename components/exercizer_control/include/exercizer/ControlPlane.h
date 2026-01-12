#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "exercizer/ControlPlaneTypes.h"

class ControlPlane {
 public:
  bool Start(size_t queue_len = 16);
  bool Send(const exercizer_ctrl_event_t &ev) const;
  bool Receive(exercizer_ctrl_event_t *out, TickType_t timeout) const;

 private:
  QueueHandle_t queue_ = nullptr;
};
