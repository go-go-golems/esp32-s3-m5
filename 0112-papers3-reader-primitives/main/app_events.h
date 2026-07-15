// Phase 1 event/command/reply contracts for the PaperS3 reader-primitives
// firmware (ESP-50-PAPERS3-EREADER-PRIMITIVES).
//
// Contract rules:
//  - Every type here is POD. No owned std::string, no borrowed pointers into
//    producer stacks, no JS values. Payload text lives in fixed char arrays.
//  - Producers (console task, stress producers, later touch/timers/storage)
//    only enqueue AppEvent. All model mutation happens in the owner task.
//  - Replies travel through a bounded FreeRTOS queue whose handle is carried
//    in the event. A full reply queue is an explicit counted failure, never a
//    silent block.
#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace reader {

// Stable error vocabulary shared with future C/JS bindings.
enum class StatusCode : uint8_t {
    Ok = 0,
    InvalidArgument,
    CapacityExceeded,
    Busy,
    Timeout,
    CorruptData,
    OutOfMemory,
    Unimplemented,
};

const char *StatusCodeName(StatusCode code);

enum class AppEventKind : uint8_t {
    ConsoleCommand = 0,
    Pointer,
    TimerDue,
    StorageComplete,
    ShutdownRequest,
    kCount,
};

const char *AppEventKindName(AppEventKind kind);

// Producer identity, used for per-source ordering validation and drop
// accounting. Every producer task uses exactly one source id.
enum class EventSource : uint8_t {
    Console = 0,
    StressConsole,
    StressInput,
    Internal,
    kCount,
};

const char *EventSourceName(EventSource source);

enum class ConsoleOp : uint8_t {
    Status = 0,
    Heap,
    Display,
    Events,
    Ping,
    StressReport,
    StressReset,
};

enum class PointerPhase : uint8_t {
    Down = 0,
    Move,
    Up,
    Cancel,
};

struct ConsolePayload {
    ConsoleOp op;
};

struct PointerPayload {
    int32_t x;
    int32_t y;
    PointerPhase phase;
    uint8_t pointer_id;
};

struct TimerPayload {
    uint32_t timer_id;
};

struct StoragePayload {
    uint32_t op_id;
    StatusCode result;
};

struct AppEvent {
    AppEventKind kind;
    EventSource source;
    // Monotonic per-source sequence assigned by the producer; the owner
    // verifies it arrives strictly ordered per source.
    uint32_t producer_seq;
    // Correlation id for request/reply commands; 0 when no reply is expected.
    uint32_t request_id;
    int64_t monotonic_us;
    // Bounded reply channel; nullptr when the event has no reply. The handle
    // refers to a queue owned by the requesting task and outliving the wait.
    QueueHandle_t reply_queue;
    union {
        ConsolePayload console;
        PointerPayload pointer;
        TimerPayload timer;
        StoragePayload storage;
    } payload;
};

// Snapshot payloads returned by the owner. Fixed-size, copied by value.
struct StatusSnapshot {
    uint32_t uptime_ms;
    uint32_t owner_seq;             // total events processed
    uint32_t processed_by_kind[static_cast<uint8_t>(AppEventKind::kCount)];
    uint8_t app_state;              // AppState::Phase as uint8_t
};

struct HeapSnapshot {
    uint32_t internal_free;
    uint32_t internal_min_free;
    uint32_t internal_largest_block;
    uint32_t psram_free;
    uint32_t psram_largest_block;
};

struct EventsSnapshot {
    uint32_t queue_capacity;
    uint32_t queue_depth_now;
    uint32_t queue_high_water;
    uint32_t accepted_by_source[static_cast<uint8_t>(EventSource::kCount)];
    uint32_t dropped_by_source[static_cast<uint8_t>(EventSource::kCount)];
    uint32_t out_of_order_total;
    uint32_t replies_sent;
    uint32_t replies_dropped;
};

struct StressSnapshot {
    uint32_t console_received;
    uint32_t input_received;
    uint32_t out_of_order;
    uint32_t last_console_seq;
    uint32_t last_input_seq;
};

struct AppReply {
    uint32_t request_id;
    StatusCode status;
    union {
        StatusSnapshot status_snapshot;
        HeapSnapshot heap;
        EventsSnapshot events;
        StressSnapshot stress;
        int64_t echo_monotonic_us;  // Ping: the event's enqueue timestamp
    } payload;
};

}  // namespace reader
