// Single UI/application owner task (PULP OS).
//
// Ownership rule: this module's task is the only place application state is
// read or written. Producers interact exclusively through PostEvent(), which
// never blocks: a full queue is an explicit, counted rejection.
#pragma once

#include "app_events.h"

namespace pulp {

// Bounded event queue capacity. Deliberately small so queue-full behavior
// is observable instead of theoretical.
constexpr uint32_t kEventQueueCapacity = 32;

// Starts the owner task and creates the event queue. Must be called once,
// before any PostEvent().
void OwnerStart();

// Non-blocking enqueue usable from any task. Returns Ok, CapacityExceeded
// (queue full; counted per source), or InvalidArgument.
StatusCode PostEvent(const AppEvent &event);

// Producer-side helper: allocates the next per-source sequence number.
uint32_t NextProducerSeq(EventSource source);

// Blocks up to timeout_ms for a reply carrying request_id on reply_queue.
StatusCode AwaitReply(QueueHandle_t reply_queue, uint32_t request_id,
                      uint32_t timeout_ms, AppReply *out);

// Aborts unless called on the owner task. Modules holding application
// state call this at their entry points.
void AssertOwner();

}  // namespace pulp
