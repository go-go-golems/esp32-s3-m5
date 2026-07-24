// Bounded authenticated WebSocket text client (ESP-54).
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "app_events.h"

namespace pulp {

enum class SocketState : uint8_t {
    Idle = 0,
    Connecting,
    Open,
    Error,
};

StatusCode SocketBegin(const char *url);
StatusCode SocketBearer();
StatusCode SocketStart();
void SocketStop();
void SocketTick();

int32_t SocketStatus();
const char *SocketStateName();
uint32_t SocketMessageCount();
bool SocketCopyMessage(uint32_t index, char *out, size_t cap, uint64_t *seq);
uint32_t SocketDropped();
uint32_t SocketReceived();
const char *SocketLastError();

struct SocketSnapshot;
void FillSocketSnapshot(SocketSnapshot *out);

}  // namespace pulp
