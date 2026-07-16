#include "app_power.h"

#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_input.h"
#include "s3paper/text.h"
#include "s3paper/widget.h"
#include "s3paper_m5/m5_power.h"
#include "s3paper_runtime/runtime.h"
#include "s3paper_storage/storage.h"

namespace pulp {
namespace {

const char *kTag = "power";

uint32_t s_auto_sleep_sec = 0;  // 0 = disabled
int64_t s_boot_us = 0;
SleepImageBuilder s_sleep_image_builder = nullptr;

const char *SleepModeName(SleepMode mode) {
    switch (mode) {
        case SleepMode::DeepTimer: return "deep-timer";
        case SleepMode::RtcOff: return "rtc-off";
        case SleepMode::Off: return "off";
    }
    return "?";
}

// Placeholder sleep image until Phase 8 hands this to JS: PULP wordmark,
// state line, wake hint. E-paper keeps it without power.
s3paper::WidgetHandle BuildPlaceholderSleepImage(SleepMode mode,
                                                 uint32_t seconds) {
    s3paper::WidgetArena &a = s3paper_runtime::Arena();
    a.Reset();
    s3paper::WidgetHandle content = s3paper::NewCol(a).value;
    s3paper::WidgetNode *cn = a.Configure(content);
    if (cn == nullptr) {
        return s3paper::kNullWidget;
    }
    cn->padding = s3paper::Insets{0, 40, 0, 40};
    cn->gap = 24;
    cn->main_align = s3paper::MainAlign::Center;
    (void)a.AddChild(content,
                     s3paper::NewText(a, "PULP", s3paper::kFontXL, 0,
                                      s3paper::TextAlign::Center)
                         .value);
    (void)a.AddChild(content,
                     s3paper::NewText(a, "asleep", s3paper::kFontDisplay, 0,
                                      s3paper::TextAlign::Center)
                         .value);
    char hint[s3paper::TextProps::kCapacity];
    if (mode == SleepMode::DeepTimer || mode == SleepMode::RtcOff) {
        snprintf(hint, sizeof(hint), "waking in %u s",
                 static_cast<unsigned>(seconds));
    } else {
        snprintf(hint, sizeof(hint), "press the side button to wake");
    }
    (void)a.AddChild(content,
                     s3paper::NewText(a, hint, s3paper::kFontUi, 96,
                                      s3paper::TextAlign::Center)
                         .value);
    return content;
}

StatusCode PresentSleepImage(SleepMode mode, uint32_t seconds) {
    s3paper::WidgetHandle content = s3paper::kNullWidget;
    if (s_sleep_image_builder != nullptr) {
        content = s_sleep_image_builder(mode, seconds);
    }
    if (s3paper::IsNull(content)) {
        content = BuildPlaceholderSleepImage(mode, seconds);
    }
    if (s3paper::IsNull(content)) {
        return StatusCode::CapacityExceeded;
    }
    const s3paper::PageSlots slots{s3paper::kNullWidget, content,
                                   s3paper::kNullWidget,
                                   s3paper::kNullWidget};
    const s3paper_runtime::PresentPageResult presented =
        s3paper_runtime::PresentPage(slots,
                                     s3paper::PresentIntent::CleanFull,
                                     true, nullptr, 0, nullptr);
    return presented.status;
}

}  // namespace

void PowerSetSleepImageBuilder(SleepImageBuilder builder) {
    s_sleep_image_builder = builder;
}

StatusCode PowerSleep(SleepMode mode, uint32_t seconds) {
    const s3paper::Status init = s3paper_runtime::EnsureM5Init();
    if (!init.ok()) {
        return init.code;
    }
    ESP_LOGI(kTag, "sleep: mode=%s seconds=%u (quiesce begins)",
             SleepModeName(mode), static_cast<unsigned>(seconds));
    // 1. Input: stop the touch tick producer.
    TouchDisable();
    // 2. Persistence: everything dirty goes to the card now.
    s3paper_storage::StorageFlushNow();
    // 3. Retained sleep image; the planner present waits for EPD idle.
    const StatusCode image = PresentSleepImage(mode, seconds);
    if (image != StatusCode::Ok) {
        ESP_LOGW(kTag, "sleep image failed: %s (sleeping anyway)",
                 StatusCodeName(image));
    }
    // 4. SD: unmount so the card is quiescent when power drops.
    if (s3paper_storage::StorageMounted()) {
        (void)s3paper_storage::StorageUnmount();
    }
    // 5. Console: let the transcript reach the host.
    ESP_LOGI(kTag, "entering %s sleep", SleepModeName(mode));
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));
    // 6. Wake source + transition (never returns).
    switch (mode) {
        case SleepMode::DeepTimer:
            s3paper::PowerDeepSleep(static_cast<uint64_t>(seconds) *
                                    1000000ULL);
        case SleepMode::RtcOff:
            s3paper::PowerRtcOff(static_cast<int32_t>(seconds));
        case SleepMode::Off:
            s3paper::PowerOff();
    }
    return StatusCode::InvalidArgument;  // unreachable
}

