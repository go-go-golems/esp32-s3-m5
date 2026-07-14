#include "buzzer.h"

#include "driver/ledc.h"
#include "esp_timer.h"

namespace {
constexpr int kBuzzerGpio = 3;
constexpr ledc_timer_t kTimer = LEDC_TIMER_1;
constexpr ledc_channel_t kChannel = LEDC_CHANNEL_1;
esp_timer_handle_t s_stop_timer = nullptr;

void stop_cb(void *) { ledc_set_duty(LEDC_LOW_SPEED_MODE, kChannel, 0); ledc_update_duty(LEDC_LOW_SPEED_MODE, kChannel); }
} // namespace

bool buzzer_init() {
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_cfg.timer_num = kTimer;
    timer_cfg.duty_resolution = LEDC_TIMER_10_BIT;
    timer_cfg.freq_hz = 4000;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer_cfg) != ESP_OK) return false;

    ledc_channel_config_t ch_cfg = {};
    ch_cfg.gpio_num = kBuzzerGpio;
    ch_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_cfg.channel = kChannel;
    ch_cfg.timer_sel = kTimer;
    ch_cfg.duty = 0;
    if (ledc_channel_config(&ch_cfg) != ESP_OK) return false;

    const esp_timer_create_args_t args = {.callback = stop_cb,
                                          .arg = nullptr,
                                          .dispatch_method = ESP_TIMER_TASK,
                                          .name = "buzz_stop",
                                          .skip_unhandled_events = true};
    return esp_timer_create(&args, &s_stop_timer) == ESP_OK;
}

void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (s_stop_timer == nullptr) return;
    esp_timer_stop(s_stop_timer);
    ledc_set_freq(LEDC_LOW_SPEED_MODE, kTimer, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, kChannel, 512); // 50% of 10-bit
    ledc_update_duty(LEDC_LOW_SPEED_MODE, kChannel);
    esp_timer_start_once(s_stop_timer, static_cast<uint64_t>(duration_ms) * 1000);
}
