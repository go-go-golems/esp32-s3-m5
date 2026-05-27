#pragma once

#include <stdint.h>

// 16.16 fixed-point arithmetic
typedef int32_t fixed_t;

#define FIXED_SHIFT    16
#define FIXED_ONE      (1 << FIXED_SHIFT)         // 0x10000 = 65536
#define FIXED_HALF     (1 << (FIXED_SHIFT - 1))   // 0x8000 = 32768
#define FIXED_MASK     (FIXED_ONE - 1)            // 0xFFFF

#define FLOAT_TO_FIXED(f)   ((fixed_t)((f) * (float)FIXED_ONE))
#define FIXED_TO_FLOAT(x)   ((float)(x) / (float)FIXED_ONE)
#define INT_TO_FIXED(i)     ((fixed_t)(i) << FIXED_SHIFT)
#define FIXED_TO_INT(x)     ((x) >> FIXED_SHIFT)

// Multiply with 64-bit intermediate to avoid overflow
static inline fixed_t fixed_mul(fixed_t a, fixed_t b) {
    return (fixed_t)(((int64_t)a * b) >> FIXED_SHIFT);
}

// Divide: a / b in fixed-point
static inline fixed_t fixed_div(fixed_t a, fixed_t b) {
    return (fixed_t)(((int64_t)a << FIXED_SHIFT) / b);
}

// Absolute value
static inline fixed_t fixed_abs(fixed_t x) {
    return x < 0 ? -x : x;
}

// Min / max
static inline fixed_t fixed_min(fixed_t a, fixed_t b) {
    return a < b ? a : b;
}
static inline fixed_t fixed_max(fixed_t a, fixed_t b) {
    return a > b ? a : b;
}

// Clamp
static inline fixed_t fixed_clamp(fixed_t x, fixed_t lo, fixed_t hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}