void FillPowerSnapshot(PowerSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    const s3paper::Status init = s3paper_runtime::EnsureM5Init();
    if (init.ok()) {
        const s3paper::PowerStatus st = s3paper::PowerRead();
        out->battery_level = st.battery_level;
        out->battery_mv = st.battery_mv;
        out->charging = st.charging ? 1 : 0;
        out->wakeup_cause = st.wakeup_cause;
        out->reset_reason = st.reset_reason;
    } else {
        out->battery_level = -1;
        out->battery_mv = -1;
    }
    out->auto_sleep_sec = s_auto_sleep_sec;
}

void PowerSetAutoSleep(uint32_t seconds) {
    s_auto_sleep_sec = seconds;
    if (s_boot_us == 0) {
        s_boot_us = esp_timer_get_time();
    }
    ESP_LOGI(kTag, "auto-sleep %s (%u s)",
             seconds == 0 ? "disabled" : "armed",
             static_cast<unsigned>(seconds));
}

void PowerAutoTick(int64_t now_us) {
    // Low-battery shutdown: ADC level AND not charging, sampled at most
    // every 30 s, only after M5 is up (never force an init just to read
    // the battery). Shutdown goes through the same quiesce sequence.
    static int64_t s_last_batt_check_us = 0;
    if (s3paper_runtime::M5BackendState().initialized &&
        now_us - s_last_batt_check_us > 30'000'000) {
        s_last_batt_check_us = now_us;
        const s3paper::PowerStatus st = s3paper::PowerRead();
        if (st.battery_level >= 0 && st.battery_level <= 5 &&
            !st.charging) {
            ESP_LOGW(kTag, "battery critical (%d%%, %d mV); powering off",
                     static_cast<int>(st.battery_level),
                     static_cast<int>(st.battery_mv));
            (void)PowerSleep(SleepMode::Off, 0);
        }
    }
    if (s_auto_sleep_sec == 0) {
        return;
    }
    int64_t idle_since = InputLastInputUs();
    if (idle_since == 0) {
        idle_since = s_boot_us != 0 ? s_boot_us : now_us;
    }
    if (now_us - idle_since <
        static_cast<int64_t>(s_auto_sleep_sec) * 1000000) {
        return;
    }
    ESP_LOGI(kTag, "inactivity limit reached (%u s); powering off",
             static_cast<unsigned>(s_auto_sleep_sec));
    (void)PowerSleep(SleepMode::Off, 0);
}

void PowerLogBootCause() {
    s_boot_us = esp_timer_get_time();
    ESP_LOGI(kTag, "boot: reset_reason=%d wakeup_cause=%d",
             static_cast<int>(esp_reset_reason()),
             static_cast<int>(esp_sleep_get_wakeup_cause()));
}

}  // namespace pulp
