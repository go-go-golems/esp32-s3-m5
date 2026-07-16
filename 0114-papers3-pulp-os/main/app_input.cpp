#include "app_input.h"

#include <atomic>
#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

#include "app_owner.h"
#include "s3paper_runtime/runtime.h"

namespace pulp {
namespace {

const char *kTag = "input";

constexpr uint32_t kPollIntervalMs = 20;
// Quiet window: this long after the last input, the deferred-region
// deadline fires (quiet-while-active regions consume it).
constexpr int64_t kQuietWindowUs = 2'000'000;
constexpr uint32_t kQuietDeadlineId = 1;

// Producer-visible enable flag; all other state is owner-only.
std::atomic<bool> s_enabled{false};

struct InputState {
    s3paper::PointerTracker tracker;
    s3paper::GestureDetector detector;
    s3paper::Scheduler scheduler;
    uint32_t samples = 0;
    uint32_t events_by_kind[4] = {};
    uint32_t gestures_by_kind[6] = {};
    int32_t last_x = -1;
    int32_t last_y = -1;
    uint8_t last_gesture = 0xff;
    int32_t last_gesture_x = -1;
    int32_t last_gesture_y = -1;
    int64_t last_input_us = 0;
    uint32_t quiet_windows = 0;
};

InputState s_state;
GestureHandler s_handler = nullptr;

void TouchTickProducerTask(void *) {
    for (;;) {
        if (s_enabled.load(std::memory_order_relaxed)) {
            AppEvent event;
            std::memset(&event, 0, sizeof(event));
            event.kind = AppEventKind::TimerDue;
            event.source = EventSource::Internal;
            event.producer_seq = NextProducerSeq(EventSource::Internal);
            event.monotonic_us = esp_timer_get_time();
            event.payload.timer.timer_id = kTouchTimerId;
            // A full queue just drops this tick; the next one follows in
            // 20 ms and the rejection is counted per source.
            (void)PostEvent(event);
        }
        vTaskDelay(pdMS_TO_TICKS(kPollIntervalMs));
    }
}

void HandleGesture(const s3paper::GestureEvent &gesture) {
    const uint8_t kind = static_cast<uint8_t>(gesture.kind);
    if (kind < 6) {
        s_state.gestures_by_kind[kind]++;
    }
    s_state.last_gesture = kind;
    s_state.last_gesture_x = gesture.pos.x;
    s_state.last_gesture_y = gesture.pos.y;
    ESP_LOGI(kTag, "gesture %s at %d,%d",
             s3paper::GestureKindName(gesture.kind),
             static_cast<int>(gesture.pos.x),
             static_cast<int>(gesture.pos.y));
    if (s_handler == nullptr || !s_handler(gesture)) {
        ESP_LOGD(kTag, "gesture unconsumed");
    }
}

}  // namespace

void InputServiceInit() {
    static bool started = false;
    if (started) {
        return;
    }
    started = true;
    xTaskCreatePinnedToCore(TouchTickProducerTask, "touch_tick", 3072,
                            nullptr, 4, nullptr, 0);
}

void InputSetGestureHandler(GestureHandler handler) { s_handler = handler; }

StatusCode TouchEnable() {
    const s3paper::Status init = s3paper_runtime::EnsureM5Init();
    if (!init.ok()) {
        return init.code;
    }
    s_enabled.store(true, std::memory_order_relaxed);
    ESP_LOGI(kTag, "touch polling enabled (%ums)",
             static_cast<unsigned>(kPollIntervalMs));
    return StatusCode::Ok;
}

void TouchDisable() {
    s_enabled.store(false, std::memory_order_relaxed);
    ESP_LOGI(kTag, "touch polling disabled");
}

bool TouchEnabled() { return s_enabled.load(std::memory_order_relaxed); }

void InputHandleTick() {
    s3paper::PointerSample sample;
    if (!s3paper_runtime::ReadM5Touch(&sample)) {
        return;
    }
    s_state.samples++;
    s3paper::PointerEvent events[2];
    const uint32_t n = s_state.tracker.Feed(sample, events);
    for (uint32_t i = 0; i < n; ++i) {
        const uint8_t kind = static_cast<uint8_t>(events[i].kind);
        if (kind < 4) {
            s_state.events_by_kind[kind]++;
        }
        s_state.last_x = events[i].pos.x;
        s_state.last_y = events[i].pos.y;
        s_state.last_input_us = events[i].t_us;
        (void)s_state.scheduler.Add(kQuietDeadlineId,
                                    events[i].t_us + kQuietWindowUs);
        s3paper::GestureEvent gesture;
        if (s_state.detector.Feed(events[i], &gesture) == 1) {
            HandleGesture(gesture);
        }
    }
    // Time-driven work: long-press and due deadlines.
    s3paper::GestureEvent gesture;
    if (s_state.detector.Update(sample.t_us, &gesture) == 1) {
        HandleGesture(gesture);
    }
    const s3paper::Result<uint32_t> due =
        s_state.scheduler.PopDue(sample.t_us);
    if (due.ok() && due.value == kQuietDeadlineId) {
        s_state.quiet_windows++;
        ESP_LOGI(kTag, "quiet window reached (%u total)",
                 static_cast<unsigned>(s_state.quiet_windows));
    }
}

void FillTouchSnapshot(TouchSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->enabled = TouchEnabled() ? 1 : 0;
    out->samples = s_state.samples;
    for (int i = 0; i < 4; ++i) {
        out->events_by_kind[i] = s_state.events_by_kind[i];
    }
    for (int i = 0; i < 6; ++i) {
        out->gestures_by_kind[i] = s_state.gestures_by_kind[i];
    }
    out->last_x = s_state.last_x;
    out->last_y = s_state.last_y;
    out->last_gesture = s_state.last_gesture;
    out->last_gesture_x = s_state.last_gesture_x;
    out->last_gesture_y = s_state.last_gesture_y;
    out->quiet_windows = s_state.quiet_windows;
    out->last_input_age_ms =
        s_state.last_input_us == 0
            ? -1
            : static_cast<int64_t>(
                  (esp_timer_get_time() - s_state.last_input_us) / 1000);
}

int64_t InputLastInputUs() { return s_state.last_input_us; }

}  // namespace pulp
