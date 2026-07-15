#include "app_owner.h"

#include <atomic>
#include <cstring>
#include <initializer_list>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

namespace reader {
namespace {

const char *kTag = "owner";

constexpr uint8_t kSourceCount = static_cast<uint8_t>(EventSource::kCount);
constexpr uint8_t kKindCount = static_cast<uint8_t>(AppEventKind::kCount);

enum class Phase : uint8_t {
    Booting = 0,
    Ready,
    ShuttingDown,
};

// Application state. Touched exclusively by the owner task; AssertOwner()
// enforces that at runtime in debug and release builds alike.
struct AppState {
    Phase phase = Phase::Booting;
    int64_t boot_us = 0;
    uint32_t owner_seq = 0;
    uint32_t processed_by_kind[kKindCount] = {};
    // Per-source ordering validation.
    uint32_t last_seq_by_source[kSourceCount] = {};
    uint32_t out_of_order_by_source[kSourceCount] = {};
    uint32_t accepted_by_source[kSourceCount] = {};
    uint32_t queue_high_water = 0;
    uint32_t replies_sent = 0;
    uint32_t replies_dropped = 0;
    // Stress fixture accounting (Pointer events from StressInput and
    // ConsoleCommand events from StressConsole).
    uint32_t stress_console_received = 0;
    uint32_t stress_input_received = 0;
};

AppState s_state;
QueueHandle_t s_event_queue = nullptr;
TaskHandle_t s_owner_task = nullptr;

// Producer-side instrumentation. These are not application state: they count
// enqueue attempts that never reached the owner, so they are the only values
// legitimately written outside the owner task.
std::atomic<uint32_t> s_dropped_by_source[kSourceCount];
std::atomic<uint32_t> s_next_seq_by_source[kSourceCount];

void AssertOwner() {
    if (xTaskGetCurrentTaskHandle() != s_owner_task) {
        ESP_LOGE(kTag, "app state touched outside owner task");
        abort();
    }
}

void SendReply(const AppEvent &event, const AppReply &reply) {
    AssertOwner();
    if (event.reply_queue == nullptr) {
        return;
    }
    if (xQueueSend(event.reply_queue, &reply, 0) == pdTRUE) {
        s_state.replies_sent++;
    } else {
        s_state.replies_dropped++;
    }
}

AppReply MakeReply(const AppEvent &event, StatusCode status) {
    AppReply reply;
    std::memset(&reply, 0, sizeof(reply));
    reply.request_id = event.request_id;
    reply.status = status;
    return reply;
}

void FillStatusSnapshot(StatusSnapshot *out) {
    AssertOwner();
    out->uptime_ms =
        static_cast<uint32_t>((esp_timer_get_time() - s_state.boot_us) / 1000);
    out->owner_seq = s_state.owner_seq;
    for (uint8_t i = 0; i < kKindCount; ++i) {
        out->processed_by_kind[i] = s_state.processed_by_kind[i];
    }
    out->app_state = static_cast<uint8_t>(s_state.phase);
}

void HandleConsoleCommand(const AppEvent &event) {
    AssertOwner();
    AppReply reply = MakeReply(event, StatusCode::Ok);
    switch (event.payload.console.op) {
        case ConsoleOp::Status:
        case ConsoleOp::Display:
            // Phase 1 has no display backend; Display returns the same app
            // snapshot and the console reports backend=none.
            FillStatusSnapshot(&reply.payload.status_snapshot);
            break;
        case ConsoleOp::Heap: {
            HeapSnapshot &h = reply.payload.heap;
            h.internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            h.internal_min_free =
                heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
            h.internal_largest_block =
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
            h.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            h.psram_largest_block =
                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
            break;
        }
        case ConsoleOp::Events: {
            EventsSnapshot &e = reply.payload.events;
            e.queue_capacity = kEventQueueCapacity;
            e.queue_depth_now =
                static_cast<uint32_t>(uxQueueMessagesWaiting(s_event_queue));
            e.queue_high_water = s_state.queue_high_water;
            uint32_t out_of_order_total = 0;
            for (uint8_t i = 0; i < kSourceCount; ++i) {
                e.accepted_by_source[i] = s_state.accepted_by_source[i];
                e.dropped_by_source[i] =
                    s_dropped_by_source[i].load(std::memory_order_relaxed);
                out_of_order_total += s_state.out_of_order_by_source[i];
            }
            e.out_of_order_total = out_of_order_total;
            e.replies_sent = s_state.replies_sent;
            e.replies_dropped = s_state.replies_dropped;
            break;
        }
        case ConsoleOp::Ping:
            if (s_state.phase != Phase::Ready) {
                reply.status = StatusCode::Busy;
            }
            reply.payload.echo_monotonic_us = event.monotonic_us;
            break;
        case ConsoleOp::StressReport: {
            StressSnapshot &s = reply.payload.stress;
            s.console_received = s_state.stress_console_received;
            s.input_received = s_state.stress_input_received;
            s.out_of_order =
                s_state.out_of_order_by_source[static_cast<uint8_t>(
                    EventSource::StressConsole)] +
                s_state.out_of_order_by_source[static_cast<uint8_t>(
                    EventSource::StressInput)];
            s.last_console_seq = s_state.last_seq_by_source[static_cast<
                uint8_t>(EventSource::StressConsole)];
            s.last_input_seq = s_state.last_seq_by_source[static_cast<uint8_t>(
                EventSource::StressInput)];
            break;
        }
        case ConsoleOp::StressReset:
            s_state.stress_console_received = 0;
            s_state.stress_input_received = 0;
            for (EventSource src :
                 {EventSource::StressConsole, EventSource::StressInput}) {
                const uint8_t i = static_cast<uint8_t>(src);
                s_state.last_seq_by_source[i] = 0;
                s_state.out_of_order_by_source[i] = 0;
                s_state.accepted_by_source[i] = 0;
                s_dropped_by_source[i].store(0, std::memory_order_relaxed);
                s_next_seq_by_source[i].store(0, std::memory_order_relaxed);
            }
            break;
    }
    SendReply(event, reply);
}

void HandleEvent(const AppEvent &event) {
    AssertOwner();
    const uint8_t source = static_cast<uint8_t>(event.source);
    const uint8_t kind = static_cast<uint8_t>(event.kind);
    if (source >= kSourceCount || kind >= kKindCount) {
        ESP_LOGW(kTag, "discarding malformed event kind=%u source=%u", kind,
                 source);
        return;
    }
    s_state.owner_seq++;
    s_state.processed_by_kind[kind]++;
    s_state.accepted_by_source[source]++;
    // Per-source ordering check: sequences must arrive strictly increasing.
    if (event.producer_seq != 0) {
        if (event.producer_seq <= s_state.last_seq_by_source[source]) {
            s_state.out_of_order_by_source[source]++;
            ESP_LOGW(kTag, "out-of-order event source=%s seq=%u last=%u",
                     EventSourceName(event.source),
                     static_cast<unsigned>(event.producer_seq),
                     static_cast<unsigned>(
                         s_state.last_seq_by_source[source]));
        }
        s_state.last_seq_by_source[source] = event.producer_seq;
    }

    switch (event.kind) {
        case AppEventKind::ConsoleCommand:
            if (event.source == EventSource::StressConsole) {
                s_state.stress_console_received++;
                SendReply(event, MakeReply(event, StatusCode::Ok));
            } else {
                HandleConsoleCommand(event);
            }
            break;
        case AppEventKind::Pointer:
            // Phase 1 model: pointer events only advance counters; real hit
            // testing arrives with Phase 4.
            if (event.source == EventSource::StressInput) {
                s_state.stress_input_received++;
            }
            break;
        case AppEventKind::TimerDue:
        case AppEventKind::StorageComplete:
            // Counted via processed_by_kind; no Phase 1 model behind them.
            break;
        case AppEventKind::ShutdownRequest:
            s_state.phase = Phase::ShuttingDown;
            ESP_LOGI(kTag, "shutdown requested; commands now report Busy");
            SendReply(event, MakeReply(event, StatusCode::Ok));
            break;
        default:
            break;
    }
}

void OwnerTask(void *) {
    s_state.boot_us = esp_timer_get_time();
    s_state.phase = Phase::Ready;
    ESP_LOGI(kTag, "owner task ready; queue capacity=%u",
             static_cast<unsigned>(kEventQueueCapacity));
    for (;;) {
        AppEvent event;
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        const uint32_t depth =
            static_cast<uint32_t>(uxQueueMessagesWaiting(s_event_queue)) + 1;
        if (depth > s_state.queue_high_water) {
            s_state.queue_high_water = depth;
        }
        HandleEvent(event);
    }
}

}  // namespace

const char *StatusCodeName(StatusCode code) {
    switch (code) {
        case StatusCode::Ok: return "Ok";
        case StatusCode::InvalidArgument: return "InvalidArgument";
        case StatusCode::CapacityExceeded: return "CapacityExceeded";
        case StatusCode::Busy: return "Busy";
        case StatusCode::Timeout: return "Timeout";
        case StatusCode::CorruptData: return "CorruptData";
        case StatusCode::OutOfMemory: return "OutOfMemory";
        case StatusCode::Unimplemented: return "Unimplemented";
    }
    return "Unknown";
}

const char *AppEventKindName(AppEventKind kind) {
    switch (kind) {
        case AppEventKind::ConsoleCommand: return "ConsoleCommand";
        case AppEventKind::Pointer: return "Pointer";
        case AppEventKind::TimerDue: return "TimerDue";
        case AppEventKind::StorageComplete: return "StorageComplete";
        case AppEventKind::ShutdownRequest: return "ShutdownRequest";
        case AppEventKind::kCount: break;
    }
    return "Unknown";
}

const char *EventSourceName(EventSource source) {
    switch (source) {
        case EventSource::Console: return "console";
        case EventSource::StressConsole: return "stress-console";
        case EventSource::StressInput: return "stress-input";
        case EventSource::Internal: return "internal";
        case EventSource::kCount: break;
    }
    return "unknown";
}

void OwnerStart() {
    if (s_owner_task != nullptr) {
        return;
    }
    s_event_queue = xQueueCreate(kEventQueueCapacity, sizeof(AppEvent));
    configASSERT(s_event_queue != nullptr);
    const BaseType_t ok = xTaskCreatePinnedToCore(
        OwnerTask, "ui_owner", 8192, nullptr, 5, &s_owner_task, 1);
    configASSERT(ok == pdPASS);
}

StatusCode PostEvent(const AppEvent &event) {
    if (s_event_queue == nullptr ||
        static_cast<uint8_t>(event.source) >= kSourceCount ||
        static_cast<uint8_t>(event.kind) >= kKindCount) {
        return StatusCode::InvalidArgument;
    }
    if (xQueueSend(s_event_queue, &event, 0) == pdTRUE) {
        return StatusCode::Ok;
    }
    s_dropped_by_source[static_cast<uint8_t>(event.source)].fetch_add(
        1, std::memory_order_relaxed);
    return StatusCode::CapacityExceeded;
}

uint32_t NextProducerSeq(EventSource source) {
    return s_next_seq_by_source[static_cast<uint8_t>(source)].fetch_add(
               1, std::memory_order_relaxed) +
           1;
}

StatusCode AwaitReply(QueueHandle_t reply_queue, uint32_t request_id,
                      uint32_t timeout_ms, AppReply *out) {
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    for (;;) {
        const TickType_t now = xTaskGetTickCount();
        const TickType_t remaining =
            (deadline > now) ? (deadline - now) : 0;
        AppReply reply;
        if (xQueueReceive(reply_queue, &reply, remaining) != pdTRUE) {
            return StatusCode::Timeout;
        }
        if (reply.request_id == request_id) {
            *out = reply;
            return StatusCode::Ok;
        }
        // Stale reply from an earlier timed-out request; drop and keep
        // waiting for the matching one.
    }
}

}  // namespace reader
