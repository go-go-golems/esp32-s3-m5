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

#include "s3paper/display_backend.h"
#include "s3paper/refresh_planner.h"
#include "s3paper/status.h"

namespace reader {

// Stable error vocabulary lives in s3paper_core and is shared with future
// C/JS bindings.
using StatusCode = s3paper::StatusCode;
using s3paper::StatusCodeName;

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
    Fixture,     // arg: 0 = fake backend, 1 = M5 backend
    Refresh,     // planner policy/history inspection
    SoakStart,   // arg: number of steps
    SoakStatus,
    Touch,       // arg: 0 = status, 1 = enable, 2 = disable
};

enum class PointerPhase : uint8_t {
    Down = 0,
    Move,
    Up,
    Cancel,
};

struct ConsolePayload {
    ConsoleOp op;
    uint32_t arg;
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

struct PlannedPresentSnapshot {
    s3paper::PresentResult present;
    uint8_t full_refresh;
    uint8_t waveform;  // s3paper::EpdWaveform
    uint8_t reason;    // s3paper::RefreshReason
};

struct RefreshSnapshot {
    // Policy.
    uint32_t max_turns_between_full;
    uint64_t max_partial_area_between_full;
    int64_t max_elapsed_us_between_full;
    int32_t merge_distance;
    int32_t align_x;
    // History.
    uint8_t first_render_done;
    uint32_t turns_since_full;
    uint64_t partial_area_since_full;
    uint32_t fulls_total;
    uint32_t partials_total;
    uint32_t merge_fallbacks;
    uint32_t pending_damage;
};

struct SoakWaveformStats {
    uint32_t count;
    uint64_t total_us;  // render + wait
    uint32_t max_us;
};

struct SoakSnapshot {
    uint8_t active;
    uint32_t target;
    uint32_t completed;
    uint32_t errors;
    uint32_t fulls;
    uint32_t partials;
    uint32_t heap_free_start;
    uint32_t heap_free_now;
    uint32_t heap_free_min;
    uint32_t integrity_checks;
    uint32_t integrity_failures;
    int64_t elapsed_us;
    SoakWaveformStats by_waveform[4];  // s3paper::EpdWaveform order
};

struct TouchSnapshot {
    uint8_t enabled;
    uint32_t samples;
    uint32_t events_by_kind[4];    // s3paper::PointerEventKind order
    uint32_t gestures_by_kind[6];  // s3paper::GestureKind order
    int32_t last_x;
    int32_t last_y;
    uint8_t last_gesture;  // 0xff = none
    int32_t last_gesture_x;
    int32_t last_gesture_y;
    uint32_t quiet_windows;
    int64_t last_input_age_ms;  // -1 = never
    uint32_t scheduler_pending;
};

struct DisplaySnapshot {
    uint8_t app_state;
    uint8_t fake_initialized;
    uint8_t m5_initialized;
    int32_t m5_w;
    int32_t m5_h;
    uint32_t fake_frames;
    uint32_t m5_frames;
};

struct AppReply {
    uint32_t request_id;
    StatusCode status;
    union {
        StatusSnapshot status_snapshot;
        HeapSnapshot heap;
        EventsSnapshot events;
        StressSnapshot stress;
        DisplaySnapshot display;
        PlannedPresentSnapshot present;  // Fixture
        RefreshSnapshot refresh;
        SoakSnapshot soak;
        TouchSnapshot touch;
        int64_t echo_monotonic_us;  // Ping: the event's enqueue timestamp
    } payload;
};

}  // namespace reader
