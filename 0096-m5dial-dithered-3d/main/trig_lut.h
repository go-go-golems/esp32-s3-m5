#pragma once

#include <stdint.h>

// Sin/cos lookup table for camera orbit
// 1024 entries covering [0, 2π), stored in flash as const
// Each entry is a Q15 signed integer: 1.0 = 32767, -1.0 = -32768

#define TRIG_LUT_SIZE 1024
#define TRIG_LUT_MASK (TRIG_LUT_SIZE - 1)

// Access sin/cos by angle index (0–1023 = 0–2π)
// Returns Q15 value: sin(0)=0, sin(π/2)=32767, sin(π)=0, sin(3π/2)=-32768
int16_t trig_sin(int idx);
int16_t trig_cos(int idx);

// Convert float angle (radians) to LUT index
static inline int trig_angle_to_idx(float radians) {
    // Normalize to [0, 2π)
    const float TWO_PI = 6.283185307f;
    float norm = radians - (int)(radians / TWO_PI) * TWO_PI;
    if (norm < 0) norm += TWO_PI;
    return (int)(norm / TWO_PI * TRIG_LUT_SIZE) & TRIG_LUT_MASK;
}

// Convert LUT index to float
static inline float trig_idx_to_float(int idx) {
    return (float)(idx & TRIG_LUT_MASK) / (float)TRIG_LUT_SIZE * 6.283185307f;
}

// Get float sin/cos from radians directly
static inline float trig_sin_f(float radians) {
    return (float)trig_sin(trig_angle_to_idx(radians)) / 32768.0f;
}
static inline float trig_cos_f(float radians) {
    return (float)trig_cos(trig_angle_to_idx(radians)) / 32768.0f;
}
