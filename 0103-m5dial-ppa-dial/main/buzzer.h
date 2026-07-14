// Piezo buzzer on GPIO 3 (HAL_PIN_BUZZER in M5Dial-UserDemo), LEDC-driven.
// tone() is non-blocking; the note stops itself via esp_timer.
#pragma once

#include <cstdint>

bool buzzer_init();
void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms);
