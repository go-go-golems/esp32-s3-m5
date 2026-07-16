#include "app_buzzer.h"

#include <cstdlib>
#include <cstring>

#include "app_events.h"

#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace pulp {
namespace {

const char *kTag = "buzzer";

// Port of M5PaperS3-UserDemo/main/hal/hal.cpp:385 (verified hardware
// facts). Channel 0 is free: nothing else in this firmware uses LEDC.
constexpr gpio_num_t kPin = GPIO_NUM_21;
constexpr ledc_timer_t kTimer = LEDC_TIMER_0;
constexpr ledc_mode_t kMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_channel_t kChannel = LEDC_CHANNEL_0;
constexpr uint32_t kDuty = 4096;  // 50% of 13-bit

constexpr uint32_t kMaxNotes = 16;
constexpr int32_t kMinFreq = 40;
constexpr int32_t kMaxFreq = 12000;
constexpr int32_t kMaxNoteMs = 10000;

struct Note {
    int32_t freq_hz;  // 0 = rest
    int32_t ms;
};

struct BuzzerState {
    bool initialized = false;
    bool sounding = false;
    int64_t stop_at_us = 0;  // 0 = sustain
    Note melody[kMaxNotes] = {};
    uint8_t melody_len = 0;
    uint8_t melody_index = 0;
    bool melody_active = false;
    uint32_t tones_played = 0;
};

BuzzerState s_state;

bool EnsureInit() {
    if (s_state.initialized) {
        return true;
    }
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = kMode;
    timer_cfg.duty_resolution = LEDC_TIMER_13_BIT;
    timer_cfg.timer_num = kTimer;
    timer_cfg.freq_hz = 1000;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer_cfg) != ESP_OK) {
        ESP_LOGE(kTag, "ledc timer config failed");
        return false;
    }
    ledc_channel_config_t channel_cfg = {};
    channel_cfg.gpio_num = kPin;
    channel_cfg.speed_mode = kMode;
    channel_cfg.channel = kChannel;
    channel_cfg.timer_sel = kTimer;
    channel_cfg.intr_type = LEDC_INTR_DISABLE;
    channel_cfg.duty = 0;  // silent until the first tone
    channel_cfg.hpoint = 0;
    if (ledc_channel_config(&channel_cfg) != ESP_OK) {
        ESP_LOGE(kTag, "ledc channel config failed");
        return false;
    }
    s_state.initialized = true;
    ESP_LOGI(kTag, "ledc ready (gpio21 timer0 ch0)");
    return true;
}

void Silence() {
    if (s_state.initialized && s_state.sounding) {
        ledc_set_duty(kMode, kChannel, 0);
        ledc_update_duty(kMode, kChannel);
    }
    s_state.sounding = false;
    s_state.stop_at_us = 0;
}

// Starts one physical tone; rests only arm the stop deadline.
BuzzStatusCode StartNote(int32_t freq_hz, int32_t duration_ms) {
    if (freq_hz != 0 && (freq_hz < kMinFreq || freq_hz > kMaxFreq)) {
        return BuzzStatusCode::InvalidArgument;
    }
    if (duration_ms < 0 || duration_ms > kMaxNoteMs) {
        return BuzzStatusCode::InvalidArgument;
    }
    if (!EnsureInit()) {
        return BuzzStatusCode::Busy;
    }
    if (freq_hz == 0) {
        Silence();
    } else {
        ledc_set_freq(kMode, kTimer, static_cast<uint32_t>(freq_hz));
        ledc_set_duty(kMode, kChannel, kDuty);
        ledc_update_duty(kMode, kChannel);
        s_state.sounding = true;
        s_state.tones_played++;
    }
    s_state.stop_at_us =
        duration_ms > 0
            ? esp_timer_get_time() + static_cast<int64_t>(duration_ms) * 1000
            : 0;
    return BuzzStatusCode::Ok;
}

void MelodyAdvance() {
    while (s_state.melody_active) {
        if (s_state.melody_index >= s_state.melody_len) {
            s_state.melody_active = false;
            Silence();
            return;
        }
        const Note &note = s_state.melody[s_state.melody_index++];
        if (StartNote(note.freq_hz, note.ms) == BuzzStatusCode::Ok) {
            return;
        }
        // Skip an unplayable note rather than abandoning the melody.
    }
}

}  // namespace

BuzzStatusCode BuzzerTone(int32_t freq_hz, int32_t duration_ms) {
    if (freq_hz <= 0) {
        return BuzzStatusCode::InvalidArgument;
    }
    s_state.melody_active = false;  // a direct tone preempts a melody
    return StartNote(freq_hz, duration_ms);
}

BuzzStatusCode BuzzerBeep() { return BuzzerTone(1000, 60); }

void BuzzerStop() {
    s_state.melody_active = false;
    Silence();
}

BuzzStatusCode BuzzerMelody(const char *spec) {
    if (spec == nullptr || spec[0] == '\0') {
        return BuzzStatusCode::InvalidArgument;
    }
    Note parsed[kMaxNotes];
    uint8_t count = 0;
    const char *p = spec;
    while (*p != '\0') {
        if (count >= kMaxNotes) {
            return BuzzStatusCode::CapacityExceeded;
        }
        char *end = nullptr;
        const long freq = strtol(p, &end, 10);
        if (end == p || *end != ':') {
            return BuzzStatusCode::InvalidArgument;
        }
        p = end + 1;
        const long ms = strtol(p, &end, 10);
        if (end == p || ms <= 0 || ms > kMaxNoteMs ||
            (freq != 0 && (freq < kMinFreq || freq > kMaxFreq))) {
            return BuzzStatusCode::InvalidArgument;
        }
        parsed[count].freq_hz = static_cast<int32_t>(freq);
        parsed[count].ms = static_cast<int32_t>(ms);
        count++;
        p = end;
        if (*p == ',') {
            p++;
        } else if (*p != '\0') {
            return BuzzStatusCode::InvalidArgument;
        }
    }
    if (count == 0) {
        return BuzzStatusCode::InvalidArgument;
    }
    std::memcpy(s_state.melody, parsed, sizeof(parsed[0]) * count);
    s_state.melody_len = count;
    s_state.melody_index = 0;
    s_state.melody_active = true;
    MelodyAdvance();
    return BuzzStatusCode::Ok;
}

void BuzzerTick(int64_t now_us) {
    if (s_state.stop_at_us == 0 || now_us < s_state.stop_at_us) {
        return;
    }
    s_state.stop_at_us = 0;
    if (s_state.melody_active) {
        MelodyAdvance();
    } else {
        Silence();
    }
}

void FillBuzzSnapshot(BuzzSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->initialized = s_state.initialized ? 1 : 0;
    out->playing = s_state.sounding ? 1 : 0;
    out->melody_active = s_state.melody_active ? 1 : 0;
    out->melody_len = s_state.melody_len;
    out->melody_index = s_state.melody_index;
    out->tones_played = s_state.tones_played;
}

}  // namespace pulp
