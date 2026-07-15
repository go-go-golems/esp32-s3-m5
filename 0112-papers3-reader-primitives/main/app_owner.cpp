#include "app_owner.h"

#include "app_display.h"
#include "app_input.h"
#include "app_reader.h"
#include "app_storage.h"
#include "app_ui.h"

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

// Self-posted TimerDue id driving soak steps through the ordinary queue so
// console commands interleave with a running soak.
constexpr uint32_t kSoakTimerId = 0x50AC;

struct SoakState {
    bool active = false;
    bool step_queued = false;
    uint32_t target = 0;
    uint32_t completed = 0;
    uint32_t errors = 0;
    uint32_t fulls = 0;
    uint32_t partials = 0;
    uint32_t heap_free_start = 0;
    uint32_t heap_free_min = 0;
    uint32_t integrity_checks = 0;
    uint32_t integrity_failures = 0;
    int64_t start_us = 0;
    int64_t end_us = 0;
    SoakWaveformStats by_waveform[4] = {};
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
    SoakState soak;
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

// Runs one soak step in the owner context and folds the result into stats.
void RunOneSoakStep() {
    SoakState &soak = s_state.soak;
    const PlannedPresent planned = RunSoakStep(soak.completed);
    if (planned.present.status != StatusCode::Ok) {
        soak.errors++;
        ESP_LOGW(kTag, "soak step %u failed: %s",
                 static_cast<unsigned>(soak.completed),
                 StatusCodeName(planned.present.status));
    } else {
        if (planned.plan.full_refresh) {
            soak.fulls++;
        } else {
            soak.partials++;
        }
        const uint8_t wf = static_cast<uint8_t>(planned.plan.waveform);
        SoakWaveformStats &stats = soak.by_waveform[wf & 3];
        const uint32_t step_us =
            planned.present.render_us + planned.present.wait_us;
        stats.count++;
        stats.total_us += step_us;
        if (step_us > stats.max_us) {
            stats.max_us = step_us;
        }
    }
    soak.completed++;
    const uint32_t heap_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    if (heap_free < soak.heap_free_min) {
        soak.heap_free_min = heap_free;
    }
    if (soak.completed % 256 == 0 || soak.completed == soak.target) {
        soak.integrity_checks++;
        if (!heap_caps_check_integrity_all(true)) {
            soak.integrity_failures++;
            ESP_LOGE(kTag, "soak heap integrity FAILURE at step %u",
                     static_cast<unsigned>(soak.completed));
        }
        ESP_LOGI(kTag,
                 "soak progress %u/%u fulls=%u partials=%u errors=%u "
                 "heap_free=%u",
                 static_cast<unsigned>(soak.completed),
                 static_cast<unsigned>(soak.target),
                 static_cast<unsigned>(soak.fulls),
                 static_cast<unsigned>(soak.partials),
                 static_cast<unsigned>(soak.errors),
                 static_cast<unsigned>(heap_free));
    }
    if (soak.completed >= soak.target) {
        soak.active = false;
        soak.end_us = esp_timer_get_time();
        ESP_LOGI(kTag,
                 "soak done steps=%u fulls=%u partials=%u errors=%u "
                 "integrity_failures=%u elapsed_ms=%lld",
                 static_cast<unsigned>(soak.completed),
                 static_cast<unsigned>(soak.fulls),
                 static_cast<unsigned>(soak.partials),
                 static_cast<unsigned>(soak.errors),
                 static_cast<unsigned>(soak.integrity_failures),
                 static_cast<long long>((soak.end_us - soak.start_us) /
                                        1000));
    }
}

// Keeps exactly one soak-step event circulating through the queue while a
// soak is active, so console commands interleave with soak progress.
void MaybeQueueSoakStep() {
    AssertOwner();
    SoakState &soak = s_state.soak;
    if (!soak.active || soak.step_queued) {
        return;
    }
    AppEvent event;
    std::memset(&event, 0, sizeof(event));
    event.kind = AppEventKind::TimerDue;
    event.source = EventSource::Internal;
    event.producer_seq = NextProducerSeq(EventSource::Internal);
    event.monotonic_us = esp_timer_get_time();
    event.payload.timer.timer_id = kSoakTimerId;
    if (PostEvent(event) == StatusCode::Ok) {
        soak.step_queued = true;
    }
    // On CapacityExceeded the next owner-loop iteration retries; the drop is
    // visible in the internal source's rejected counter.
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
            FillStatusSnapshot(&reply.payload.status_snapshot);
            break;
        case ConsoleOp::Display: {
            DisplaySnapshot &d = reply.payload.display;
            const s3paper::BackendState fake = FakeBackendState();
            const s3paper::BackendState m5 = M5BackendState();
            d.app_state = static_cast<uint8_t>(s_state.phase);
            d.fake_initialized = fake.initialized ? 1 : 0;
            d.m5_initialized = m5.initialized ? 1 : 0;
            d.m5_w = m5.physical_size.w;
            d.m5_h = m5.physical_size.h;
            d.fake_frames = fake.frames_presented;
            d.m5_frames = m5.frames_presented;
            break;
        }
        case ConsoleOp::Fixture: {
            const uint32_t arg = event.payload.console.arg;
            const bool use_m5 = arg != 0;
            const PlannedPresent planned =
                (arg == 2) ? RunTextFixture(true) : RunFixture(use_m5);
            reply.payload.present.present = planned.present;
            reply.payload.present.full_refresh =
                planned.plan.full_refresh ? 1 : 0;
            reply.payload.present.waveform =
                static_cast<uint8_t>(planned.plan.waveform);
            reply.payload.present.reason =
                static_cast<uint8_t>(planned.plan.reason);
            reply.status = planned.present.status;
            if (!use_m5 && reply.status == StatusCode::Ok) {
                // Owner prints the normalized trace; the console command
                // only round-trips the summary.
                PrintFakeTrace();
            }
            break;
        }
        case ConsoleOp::Widget:
            reply.status = UiRunFixture(event.payload.console.arg);
            break;
        case ConsoleOp::Refresh: {
            const s3paper::RefreshPolicy &policy =
                Planner().policy();
            const s3paper::RefreshHistory &hist = Planner().history();
            RefreshSnapshot &r = reply.payload.refresh;
            r.max_turns_between_full = policy.max_turns_between_full;
            r.max_partial_area_between_full =
                policy.max_partial_area_between_full;
            r.max_elapsed_us_between_full = policy.max_elapsed_us_between_full;
            r.merge_distance = policy.merge_distance;
            r.align_x = policy.align_x;
            r.first_render_done = hist.first_render_done ? 1 : 0;
            r.turns_since_full = hist.turns_since_full;
            r.partial_area_since_full = hist.partial_area_since_full;
            r.fulls_total = hist.fulls_total;
            r.partials_total = hist.partials_total;
            r.merge_fallbacks = hist.merge_fallbacks;
            r.pending_damage = Planner().pending_damage_count();
            break;
        }
        case ConsoleOp::SoakStart: {
            if (s_state.soak.active) {
                reply.status = StatusCode::Busy;
                break;
            }
            const uint32_t target = event.payload.console.arg;
            if (target == 0 || target > 1000000) {
                reply.status = StatusCode::InvalidArgument;
                break;
            }
            s_state.soak = SoakState{};
            s_state.soak.active = true;
            s_state.soak.target = target;
            s_state.soak.heap_free_start =
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            s_state.soak.heap_free_min = s_state.soak.heap_free_start;
            s_state.soak.start_us = esp_timer_get_time();
            ESP_LOGI(kTag, "soak start target=%u",
                     static_cast<unsigned>(target));
            break;
        }
        case ConsoleOp::Reader: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = ReaderOpen();
                    break;
                case 2:
                    reply.status = ReaderNext();
                    break;
                case 3:
                    reply.status = ReaderPrev();
                    break;
                case 4:
                    reply.status = ReaderOpenSd(event.payload.console.arg2);
                    break;
                default:
                    break;
            }
            FillReaderSnapshot(&reply.payload.reader);
            break;
        }
        case ConsoleOp::Sd: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = StorageMount();
                    break;
                case 2:
                    reply.status = StorageUnmount();
                    break;
                case 3:
                    reply.status = StorageWriteDemoBook();
                    break;
                default:
                    break;
            }
            FillSdSnapshot(&reply.payload.sd);
            break;
        }
        case ConsoleOp::Library: {
            if (event.payload.console.arg == 1) {
                uint32_t count = 0;
                reply.status = LibraryScan(&count);
            } else if (event.payload.console.arg == 2) {
                reply.status = LibraryShow();
            }
            // Owner prints the catalog (strings do not fit POD replies).
            LibraryPrint();
            FillSdSnapshot(&reply.payload.sd);
            break;
        }
        case ConsoleOp::Bookmark: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = ReaderBookmarkToggle();
                    break;
                case 2:
                    reply.status =
                        ReaderBookmarkGoto(event.payload.console.arg2);
                    break;
                default:
                    ReaderBookmarksPrint();
                    break;
            }
            FillReaderSnapshot(&reply.payload.reader);
            break;
        }
        case ConsoleOp::Touch: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = TouchEnable();
                    break;
                case 2:
                    TouchDisable();
                    break;
                default:
                    break;
            }
            FillTouchSnapshot(&reply.payload.touch);
            break;
        }
        case ConsoleOp::SoakStatus: {
            const SoakState &s = s_state.soak;
            SoakSnapshot &out = reply.payload.soak;
            out.active = s.active ? 1 : 0;
            out.target = s.target;
            out.completed = s.completed;
            out.errors = s.errors;
            out.fulls = s.fulls;
            out.partials = s.partials;
            out.heap_free_start = s.heap_free_start;
            out.heap_free_now = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            out.heap_free_min = s.heap_free_min;
            out.integrity_checks = s.integrity_checks;
            out.integrity_failures = s.integrity_failures;
            out.elapsed_us =
                (s.active ? esp_timer_get_time() : s.end_us) - s.start_us;
            for (int i = 0; i < 4; ++i) {
                out.by_waveform[i] = s.by_waveform[i];
            }
            break;
        }
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
            if (event.payload.timer.timer_id == kSoakTimerId &&
                s_state.soak.active) {
                s_state.soak.step_queued = false;
                RunOneSoakStep();
            } else if (event.payload.timer.timer_id == kTouchTimerId) {
                InputHandleTick();
            }
            break;
        case AppEventKind::StorageComplete:
            // Counted via processed_by_kind; no model behind it yet.
            break;
        case AppEventKind::ShutdownRequest:
            s_state.phase = Phase::ShuttingDown;
            StorageFlushNow();  // final persistence flush
            ESP_LOGI(kTag, "shutdown requested; commands now report Busy");
            SendReply(event, MakeReply(event, StatusCode::Ok));
            break;
        default:
            break;
    }
}

