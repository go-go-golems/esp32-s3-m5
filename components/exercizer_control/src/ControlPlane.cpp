#include "exercizer/ControlPlane.h"

#include "exercizer/ControlPlaneC.h"

static ControlPlane *s_default_ctrl = nullptr;

bool ControlPlane::Start(size_t queue_len) {
  if (queue_) {
    return true;
  }
  if (queue_len == 0) {
    queue_len = 16;
  }
  queue_ = xQueueCreate(queue_len, sizeof(exercizer_ctrl_event_t));
  return queue_ != nullptr;
}

bool ControlPlane::Send(const exercizer_ctrl_event_t &ev) const {
  if (!queue_) {
    return false;
  }
  return xQueueSend(queue_, &ev, 0) == pdTRUE;
}

bool ControlPlane::Receive(exercizer_ctrl_event_t *out, TickType_t timeout) const {
  if (!queue_ || !out) {
    return false;
  }
  return xQueueReceive(queue_, out, timeout) == pdTRUE;
}

extern "C" void exercizer_control_set_default(void *ctrl) {
  s_default_ctrl = static_cast<ControlPlane *>(ctrl);
}

extern "C" bool exercizer_control_send(const exercizer_ctrl_event_t *ev) {
  if (!s_default_ctrl || !ev) {
    return false;
  }
  return s_default_ctrl->Send(*ev);
}