void OwnerTask(void *) {
    s_state.boot_us = esp_timer_get_time();
    // Display/frame storage is application state: initialize inside the
    // owner task so no other task ever touches it.
    DisplayServiceInit();
    InputServiceInit();
    // Product boot flow: restore the last book (or show the library) and
    // enable touch so the device reads standalone from power-on.
    (void)ReaderBootRestore();
    (void)TouchEnable();
    s_state.phase = Phase::Ready;
    ESP_LOGI(kTag, "owner task ready; queue capacity=%u",
             static_cast<unsigned>(kEventQueueCapacity));
    for (;;) {
        AppEvent event;
        // Bounded wait so deferred work (coalesced persistence flushes)
        // runs even when no events arrive.
        if (xQueueReceive(s_event_queue, &event, pdMS_TO_TICKS(500)) !=
            pdTRUE) {
            StorageFlushIfDue(esp_timer_get_time());
            UiRegionTick(esp_timer_get_time());
            continue;
        }
        const uint32_t depth =
            static_cast<uint32_t>(uxQueueMessagesWaiting(s_event_queue)) + 1;
        if (depth > s_state.queue_high_water) {
            s_state.queue_high_water = depth;
        }
        HandleEvent(event);
        MaybeQueueSoakStep();
        StorageFlushIfDue(esp_timer_get_time());
        UiRegionTick(esp_timer_get_time());
    }
}

}  // namespace

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
