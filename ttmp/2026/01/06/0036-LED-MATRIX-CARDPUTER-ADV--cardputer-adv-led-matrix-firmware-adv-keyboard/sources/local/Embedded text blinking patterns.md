---
Title: "Embedded text blinking patterns (local notes)"
Ticket: 0036-LED-MATRIX-CARDPUTER-ADV
Status: active
Topics:
  - cardputer
  - esp32s3
DocType: reference
Intent: short-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Local reference: 5x7 font + text-centric animation pseudocode for a 4x 8x8 (32x8) LED matrix."
LastUpdated: 2026-01-06T21:37:58-05:00
WhatFor: ""
WhenToUse: ""
---

# LED Matrix Animation Algorithms
## Embedded C Pseudocode Reference

This document provides embedded C implementations for 12 text-based LED matrix animations designed for a 4x8x8 (32x8) monochrome LED display running at 10-15 FPS.

---

## Table of Contents

1. [Common Infrastructure](#common-infrastructure)
2. [Easing Functions](#easing-functions)
3. [Font Data & Text Rendering](#font-data--text-rendering)
4. [Animation 1: Smooth Scroll](#animation-1-smooth-scroll)
5. [Animation 2: Drop Bounce](#animation-2-drop-bounce)
6. [Animation 3: Typewriter](#animation-3-typewriter)
7. [Animation 4: Wave Text](#animation-4-wave-text)
8. [Animation 5: Zoom Elastic](#animation-5-zoom-elastic)
9. [Animation 6: Slide Meet](#animation-6-slide-meet)
10. [Animation 7: Flip Board](#animation-7-flip-board)
11. [Animation 8: Glitch](#animation-8-glitch)
12. [Animation 9: Countdown](#animation-9-countdown)
13. [Animation 10: Shutter Reveal](#animation-10-shutter-reveal)
14. [Animation 11: Column Reveal](#animation-11-column-reveal)
15. [Animation 12: Spin Letters](#animation-12-spin-letters)

---

## Common Infrastructure

### Display Configuration

```c
/*
 * LED MATRIX CONFIGURATION
 * 
 * Physical layout: 4 cascaded 8x8 LED matrices
 * Total resolution: 32 columns x 8 rows
 * Origin (0,0): Top-left corner
 * 
 * Memory layout: Column-major for efficient shifting
 * Each column stored as uint8_t (8 bits = 8 rows)
 * Bit 0 = top row, Bit 7 = bottom row
 */

#define MATRIX_WIDTH    32
#define MATRIX_HEIGHT   8
#define NUM_MATRICES    4
#define MATRIX_SIZE     8

#define FPS             15
#define FRAME_MS        (1000 / FPS)

/* Frame buffer: 32 bytes, one per column */
typedef struct {
    uint8_t columns[MATRIX_WIDTH];
} FrameBuffer;

/* Animation state container */
typedef struct {
    uint16_t frame;           /* Current frame counter */
    uint16_t cycle_frame;     /* Frame within current cycle */
    uint8_t  phase;           /* Animation phase (intro/hold/exit) */
    void*    custom_data;     /* Animation-specific state */
} AnimState;

/* Clear entire frame buffer */
void fb_clear(FrameBuffer* fb) {
    memset(fb->columns, 0, MATRIX_WIDTH);
}

/* Set single pixel */
void fb_set_pixel(FrameBuffer* fb, int8_t x, int8_t y, uint8_t on) {
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
    if (on) {
        fb->columns[x] |= (1 << y);
    } else {
        fb->columns[x] &= ~(1 << y);
    }
}

/* Get pixel state */
uint8_t fb_get_pixel(FrameBuffer* fb, int8_t x, int8_t y) {
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return 0;
    return (fb->columns[x] >> y) & 1;
}
```

---

## Easing Functions

### Concept

Easing functions transform linear time `t ∈ [0,1]` into curved motion. They create natural-feeling acceleration and deceleration instead of robotic linear movement.

```
Linear:        |████████████████|  (constant speed)
Ease-In:       |█▂▃▄▅▆▇████████|  (slow start, fast end)
Ease-Out:      |████████▇▆▅▄▃▂█|  (fast start, slow end)
Ease-In-Out:   |█▂▄▆████████▆▄▂|  (slow both ends)
Bounce:        |████▆▄▂▄▆████▂▄|  (overshoot and settle)
```

### Fixed-Point Implementation

```c
/*
 * FIXED-POINT EASING FUNCTIONS
 * 
 * Using Q8.8 fixed-point: 8 integer bits, 8 fractional bits
 * Range: 0x0000 (0.0) to 0x0100 (1.0)
 * 
 * Input:  t in range [0, 256] representing [0.0, 1.0]
 * Output: eased value in range [0, 256]
 * 
 * For embedded systems without FPU, these avoid floating-point entirely.
 */

typedef int16_t fixed8_t;  /* Q8.8 fixed point */

#define FIXED_ONE   256
#define FIXED_HALF  128

/* Multiply two Q8.8 values */
static inline fixed8_t fixed_mul(fixed8_t a, fixed8_t b) {
    return (fixed8_t)(((int32_t)a * b) >> 8);
}

/*
 * ease_linear: No easing, linear interpolation
 * 
 * f(t) = t
 */
fixed8_t ease_linear(fixed8_t t) {
    return t;
}

/*
 * ease_in_quad: Quadratic ease-in (slow start)
 * 
 * f(t) = t²
 * 
 * Creates gradual acceleration from rest.
 * Good for: objects starting to move, fade-ins
 */
fixed8_t ease_in_quad(fixed8_t t) {
    return fixed_mul(t, t);
}

/*
 * ease_out_quad: Quadratic ease-out (slow end)
 * 
 * f(t) = 1 - (1-t)²  =  t(2-t)
 * 
 * Creates gradual deceleration to rest.
 * Good for: objects coming to stop, fade-outs
 */
fixed8_t ease_out_quad(fixed8_t t) {
    fixed8_t inv = FIXED_ONE - t;
    return FIXED_ONE - fixed_mul(inv, inv);
}

/*
 * ease_in_out_quad: Quadratic ease-in-out
 * 
 * f(t) = 2t²           if t < 0.5
 *      = 1 - 2(1-t)²   if t >= 0.5
 * 
 * Smooth acceleration then deceleration.
 * Good for: scrolling text, camera moves
 */
fixed8_t ease_in_out_quad(fixed8_t t) {
    if (t < FIXED_HALF) {
        /* First half: ease in */
        return fixed_mul(t, t) >> 7;  /* 2t² */
    } else {
        /* Second half: ease out */
        fixed8_t inv = FIXED_ONE - t;
        return FIXED_ONE - (fixed_mul(inv, inv) >> 7);
    }
}

/*
 * ease_in_cubic: Cubic ease-in (slower start than quad)
 * 
 * f(t) = t³
 */
fixed8_t ease_in_cubic(fixed8_t t) {
    return fixed_mul(fixed_mul(t, t), t);
}

/*
 * ease_out_cubic: Cubic ease-out (slower end than quad)
 * 
 * f(t) = 1 - (1-t)³
 */
fixed8_t ease_out_cubic(fixed8_t t) {
    fixed8_t inv = FIXED_ONE - t;
    fixed8_t inv3 = fixed_mul(fixed_mul(inv, inv), inv);
    return FIXED_ONE - inv3;
}

/*
 * ease_in_out_cubic: Smooth S-curve
 * 
 * f(t) = 4t³           if t < 0.5
 *      = 1 - 4(1-t)³   if t >= 0.5
 * 
 * Very smooth, natural-feeling motion.
 * Good for: main scrolling animations, UI transitions
 */
fixed8_t ease_in_out_cubic(fixed8_t t) {
    if (t < FIXED_HALF) {
        return fixed_mul(fixed_mul(t, t), t) >> 6;  /* 4t³ */
    } else {
        fixed8_t inv = FIXED_ONE - t;
        return FIXED_ONE - (fixed_mul(fixed_mul(inv, inv), inv) >> 6);
    }
}

/*
 * ease_out_bounce: Bouncing ball effect
 * 
 * Simulates elastic collision with ground.
 * Uses piecewise quadratic approximation.
 * 
 * Good for: dropping letters, playful UI
 */
fixed8_t ease_out_bounce(fixed8_t t) {
    /* 
     * Approximation using lookup + interpolation
     * Original formula has 4 quadratic segments
     */
    static const uint8_t bounce_lut[17] = {
        0, 8, 30, 68, 120, 188, 240, 222, 
        240, 250, 240, 248, 255, 252, 255, 255, 255
    };
    
    uint8_t idx = t >> 4;  /* 0-16 */
    uint8_t frac = t & 0x0F;
    
    if (idx >= 16) return FIXED_ONE;
    
    uint8_t v0 = bounce_lut[idx];
    uint8_t v1 = bounce_lut[idx + 1];
    
    return v0 + (((v1 - v0) * frac) >> 4);
}

/*
 * ease_out_elastic: Spring/rubber band effect
 * 
 * f(t) = 2^(-10t) * sin((t-0.1) * 5π) + 1
 * 
 * Overshoots target then oscillates to rest.
 * Good for: zoom effects, attention-grabbing reveals
 * 
 * Note: Expensive due to sin approximation. Consider LUT.
 */
fixed8_t ease_out_elastic(fixed8_t t) {
    if (t == 0) return 0;
    if (t >= FIXED_ONE) return FIXED_ONE;
    
    /* Simplified LUT-based approximation */
    static const int16_t elastic_lut[17] = {
        0, 128, 280, 300, 270, 256, 260, 255,
        257, 256, 256, 256, 256, 256, 256, 256, 256
    };
    
    uint8_t idx = t >> 4;
    return elastic_lut[idx > 16 ? 16 : idx];
}

/*
 * ease_in_back: Anticipation (pulls back before moving)
 * 
 * f(t) = t² * (2.7t - 1.7)
 * 
 * Goes slightly negative before moving forward.
 * Good for: exit animations, "wind up" effects
 */
fixed8_t ease_in_back(fixed8_t t) {
    /* Approximation: t² * (2.7t - 1.7) */
    /* Simplified: t³ * 2.7 - t² * 1.7 */
    fixed8_t t2 = fixed_mul(t, t);
    fixed8_t t3 = fixed_mul(t2, t);
    
    /* 2.7 ≈ 691/256, 1.7 ≈ 435/256 */
    int32_t result = ((int32_t)t3 * 691 - (int32_t)t2 * 435) >> 8;
    
    /* Clamp (can go negative due to back effect) */
    if (result < 0) result = 0;
    if (result > FIXED_ONE) result = FIXED_ONE;
    
    return (fixed8_t)result;
}

/*
 * ease_out_back: Overshoot then settle
 * 
 * f(t) = 1 + (t-1)² * (2.7(t-1) + 1.7)
 * 
 * Goes past target then returns.
 * Good for: pop-in effects, bouncy reveals
 */
fixed8_t ease_out_back(fixed8_t t) {
    fixed8_t s = t - FIXED_ONE;  /* s = t-1, negative */
    fixed8_t s2 = fixed_mul(s, s);
    
    /* (t-1)² * (2.7(t-1) + 1.7) + 1 */
    /* Since s is negative, 2.7s + 1.7 handles the overshoot */
    int32_t factor = ((int32_t)s * 691 >> 8) + 435;
    int32_t result = FIXED_ONE + ((int32_t)s2 * factor >> 8);
    
    if (result > FIXED_ONE) result = FIXED_ONE;
    
    return (fixed8_t)result;
}
```

---

## Font Data & Text Rendering

### 5x7 Bitmap Font

```c
/*
 * BITMAP FONT SYSTEM
 * 
 * Each character is 5 columns wide, stored as 5 bytes.
 * Each byte represents one column, LSB = top row.
 * 
 * Character cell: 5x7 pixels + 1 column spacing = 6 pixels per char
 * 
 * Example 'A':
 *   Col:  0    1    2    3    4
 *        0x7C 0x12 0x11 0x12 0x7C
 * 
 *   Bit 0: .  #  #  #  .     (0x7C = 01111100)
 *   Bit 1: #  .  .  .  #     Row 1 uses bit 1 of each byte
 *   Bit 2: #  .  .  .  #
 *   Bit 3: #  #  #  #  #     (all have bit 3 set)
 *   Bit 4: #  .  .  .  #
 *   Bit 5: #  .  .  .  #
 *   Bit 6: #  .  .  .  #
 */

#define CHAR_WIDTH   5
#define CHAR_SPACING 1
#define CHAR_CELL    (CHAR_WIDTH + CHAR_SPACING)  /* 6 */

/* Font data stored in program memory (Flash) */
static const uint8_t PROGMEM font_data[][5] = {
    /* ASCII 32-90 subset shown, extend as needed */
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!' - 32] = {0x00, 0x00, 0x5F, 0x00, 0x00},
    /* ... */
    ['A' - 32] = {0x7C, 0x12, 0x11, 0x12, 0x7C},
    ['B' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D' - 32] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G' - 32] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H' - 32] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I' - 32] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['J' - 32] = {0x20, 0x40, 0x41, 0x3F, 0x01},
    ['K' - 32] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L' - 32] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M' - 32] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N' - 32] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['Q' - 32] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    ['R' - 32] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S' - 32] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T' - 32] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U' - 32] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V' - 32] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W' - 32] = {0x3F, 0x40, 0x38, 0x40, 0x3F},
    ['X' - 32] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y' - 32] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z' - 32] = {0x61, 0x51, 0x49, 0x45, 0x43},
    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6' - 32] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7' - 32] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8' - 32] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9' - 32] = {0x06, 0x49, 0x49, 0x29, 0x1E},
};

/*
 * Get font column data for a character
 * Returns pointer to 5-byte array in program memory
 */
const uint8_t* font_get_char(char c) {
    if (c >= 32 && c <= 90) {
        return font_data[c - 32];
    }
    return font_data[0];  /* Space for unknown */
}

/*
 * Calculate pixel width of string
 */
uint16_t text_width(const char* str) {
    uint16_t len = strlen(str);
    return len * CHAR_CELL;
}

/*
 * Render character at position with Y offset
 * Returns number of columns drawn (6)
 */
uint8_t render_char(FrameBuffer* fb, char c, int16_t x, int8_t y_offset) {
    const uint8_t* glyph = font_get_char(c);
    
    for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
        int16_t draw_x = x + col;
        if (draw_x < 0 || draw_x >= MATRIX_WIDTH) continue;
        
        uint8_t column_data = pgm_read_byte(&glyph[col]);
        
        /* Apply Y offset by shifting */
        if (y_offset > 0) {
            column_data <<= y_offset;
        } else if (y_offset < 0) {
            column_data >>= (-y_offset);
        }
        
        fb->columns[draw_x] |= column_data;
    }
    
    return CHAR_CELL;
}

/*
 * Render string centered on display
 */
void render_text_centered(FrameBuffer* fb, const char* str, int8_t y_offset) {
    uint16_t width = text_width(str);
    int16_t start_x = (MATRIX_WIDTH - width) / 2;
    
    for (const char* p = str; *p; p++) {
        start_x += render_char(fb, *p, start_x, y_offset);
    }
}
```

---

## Animation 1: Smooth Scroll

### Concept

Text scrolls horizontally across the display with smooth acceleration at the start and deceleration at the end, rather than constant velocity. This creates a more natural, polished appearance.

```
Timeline:
|--PAUSE--|--------SCROLL (eased)--------|--PAUSE--|
     ↓              ↓                          ↓
  Text visible   ease-in-out-cubic        Text gone
  at start       acceleration curve       off screen
```

### Pseudocode

```c
/*
 * SMOOTH SCROLL ANIMATION
 * 
 * Phases:
 *   1. PAUSE_START: Text visible at initial position
 *   2. SCROLL: Text moves with easeInOutCubic curve
 *   3. PAUSE_END: Brief pause before cycle restart
 * 
 * The easing creates:
 *   - Gentle acceleration as scroll begins
 *   - Fast movement through middle
 *   - Gentle deceleration as scroll ends
 */

typedef struct {
    const char* text;
    uint16_t text_width;      /* Cached pixel width */
    uint16_t scroll_distance; /* Total pixels to scroll */
    uint16_t pause_duration;  /* Frames to pause */
    uint16_t scroll_duration; /* Frames for scroll */
    uint16_t cycle_duration;  /* Total cycle length */
} SmoothScrollConfig;

void smooth_scroll_init(SmoothScrollConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->text_width = text_width(text);
    cfg->scroll_distance = cfg->text_width + MATRIX_WIDTH;
    cfg->pause_duration = 30;
    cfg->scroll_duration = cfg->scroll_distance * 3;  /* ~3 frames per pixel */
    cfg->cycle_duration = cfg->scroll_duration + cfg->pause_duration * 2;
}

void smooth_scroll_render(FrameBuffer* fb, SmoothScrollConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    int16_t offset;
    
    if (cycle_frame < cfg->pause_duration) {
        /*
         * Phase 1: Pause at start
         * Text positioned so it's fully visible on left
         */
        offset = 0;
    }
    else if (cycle_frame < cfg->pause_duration + cfg->scroll_duration) {
        /*
         * Phase 2: Scrolling with easing
         * 
         * 1. Calculate linear progress t ∈ [0, 256]
         * 2. Apply easeInOutCubic to get curved progress
         * 3. Map to pixel offset
         */
        uint16_t scroll_frame = cycle_frame - cfg->pause_duration;
        
        /* Linear progress: 0-256 */
        fixed8_t t = (scroll_frame * FIXED_ONE) / cfg->scroll_duration;
        
        /* Apply easing curve */
        fixed8_t eased_t = ease_in_out_cubic(t);
        
        /* Convert to pixel offset */
        /* offset moves from -MATRIX_WIDTH to text_width */
        offset = ((int32_t)eased_t * cfg->scroll_distance / FIXED_ONE) - MATRIX_WIDTH;
    }
    else {
        /*
         * Phase 3: Pause at end
         * Text has scrolled off right side
         */
        offset = cfg->scroll_distance - MATRIX_WIDTH;
    }
    
    /*
     * Render text with current offset
     * Only visible portion is drawn (clipped by render_char)
     */
    int16_t draw_x = -offset;
    for (const char* p = cfg->text; *p; p++) {
        draw_x += render_char(fb, *p, draw_x, 0);
    }
}
```

---

## Animation 2: Drop Bounce

### Concept

Each letter drops from above the display and bounces when it "hits" the baseline. Letters are staggered so they drop sequentially, creating a cascade effect. Exit animation drops letters down off-screen.

```
Frame 0:     Frame 10:    Frame 20:    Frame 40:
  F            F            FUNK         FUNKY
               U              Y           (all settled)
               N
               K
(letters drop in sequence with bounce)
```

### Pseudocode

```c
/*
 * DROP BOUNCE ANIMATION
 * 
 * Each letter has independent timing:
 *   - Start time: charIndex * CHAR_DELAY
 *   - Drop duration: EASE_OUT_BOUNCE over 15 frames
 *   - Bounce: overshoots then settles (simulates elasticity)
 * 
 * Exit: all letters fall simultaneously with EASE_IN_CUBIC
 */

#define DROP_DURATION    15
#define HOLD_DURATION    60
#define EXIT_DURATION    20
#define CHAR_DELAY       8

typedef struct {
    const char* text;
    uint8_t num_chars;
    int16_t start_x;
    uint16_t cycle_duration;
} DropBounceConfig;

void drop_bounce_init(DropBounceConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->num_chars = strlen(text);
    cfg->start_x = (MATRIX_WIDTH - text_width(text)) / 2;
    cfg->cycle_duration = cfg->num_chars * CHAR_DELAY + 
                          DROP_DURATION + HOLD_DURATION + EXIT_DURATION;
}

void drop_bounce_render(FrameBuffer* fb, DropBounceConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    uint16_t exit_start = cfg->num_chars * CHAR_DELAY + DROP_DURATION + HOLD_DURATION;
    
    for (uint8_t i = 0; i < cfg->num_chars; i++) {
        /* Calculate this character's start time */
        uint16_t char_start = i * CHAR_DELAY;
        
        /* Skip if character hasn't started yet */
        if (cycle_frame < char_start) continue;
        
        int16_t char_frame = cycle_frame - char_start;
        int8_t y_offset;
        
        if (cycle_frame >= exit_start) {
            /*
             * EXIT PHASE: All characters fall down
             * 
             * Use easeInCubic for accelerating fall
             * Maps t=0→y=0, t=1→y=16 (off bottom)
             */
            uint16_t exit_frame = cycle_frame - exit_start;
            fixed8_t t = (exit_frame * FIXED_ONE) / EXIT_DURATION;
            if (t > FIXED_ONE) t = FIXED_ONE;
            
            fixed8_t eased = ease_in_cubic(t);
            y_offset = (eased * 16) / FIXED_ONE;  /* Fall 16 pixels down */
        }
        else if (char_frame < DROP_DURATION) {
            /*
             * DROP PHASE: Character dropping in
             * 
             * easeOutBounce: starts at 1.0 (high), ends at 0.0 (baseline)
             * Invert: (1 - eased) * -12 gives Y offset from above
             * 
             * Bounce creates 2-3 smaller bounces before settling
             */
            fixed8_t t = (char_frame * FIXED_ONE) / DROP_DURATION;
            fixed8_t eased = ease_out_bounce(t);
            
            /* eased goes 0→1, we want offset to go -12→0 */
            y_offset = ((FIXED_ONE - eased) * -12) / FIXED_ONE;
        }
        else {
            /*
             * HOLD PHASE: Character at rest
             */
            y_offset = 0;
        }
        
        /* Render character at computed position */
        int16_t x = cfg->start_x + i * CHAR_CELL;
        render_char(fb, cfg->text[i], x, y_offset);
    }
}
```

---

## Animation 3: Typewriter

### Concept

Characters appear one at a time as if being typed, with a blinking cursor. Backspace/erase effect removes characters when animation resets.

```
Frame 0:   |           (cursor blinks)
Frame 6:   H|          (H typed)
Frame 12:  HE|         (E typed)
Frame 18:  HEL|        ...
...
Frame N:   HELLO|      (all typed, hold)
Frame N+M: HELL|       (erasing)
Frame N+M+X: |         (cleared)
```

### Pseudocode

```c
/*
 * TYPEWRITER ANIMATION
 * 
 * Phases:
 *   1. TYPING: Characters appear with TYPE_DELAY between each
 *   2. HOLD: Full text displayed with blinking cursor
 *   3. ERASE: Characters removed with easeInQuad (accelerating erase)
 * 
 * Cursor: Vertical bar that blinks every CURSOR_BLINK frames
 */

#define TYPE_DELAY      6
#define CURSOR_BLINK    8
#define TYPE_HOLD       40
#define ERASE_DURATION  30

typedef struct {
    const char* text;
    uint8_t num_chars;
    int16_t start_x;
    uint16_t type_duration;
    uint16_t cycle_duration;
} TypewriterConfig;

void typewriter_init(TypewriterConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->num_chars = strlen(text);
    cfg->start_x = (MATRIX_WIDTH - text_width(text)) / 2;
    cfg->type_duration = cfg->num_chars * TYPE_DELAY;
    cfg->cycle_duration = cfg->type_duration + TYPE_HOLD + ERASE_DURATION + 20;
}

void typewriter_render(FrameBuffer* fb, TypewriterConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    uint8_t chars_visible;
    int16_t cursor_x;
    uint8_t show_cursor;
    
    if (cycle_frame < cfg->type_duration) {
        /*
         * TYPING PHASE
         * 
         * Characters appear at regular intervals.
         * chars_visible = floor(frame / TYPE_DELAY)
         */
        chars_visible = cycle_frame / TYPE_DELAY;
        cursor_x = cfg->start_x + chars_visible * CHAR_CELL;
        show_cursor = (frame / CURSOR_BLINK) % 2 == 0;
    }
    else if (cycle_frame < cfg->type_duration + TYPE_HOLD) {
        /*
         * HOLD PHASE
         * 
         * All characters visible, cursor continues blinking
         */
        chars_visible = cfg->num_chars;
        cursor_x = cfg->start_x + cfg->num_chars * CHAR_CELL;
        show_cursor = (frame / CURSOR_BLINK) % 2 == 0;
    }
    else {
        /*
         * ERASE PHASE
         * 
         * Characters removed with easeInQuad (accelerating)
         * Feels like holding backspace - starts slow, speeds up
         */
        uint16_t erase_frame = cycle_frame - cfg->type_duration - TYPE_HOLD;
        fixed8_t t = (erase_frame * FIXED_ONE) / ERASE_DURATION;
        if (t > FIXED_ONE) t = FIXED_ONE;
        
        fixed8_t eased = ease_in_quad(t);
        
        /* eased 0→1 maps to num_chars→0 visible */
        chars_visible = cfg->num_chars - (eased * cfg->num_chars / FIXED_ONE);
        cursor_x = cfg->start_x + chars_visible * CHAR_CELL;
        show_cursor = 1;  /* Solid cursor during erase */
    }
    
    /* Render visible characters */
    int16_t x = cfg->start_x;
    for (uint8_t i = 0; i < chars_visible && i < cfg->num_chars; i++) {
        x += render_char(fb, cfg->text[i], x, 0);
    }
    
    /* Render cursor (full-height vertical line) */
    if (show_cursor && cursor_x < MATRIX_WIDTH) {
        fb->columns[cursor_x] = 0xFF;
    }
}
```

---

## Animation 4: Wave Text

### Concept

Text oscillates vertically with a sinusoidal wave traveling through it. Each character has a phase offset, creating a "snake" or "wave" effect.

```
       WAVE
      W  E         W   V         WA E
       AV   →    A  E   →      V
                 W              (continuous motion)
```

### Pseudocode

```c
/*
 * WAVE TEXT ANIMATION
 * 
 * Each character's Y position follows:
 *   y_offset = A * sin(ωt + φi)
 * 
 * Where:
 *   A = amplitude (2 pixels)
 *   ω = angular frequency (0.15 rad/frame)
 *   φi = phase offset per character (0.8 rad * index)
 * 
 * No phase transitions - continuous looping animation
 */

#define WAVE_AMPLITUDE   2
#define WAVE_SPEED       24   /* Higher = slower */
#define WAVE_PHASE_STEP  51   /* ~0.8 rad in fixed point (0.8 * 64) */

/* Sine lookup table: 64 entries for quarter wave */
static const int8_t sin_lut[65] = {
    0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
    49, 51, 54, 57, 60, 62, 65, 67, 70, 72, 75, 77, 79, 82, 84, 86,
    88, 90, 92, 94, 96, 97, 99, 101, 102, 104, 105, 107, 108, 109, 110, 112,
    113, 114, 115, 116, 116, 117, 118, 118, 119, 119, 120, 120, 120, 121, 121, 121, 121
};

/* Returns sin(angle) where angle is 0-255 (0-2π), result is -128 to 127 */
int8_t fast_sin(uint8_t angle) {
    uint8_t quadrant = angle >> 6;  /* 0-3 */
    uint8_t idx = angle & 0x3F;     /* 0-63 */
    
    int8_t value;
    switch (quadrant) {
        case 0: value = sin_lut[idx]; break;
        case 1: value = sin_lut[64 - idx]; break;
        case 2: value = -sin_lut[idx]; break;
        case 3: value = -sin_lut[64 - idx]; break;
    }
    return value;
}

typedef struct {
    const char* text;
    uint8_t num_chars;
    int16_t start_x;
} WaveTextConfig;

void wave_text_init(WaveTextConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->num_chars = strlen(text);
    cfg->start_x = (MATRIX_WIDTH - text_width(text)) / 2;
}

void wave_text_render(FrameBuffer* fb, WaveTextConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    for (uint8_t i = 0; i < cfg->num_chars; i++) {
        /*
         * Calculate wave angle for this character
         * 
         * angle = (frame * speed_factor) + (char_index * phase_step)
         * All in uint8_t so natural wraparound gives 0-2π range
         */
        uint8_t angle = (frame * 256 / WAVE_SPEED) + (i * WAVE_PHASE_STEP);
        
        /* Get sine value (-128 to 127) */
        int8_t sin_val = fast_sin(angle);
        
        /* Scale to amplitude: (-128..127) * 2 / 128 = -2..2 */
        int8_t y_offset = (sin_val * WAVE_AMPLITUDE) / 128;
        
        /* Round to nearest integer */
        y_offset = (y_offset + 64) >> 7;  /* Add 0.5 and truncate */
        
        /* Render character */
        int16_t x = cfg->start_x + i * CHAR_CELL;
        render_char(fb, cfg->text[i], x, y_offset);
    }
}
```

---

## Animation 5: Zoom Elastic

### Concept

Text zooms in from tiny to full size with an elastic "overshoot and spring" effect. Holds at full size, then zooms out with a "suck back" anticipation effect.

```
Frame 0:        Frame 15:       Frame 25:       Frame 80:
    .            BOOM!           BOOM            BOOM
   (tiny)     (overshoot)      (settled)       (pulling back)
```

### Pseudocode

```c
/*
 * ZOOM ELASTIC ANIMATION
 * 
 * Phases:
 *   1. ZOOM_IN: Scale 0→1 with easeOutElastic (overshoot + oscillate)
 *   2. HOLD: Full size
 *   3. ZOOM_OUT: Scale 1→0 with easeInBack (anticipation pullback)
 * 
 * Scaling is applied by sampling source text at transformed coordinates.
 * scale < 1: text appears smaller (pixels spread out in source space)
 * scale > 1: text appears larger (pixels compressed in source space)
 */

#define ZOOM_IN_DURATION   25
#define ZOOM_HOLD_DURATION 50
#define ZOOM_OUT_DURATION  20

typedef struct {
    const char* text;
    uint8_t bitmap[64];   /* Pre-rendered text bitmap */
    uint8_t bitmap_width;
    uint16_t cycle_duration;
} ZoomElasticConfig;

void zoom_elastic_init(ZoomElasticConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->cycle_duration = ZOOM_IN_DURATION + ZOOM_HOLD_DURATION + 
                          ZOOM_OUT_DURATION + 15;
    
    /* Pre-render text to bitmap */
    cfg->bitmap_width = text_width(text);
    memset(cfg->bitmap, 0, sizeof(cfg->bitmap));
    
    int16_t x = 0;
    for (const char* p = text; *p; p++) {
        const uint8_t* glyph = font_get_char(*p);
        for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
            if (x + col < 64) {
                cfg->bitmap[x + col] = pgm_read_byte(&glyph[col]);
            }
        }
        x += CHAR_CELL;
    }
}

void zoom_elastic_render(FrameBuffer* fb, ZoomElasticConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    fixed8_t scale;  /* Q8.8: 256 = 1.0 */
    
    if (cycle_frame < ZOOM_IN_DURATION) {
        /*
         * ZOOM IN with elastic effect
         * 
         * easeOutElastic: overshoots to ~1.3, oscillates, settles at 1.0
         * Creates "boing" effect
         */
        fixed8_t t = (cycle_frame * FIXED_ONE) / ZOOM_IN_DURATION;
        scale = ease_out_elastic(t);
    }
    else if (cycle_frame < ZOOM_IN_DURATION + ZOOM_HOLD_DURATION) {
        /*
         * HOLD at full size
         */
        scale = FIXED_ONE;
    }
    else {
        /*
         * ZOOM OUT with back effect
         * 
         * easeInBack: scale briefly increases (anticipation) before shrinking
         * Creates "suck in" effect
         */
        uint16_t out_frame = cycle_frame - ZOOM_IN_DURATION - ZOOM_HOLD_DURATION;
        fixed8_t t = (out_frame * FIXED_ONE) / ZOOM_OUT_DURATION;
        if (t > FIXED_ONE) t = FIXED_ONE;
        
        scale = FIXED_ONE - ease_in_back(t);
    }
    
    if (scale <= 0) return;  /* Invisible */
    
    /*
     * Render scaled text using inverse mapping
     * 
     * For each output pixel (x, y):
     *   1. Transform to centered coordinates
     *   2. Divide by scale to get source coordinates
     *   3. Sample source bitmap at that location
     */
    int16_t center_x = MATRIX_WIDTH / 2;
    int16_t center_y = MATRIX_HEIGHT / 2;
    int16_t src_center_x = cfg->bitmap_width / 2;
    int16_t src_center_y = MATRIX_HEIGHT / 2;
    
    for (int8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (int8_t y = 0; y < MATRIX_HEIGHT; y++) {
            /* Transform to source coordinates */
            int16_t dx = x - center_x;
            int16_t dy = y - center_y;
            
            /* Inverse scale: src = dst / scale */
            int16_t src_x = src_center_x + (dx * FIXED_ONE) / scale;
            int16_t src_y = src_center_y + (dy * FIXED_ONE) / scale;
            
            /* Sample source */
            if (src_x >= 0 && src_x < cfg->bitmap_width &&
                src_y >= 0 && src_y < MATRIX_HEIGHT) {
                if ((cfg->bitmap[src_x] >> src_y) & 1) {
                    fb_set_pixel(fb, x, y, 1);
                }
            }
        }
    }
}
```

---

## Animation 6: Slide Meet

### Concept

Text is split in half. Left half slides in from off-screen left, right half slides in from off-screen right. They meet in the middle with eased deceleration.

```
Frame 0:            Frame 15:           Frame 30:
   FU          NY      FU     NY          FUNY
(off-screen)    (sliding)            (met in center)
```

### Pseudocode

```c
/*
 * SLIDE MEET ANIMATION
 * 
 * Phases:
 *   1. SLIDE_IN: Both halves approach center with easeOutCubic
 *   2. HOLD: Text assembled in center
 *   3. SLIDE_OUT: Halves separate with easeInCubic
 * 
 * Left half: starts at x = -width, moves to final_x
 * Right half: starts at x = MATRIX_WIDTH, moves to final_x
 */

#define SLIDE_DURATION    30
#define SLIDE_HOLD        50
#define SLIDE_OUT_DURATION 25
#define SLIDE_DISTANCE    20  /* Pixels to travel */

typedef struct {
    const char* text;
    uint8_t mid_point;        /* Index where text splits */
    char left_text[16];
    char right_text[16];
    int16_t final_start_x;
    uint16_t cycle_duration;
} SlideMeetConfig;

void slide_meet_init(SlideMeetConfig* cfg, const char* text) {
    cfg->text = text;
    uint8_t len = strlen(text);
    cfg->mid_point = len / 2;
    
    /* Split text */
    strncpy(cfg->left_text, text, cfg->mid_point);
    cfg->left_text[cfg->mid_point] = '\0';
    strcpy(cfg->right_text, text + cfg->mid_point);
    
    cfg->final_start_x = (MATRIX_WIDTH - text_width(text)) / 2;
    cfg->cycle_duration = SLIDE_DURATION + SLIDE_HOLD + SLIDE_OUT_DURATION + 15;
}

void slide_meet_render(FrameBuffer* fb, SlideMeetConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    int16_t left_offset, right_offset;
    
    if (cycle_frame < SLIDE_DURATION) {
        /*
         * SLIDE IN
         * 
         * easeOutCubic: fast start, slow approach
         * Creates smooth deceleration as pieces meet
         */
        fixed8_t t = (cycle_frame * FIXED_ONE) / SLIDE_DURATION;
        fixed8_t eased = ease_out_cubic(t);
        
        /* Offsets decrease from SLIDE_DISTANCE to 0 */
        int16_t remaining = SLIDE_DISTANCE - (eased * SLIDE_DISTANCE / FIXED_ONE);
        left_offset = -remaining;
        right_offset = remaining;
    }
    else if (cycle_frame < SLIDE_DURATION + SLIDE_HOLD) {
        /*
         * HOLD together
         */
        left_offset = 0;
        right_offset = 0;
    }
    else {
        /*
         * SLIDE OUT
         * 
         * easeInCubic: slow start, fast departure
         * Creates smooth acceleration as pieces separate
         */
        uint16_t out_frame = cycle_frame - SLIDE_DURATION - SLIDE_HOLD;
        fixed8_t t = (out_frame * FIXED_ONE) / SLIDE_OUT_DURATION;
        if (t > FIXED_ONE) t = FIXED_ONE;
        
        fixed8_t eased = ease_in_cubic(t);
        int16_t travel = eased * SLIDE_DISTANCE / FIXED_ONE;
        left_offset = -travel;
        right_offset = travel;
    }
    
    /* Render left half */
    int16_t x = cfg->final_start_x + left_offset;
    for (const char* p = cfg->left_text; *p; p++) {
        x += render_char(fb, *p, x, 0);
    }
    
    /* Render right half */
    x = cfg->final_start_x + cfg->mid_point * CHAR_CELL + right_offset;
    for (const char* p = cfg->right_text; *p; p++) {
        x += render_char(fb, *p, x, 0);
    }
}
```

---

## Animation 7: Flip Board

### Concept

Like an airport departure board or split-flap display. Text flips vertically to transition between different words. The flip squashes vertically at the midpoint.

```
Frame 0:     Frame 6:      Frame 8:      Frame 12:
  COOL        ═══           ---           NICE
            (squashing)   (thin line)   (expanding)
```

### Pseudocode

```c
/*
 * FLIP BOARD ANIMATION
 * 
 * Cycles through array of texts with flip transition.
 * 
 * Flip mechanism:
 *   1. Current text squashes vertically (scaleY: 1 → 0)
 *   2. At midpoint, switch to next text
 *   3. New text expands vertically (scaleY: 0 → 1)
 * 
 * Y scaling implemented by sampling source at transformed Y coordinates.
 */

#define FLIP_DURATION  12
#define FLIP_HOLD      45
#define MAX_TEXTS      8

typedef struct {
    const char* texts[MAX_TEXTS];
    uint8_t num_texts;
    uint16_t cycle_duration;
} FlipBoardConfig;

void flip_board_init(FlipBoardConfig* cfg, const char** texts, uint8_t count) {
    cfg->num_texts = count < MAX_TEXTS ? count : MAX_TEXTS;
    for (uint8_t i = 0; i < cfg->num_texts; i++) {
        cfg->texts[i] = texts[i];
    }
    cfg->cycle_duration = cfg->num_texts * (FLIP_DURATION + FLIP_HOLD);
}

void flip_board_render(FrameBuffer* fb, FlipBoardConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    uint16_t segment_duration = FLIP_DURATION + FLIP_HOLD;
    
    /* Determine which text and frame within transition */
    uint8_t text_idx = cycle_frame / segment_duration;
    uint16_t text_frame = cycle_frame % segment_duration;
    
    const char* current_text = cfg->texts[text_idx];
    const char* next_text = cfg->texts[(text_idx + 1) % cfg->num_texts];
    
    const char* display_text;
    fixed8_t scale_y;
    
    if (text_frame < FLIP_DURATION / 2) {
        /*
         * FLIP OUT (squashing current text)
         * 
         * easeInQuad: accelerating squash
         * scaleY: 1.0 → 0.0
         */
        display_text = current_text;
        fixed8_t t = (text_frame * FIXED_ONE) / (FLIP_DURATION / 2);
        fixed8_t eased = ease_in_quad(t);
        scale_y = FIXED_ONE - eased;
    }
    else if (text_frame < FLIP_DURATION) {
        /*
         * FLIP IN (expanding new text)
         * 
         * easeOutQuad: decelerating expansion
         * scaleY: 0.0 → 1.0
         */
        display_text = next_text;
        uint16_t expand_frame = text_frame - FLIP_DURATION / 2;
        fixed8_t t = (expand_frame * FIXED_ONE) / (FLIP_DURATION / 2);
        scale_y = ease_out_quad(t);
    }
    else {
        /*
         * HOLD (showing new text at full size)
         */
        display_text = next_text;
        scale_y = FIXED_ONE;
    }
    
    /* Render with Y scaling */
    uint16_t text_w = text_width(display_text);
    int16_t start_x = (MATRIX_WIDTH - text_w) / 2;
    int16_t center_y = MATRIX_HEIGHT / 2;
    
    int16_t x = start_x;
    for (const char* p = display_text; *p; p++) {
        const uint8_t* glyph = font_get_char(*p);
        
        for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
            int16_t draw_x = x + col;
            if (draw_x < 0 || draw_x >= MATRIX_WIDTH) continue;
            
            uint8_t column_data = pgm_read_byte(&glyph[col]);
            
            /*
             * Apply Y scaling by inverse mapping
             * For each output row, find source row
             */
            for (int8_t y = 0; y < MATRIX_HEIGHT; y++) {
                /* Transform Y: src_y = center + (y - center) / scale */
                int16_t dy = y - center_y;
                int16_t src_y;
                
                if (scale_y < 16) {
                    /* Scale too small, skip */
                    continue;
                }
                
                src_y = center_y + (dy * FIXED_ONE) / scale_y;
                
                if (src_y >= 0 && src_y < MATRIX_HEIGHT) {
                    if ((column_data >> src_y) & 1) {
                        fb_set_pixel(fb, draw_x, y, 1);
                    }
                }
            }
        }
        x += CHAR_CELL;
    }
}
```

---

## Animation 8: Glitch

### Concept

Text displays normally most of the time, with periodic "glitch" bursts that add visual corruption: horizontal offsets, bit flips, and scanlines.

```
Normal:     Glitch:
 ERROR       ER█OR
            E ROR
             ════  (random scanline)
```

### Pseudocode

```c
/*
 * GLITCH ANIMATION
 * 
 * Two states:
 *   1. NORMAL: Clean text display
 *   2. GLITCH: Random visual artifacts for brief period
 * 
 * Glitch effects (applied per-column with probability):
 *   - X offset: shift column left/right by 1-2 pixels
 *   - Y offset: shift column up/down by 1 pixel
 *   - Bit corruption: XOR with random value
 *   - Scanlines: random horizontal lines across display
 * 
 * Uses LFSR for fast pseudo-random number generation.
 */

#define NORMAL_DURATION  40
#define GLITCH_DURATION  8

typedef struct {
    const char* text;
    uint8_t bitmap[64];
    uint8_t bitmap_width;
    int16_t start_x;
    uint16_t lfsr;  /* Linear feedback shift register for RNG */
    uint16_t cycle_duration;
} GlitchConfig;

/* Fast 16-bit LFSR pseudo-random generator */
uint16_t lfsr_next(uint16_t* lfsr) {
    uint16_t bit = ((*lfsr >> 0) ^ (*lfsr >> 2) ^ (*lfsr >> 3) ^ (*lfsr >> 5)) & 1;
    *lfsr = (*lfsr >> 1) | (bit << 15);
    return *lfsr;
}

void glitch_init(GlitchConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->lfsr = 0xACE1;  /* Non-zero seed */
    cfg->cycle_duration = NORMAL_DURATION + GLITCH_DURATION;
    
    /* Pre-render text */
    cfg->bitmap_width = text_width(text);
    cfg->start_x = (MATRIX_WIDTH - cfg->bitmap_width) / 2;
    memset(cfg->bitmap, 0, sizeof(cfg->bitmap));
    
    int16_t x = 0;
    for (const char* p = text; *p; p++) {
        const uint8_t* glyph = font_get_char(*p);
        for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
            if (x + col < 64) {
                cfg->bitmap[x + col] = pgm_read_byte(&glyph[col]);
            }
        }
        x += CHAR_CELL;
    }
}

void glitch_render(FrameBuffer* fb, GlitchConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    uint8_t is_glitching = (cycle_frame >= NORMAL_DURATION);
    
    for (uint8_t i = 0; i < cfg->bitmap_width; i++) {
        int16_t x = cfg->start_x + i;
        if (x < 0 || x >= MATRIX_WIDTH) continue;
        
        uint8_t column = cfg->bitmap[i];
        int8_t x_offset = 0;
        int8_t y_offset = 0;
        
        if (is_glitching) {
            uint16_t rnd = lfsr_next(&cfg->lfsr);
            
            /*
             * Apply random effects based on LFSR bits
             */
            
            /* 30% chance: X offset (-2 to +2) */
            if ((rnd & 0x0F) < 5) {
                x_offset = ((rnd >> 4) & 0x07) - 3;
                if (x_offset > 2) x_offset = 2;
                if (x_offset < -2) x_offset = -2;
            }
            
            /* 20% chance: Y offset (-1 to +1) */
            if ((rnd & 0xF0) < 0x30) {
                y_offset = ((rnd >> 8) & 0x03) - 1;
            }
            
            /* 10% chance: bit corruption */
            if ((rnd >> 12) < 2) {
                column ^= (uint8_t)(rnd >> 4);
            }
        }
        
        /* Apply offsets and render */
        int16_t draw_x = x + x_offset;
        if (draw_x < 0 || draw_x >= MATRIX_WIDTH) continue;
        
        /* Shift column data for Y offset */
        if (y_offset > 0) {
            column <<= y_offset;
        } else if (y_offset < 0) {
            column >>= (-y_offset);
        }
        
        fb->columns[draw_x] |= column;
    }
    
    /* Add random scanlines during glitch */
    if (is_glitching) {
        uint16_t rnd = lfsr_next(&cfg->lfsr);
        if ((rnd & 0x01) == 0) {
            uint8_t scan_y = (rnd >> 1) & 0x07;
            uint8_t scan_mask = 1 << scan_y;
            
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                if (lfsr_next(&cfg->lfsr) & 0x03) {  /* 75% fill */
                    fb->columns[x] |= scan_mask;
                }
            }
        }
    }
}
```

---

## Animation 9: Countdown

### Concept

Displays "5", "4", "3", "2", "1" followed by "GO!", with each number zooming in and fading. Numbers scale down as they fade.

```
Frame 0:    Frame 10:    Frame 20:    Frame 100:
   5           5            4           GO!
 (big)      (normal)      (big)       (pulsing)
```

### Pseudocode

```c
/*
 * COUNTDOWN ANIMATION
 * 
 * Sequence: 5 → 4 → 3 → 2 → 1 → GO!
 * 
 * Each number:
 *   - Starts large (scale ~3)
 *   - Shrinks to normal (scale 1) with easeOutCubic
 *   - Brief hold before next number
 * 
 * "GO!" pulses at end before cycle restarts.
 */

#define NUM_DURATION    20
#define NUM_COUNT       5
#define GO_DURATION     40

typedef struct {
    uint16_t cycle_duration;
} CountdownConfig;

void countdown_init(CountdownConfig* cfg) {
    cfg->cycle_duration = NUM_COUNT * NUM_DURATION + GO_DURATION;
}

void countdown_render(FrameBuffer* fb, CountdownConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    
    if (cycle_frame >= NUM_COUNT * NUM_DURATION) {
        /*
         * "GO!" phase
         * Simple display, could add pulse effect
         */
        const char* go_text = "GO!";
        uint16_t width = text_width(go_text);
        int16_t start_x = (MATRIX_WIDTH - width) / 2;
        
        int16_t x = start_x;
        for (const char* p = go_text; *p; p++) {
            x += render_char(fb, *p, x, 0);
        }
    }
    else {
        /*
         * Number countdown phase
         */
        uint8_t num_idx = cycle_frame / NUM_DURATION;
        uint16_t num_frame = cycle_frame % NUM_DURATION;
        
        /* Number to display: 5, 4, 3, 2, 1 */
        char num_char = '5' - num_idx;
        
        /*
         * Scale animation:
         * Starts at scale 3, ends at scale 1
         * Uses easeOutCubic for smooth deceleration
         */
        fixed8_t t = (num_frame * FIXED_ONE) / NUM_DURATION;
        fixed8_t eased = ease_out_cubic(t);
        
        /* Scale: 3 at t=0, 1 at t=1 */
        /* scale = 3 - 2*eased = 3 - 2*eased */
        fixed8_t scale = 3 * FIXED_ONE - 2 * eased;
        
        /* Get character bitmap */
        const uint8_t* glyph = font_get_char(num_char);
        
        /* Render scaled */
        int16_t center_x = MATRIX_WIDTH / 2;
        int16_t center_y = MATRIX_HEIGHT / 2;
        int16_t src_center_x = CHAR_WIDTH / 2;
        
        for (int8_t x = 0; x < MATRIX_WIDTH; x++) {
            for (int8_t y = 0; y < MATRIX_HEIGHT; y++) {
                int16_t dx = x - center_x;
                int16_t dy = y - center_y;
                
                int16_t src_x = src_center_x + (dx * FIXED_ONE) / scale;
                int16_t src_y = center_y + (dy * FIXED_ONE) / scale;
                
                if (src_x >= 0 && src_x < CHAR_WIDTH &&
                    src_y >= 0 && src_y < MATRIX_HEIGHT) {
                    uint8_t col_data = pgm_read_byte(&glyph[src_x]);
                    if ((col_data >> src_y) & 1) {
                        fb_set_pixel(fb, x, y, 1);
                    }
                }
            }
        }
    }
}
```

---

## Animation 10: Shutter Reveal

### Concept

Text is revealed row-by-row from top to bottom, like a vertical blind or shutter opening. Then closes from top to hide the text.

```
Frame 0:    Frame 10:    Frame 25:    Frame 80:
........    OOOO....     OPEN         OPE.....
........    OOOO....     OPEN         ........
........    ........     OPEN         ........
```

### Pseudocode

```c
/*
 * SHUTTER REVEAL ANIMATION
 * 
 * Phases:
 *   1. REVEAL: Rows become visible from top, easeOutCubic
 *   2. HOLD: Full text visible
 *   3. CLOSE: Rows hidden from top, easeInCubic
 * 
 * Implementation: For each column, only render rows < reveal_y
 */

#define REVEAL_DURATION  25
#define SHUTTER_HOLD     50
#define CLOSE_DURATION   20

typedef struct {
    const char* text;
    uint8_t bitmap[64];
    uint8_t bitmap_width;
    int16_t start_x;
    uint16_t cycle_duration;
} ShutterConfig;

void shutter_init(ShutterConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->bitmap_width = text_width(text);
    cfg->start_x = (MATRIX_WIDTH - cfg->bitmap_width) / 2;
    cfg->cycle_duration = REVEAL_DURATION + SHUTTER_HOLD + CLOSE_DURATION + 10;
    
    /* Pre-render text */
    memset(cfg->bitmap, 0, sizeof(cfg->bitmap));
    int16_t x = 0;
    for (const char* p = text; *p; p++) {
        const uint8_t* glyph = font_get_char(*p);
        for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
            if (x + col < 64) {
                cfg->bitmap[x + col] = pgm_read_byte(&glyph[col]);
            }
        }
        x += CHAR_CELL;
    }
}

void shutter_render(FrameBuffer* fb, ShutterConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    uint8_t reveal_y;  /* Rows 0 to reveal_y-1 are visible */
    
    if (cycle_frame < REVEAL_DURATION) {
        /*
         * REVEAL: easeOutCubic for smooth deceleration
         * reveal_y: 0 → 8
         */
        fixed8_t t = (cycle_frame * FIXED_ONE) / REVEAL_DURATION;
        fixed8_t eased = ease_out_cubic(t);
        reveal_y = (eased * MATRIX_HEIGHT) / FIXED_ONE;
    }
    else if (cycle_frame < REVEAL_DURATION + SHUTTER_HOLD) {
        /*
         * HOLD: fully visible
         */
        reveal_y = MATRIX_HEIGHT;
    }
    else {
        /*
         * CLOSE: easeInCubic for smooth acceleration
         * reveal_y: 8 → 0
         */
        uint16_t close_frame = cycle_frame - REVEAL_DURATION - SHUTTER_HOLD;
        fixed8_t t = (close_frame * FIXED_ONE) / CLOSE_DURATION;
        if (t > FIXED_ONE) t = FIXED_ONE;
        
        fixed8_t eased = ease_in_cubic(t);
        reveal_y = MATRIX_HEIGHT - (eased * MATRIX_HEIGHT) / FIXED_ONE;
    }
    
    /*
     * Render with row clipping
     * Create mask: bits 0 to (reveal_y-1) set
     */
    uint8_t row_mask = (1 << reveal_y) - 1;
    
    for (uint8_t i = 0; i < cfg->bitmap_width; i++) {
        int16_t x = cfg->start_x + i;
        if (x < 0 || x >= MATRIX_WIDTH) continue;
        
        fb->columns[x] = cfg->bitmap[i] & row_mask;
    }
}
```

---

## Animation 11: Column Reveal

### Concept

Each column of text reveals independently with staggered timing, creating a wave effect. Uses easeOutBack for a slight overshoot at the end of each column's reveal.

```
Frame 0:    Frame 10:    Frame 20:    Frame 40:
........    H.......     HEL.....     HELLO
........    H.......     HEL.....     HELLO
........    H.......     HEL.....     HELLO
            (columns reveal left to right with delay)
```

### Pseudocode

```c
/*
 * COLUMN REVEAL ANIMATION
 * 
 * Each column has:
 *   - Start delay: column_index * COLUMN_DELAY
 *   - Reveal: easeOutBack (slight overshoot on height)
 *   - Exit: reverse wave with easeInCubic
 * 
 * Creates cascading wipe effect across display.
 */

#define COLUMN_REVEAL_DURATION  40
#define COLUMN_HOLD             45
#define COLUMN_EXIT_DURATION    35
#define COLUMN_DELAY            0.8  /* Frames delay per column */

typedef struct {
    const char* text;
    uint8_t bitmap[64];
    uint8_t bitmap_width;
    int16_t start_x;
    uint16_t cycle_duration;
} ColumnRevealConfig;

void column_reveal_init(ColumnRevealConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->bitmap_width = text_width(text);
    cfg->start_x = (MATRIX_WIDTH - cfg->bitmap_width) / 2;
    cfg->cycle_duration = COLUMN_REVEAL_DURATION + COLUMN_HOLD + 
                          COLUMN_EXIT_DURATION + 15;
    
    /* Pre-render */
    memset(cfg->bitmap, 0, sizeof(cfg->bitmap));
    int16_t x = 0;
    for (const char* p = text; *p; p++) {
        const uint8_t* glyph = font_get_char(*p);
        for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
            if (x + col < 64) {
                cfg->bitmap[x + col] = pgm_read_byte(&glyph[col]);
            }
        }
        x += CHAR_CELL;
    }
}

void column_reveal_render(FrameBuffer* fb, ColumnRevealConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    
    for (uint8_t i = 0; i < cfg->bitmap_width; i++) {
        int16_t x = cfg->start_x + i;
        if (x < 0 || x >= MATRIX_WIDTH) continue;
        
        /*
         * Calculate this column's progress
         * Each column starts slightly after the previous
         */
        
        /* Column delay in fixed point (0.8 frames per column) */
        uint16_t col_delay = (i * 205) >> 8;  /* 0.8 * 256 ≈ 205 */
        
        fixed8_t col_progress;
        
        if (cycle_frame < COLUMN_REVEAL_DURATION) {
            /*
             * REVEAL phase with per-column delay
             */
            int16_t col_frame = (int16_t)cycle_frame - col_delay;
            if (col_frame < 0) {
                col_progress = 0;
            } else {
                uint16_t effective_duration = COLUMN_REVEAL_DURATION - col_delay;
                if (effective_duration == 0) effective_duration = 1;
                
                fixed8_t t = (col_frame * FIXED_ONE) / effective_duration;
                if (t > FIXED_ONE) t = FIXED_ONE;
                
                col_progress = ease_out_back(t);
            }
        }
        else if (cycle_frame < COLUMN_REVEAL_DURATION + COLUMN_HOLD) {
            /*
             * HOLD phase
             */
            col_progress = FIXED_ONE;
        }
        else {
            /*
             * EXIT phase - reverse wave (right columns exit first)
             */
            uint16_t exit_frame = cycle_frame - COLUMN_REVEAL_DURATION - COLUMN_HOLD;
            
            /* Reverse delay: rightmost columns exit first */
            uint16_t rev_col_delay = ((cfg->bitmap_width - 1 - i) * 154) >> 8;  /* 0.6 */
            
            int16_t col_exit_frame = (int16_t)exit_frame - rev_col_delay;
            if (col_exit_frame < 0) {
                col_progress = FIXED_ONE;
            } else {
                uint16_t effective_duration = COLUMN_EXIT_DURATION - rev_col_delay;
                if (effective_duration == 0) effective_duration = 1;
                
                fixed8_t t = (col_exit_frame * FIXED_ONE) / effective_duration;
                if (t > FIXED_ONE) t = FIXED_ONE;
                
                col_progress = FIXED_ONE - ease_in_cubic(t);
            }
        }
        
        /*
         * Apply progress as vertical mask
         * col_progress 0-256 maps to 0-8 visible rows
         */
        uint8_t visible_rows = (col_progress * MATRIX_HEIGHT) / FIXED_ONE;
        if (visible_rows > MATRIX_HEIGHT) visible_rows = MATRIX_HEIGHT;
        
        uint8_t row_mask = (1 << visible_rows) - 1;
        fb->columns[x] = cfg->bitmap[i] & row_mask;
    }
}
```

---

## Animation 12: Spin Letters

### Concept

Each letter spins around its vertical axis (like a card flipping). Implemented as horizontal scaling that goes 1 → 0 → -1 → 0 → 1 (with negative scale showing "back" of card, but since text is symmetrical, we just show squashed).

```
Frame 0:    Frame 8:      Frame 12:     Frame 20:
  SPIN       S|IN          S IN          SPIN
           (P squashed)  (P thin)     (all settled)
```

### Pseudocode

```c
/*
 * SPIN LETTERS ANIMATION
 * 
 * Each letter rotates around Y axis:
 *   - scaleX follows cos(θ) where θ animates 0 → 2π → 0
 *   - Creates card-flip appearance
 * 
 * Staggered timing: each letter starts CHAR_DELAY frames after previous.
 * Exit: all letters spin out simultaneously.
 */

#define SPIN_DURATION      20
#define SPIN_HOLD          40
#define SPIN_OUT_DURATION  15
#define SPIN_CHAR_DELAY    6

typedef struct {
    const char* text;
    uint8_t num_chars;
    int16_t start_x;
    uint16_t cycle_duration;
} SpinLettersConfig;

/* Cosine approximation using sine LUT */
int8_t fast_cos(uint8_t angle) {
    return fast_sin(angle + 64);  /* cos(x) = sin(x + π/2) */
}

void spin_letters_init(SpinLettersConfig* cfg, const char* text) {
    cfg->text = text;
    cfg->num_chars = strlen(text);
    cfg->start_x = (MATRIX_WIDTH - text_width(text)) / 2;
    cfg->cycle_duration = cfg->num_chars * SPIN_CHAR_DELAY + 
                          SPIN_DURATION + SPIN_HOLD + SPIN_OUT_DURATION + 20;
}

void spin_letters_render(FrameBuffer* fb, SpinLettersConfig* cfg, uint16_t frame) {
    fb_clear(fb);
    
    uint16_t cycle_frame = frame % cfg->cycle_duration;
    uint16_t exit_start = cfg->num_chars * SPIN_CHAR_DELAY + SPIN_DURATION + SPIN_HOLD;
    
    for (uint8_t i = 0; i < cfg->num_chars; i++) {
        uint16_t char_start = i * SPIN_CHAR_DELAY;
        
        if (cycle_frame < char_start) continue;
        
        int16_t char_frame = cycle_frame - char_start;
        fixed8_t scale_x;
        
        if (cycle_frame >= exit_start) {
            /*
             * EXIT: All letters spin out
             * 
             * Scale follows cos curve from 1 to 0
             */
            uint16_t exit_frame = cycle_frame - exit_start;
            fixed8_t t = (exit_frame * FIXED_ONE) / SPIN_OUT_DURATION;
            if (t > FIXED_ONE) t = FIXED_ONE;
            
            /* cos(t * π/2): starts at 1, ends at 0 */
            uint8_t angle = (ease_in_quad(t) * 64) / FIXED_ONE;  /* 0-64 = 0-π/2 */
            int8_t cos_val = fast_cos(angle);
            scale_x = (cos_val * FIXED_ONE) / 128;
        }
        else if (char_frame < SPIN_DURATION) {
            /*
             * SPIN IN: Each letter spins through 2 full rotations
             * 
             * cos(θ) where θ goes from 2π to 0
             * This creates the "front-back-front" card flip effect
             */
            fixed8_t t = (char_frame * FIXED_ONE) / SPIN_DURATION;
            fixed8_t eased_t = ease_out_cubic(t);
            
            /* Angle: starts at 512 (2 rotations), ends at 0 */
            /* We use (1-eased) * 2 * 256 for 2 full rotations */
            uint16_t angle = ((FIXED_ONE - eased_t) * 512) / FIXED_ONE;
            int8_t cos_val = fast_cos((uint8_t)(angle & 0xFF));
            scale_x = (cos_val * FIXED_ONE) / 128;
            
            /* Take absolute value (we don't show "back" differently) */
            if (scale_x < 0) scale_x = -scale_x;
        }
        else {
            /*
             * HOLD: Full size
             */
            scale_x = FIXED_ONE;
        }
        
        /* Skip if too thin to render */
        if (scale_x < 32) continue;  /* Less than 12.5% width */
        
        /*
         * Render horizontally scaled character
         * 
         * For each output column, find corresponding source column
         * Character center is at start_x + i*6 + 2.5
         */
        const uint8_t* glyph = font_get_char(cfg->text[i]);
        int16_t char_center_x = cfg->start_x + i * CHAR_CELL + CHAR_WIDTH / 2;
        
        for (int8_t col = 0; col < CHAR_WIDTH; col++) {
            /* Local X: -2.5 to +2.5 (we use fixed point) */
            int16_t local_x = (col * FIXED_ONE) - (CHAR_WIDTH * FIXED_ONE / 2);
            
            /* Scale local X */
            int16_t scaled_x = (local_x * scale_x) / FIXED_ONE;
            
            /* Convert back to screen X */
            int16_t screen_x = char_center_x + (scaled_x / FIXED_ONE);
            
            if (screen_x >= 0 && screen_x < MATRIX_WIDTH) {
                fb->columns[screen_x] |= pgm_read_byte(&glyph[col]);
            }
        }
    }
}
```

---

## Main Loop Example

```c
/*
 * MAIN APPLICATION LOOP
 * 
 * Typical embedded implementation with timer interrupt
 */

/* Currently active animation */
static uint8_t current_animation = 0;
static uint16_t frame_counter = 0;

/* Animation configurations */
static SmoothScrollConfig scroll_cfg;
static DropBounceConfig drop_cfg;
static TypewriterConfig type_cfg;
/* ... etc ... */

/* Frame buffer */
static FrameBuffer display_buffer;

void setup(void) {
    /* Initialize hardware */
    led_matrix_init();
    timer_init(FRAME_MS);
    
    /* Initialize animations */
    smooth_scroll_init(&scroll_cfg, "HELLO WORLD");
    drop_bounce_init(&drop_cfg, "FUNKY");
    typewriter_init(&type_cfg, "HELLO");
    /* ... etc ... */
}

/* Called by timer interrupt every FRAME_MS milliseconds */
void frame_tick(void) {
    /* Render current animation */
    switch (current_animation) {
        case 0:
            smooth_scroll_render(&display_buffer, &scroll_cfg, frame_counter);
            break;
        case 1:
            drop_bounce_render(&display_buffer, &drop_cfg, frame_counter);
            break;
        case 2:
            typewriter_render(&display_buffer, &type_cfg, frame_counter);
            break;
        /* ... etc ... */
    }
    
    /* Output to hardware */
    led_matrix_update(display_buffer.columns);
    
    frame_counter++;
}

void switch_animation(uint8_t index) {
    current_animation = index;
    frame_counter = 0;
}

int main(void) {
    setup();
    
    while (1) {
        /* Main loop can handle button input, serial commands, etc. */
        if (button_pressed()) {
            switch_animation((current_animation + 1) % NUM_ANIMATIONS);
        }
        
        /* Low-power wait for next interrupt */
        sleep_until_interrupt();
    }
}
```

---

## Memory Optimization Notes

```c
/*
 * MEMORY OPTIMIZATION STRATEGIES
 * 
 * For constrained embedded systems (e.g., ATmega328 with 2KB RAM):
 * 
 * 1. Store font in PROGMEM (Flash)
 *    - Use pgm_read_byte() to access
 *    - Saves ~200 bytes RAM
 * 
 * 2. Reuse frame buffer
 *    - Single 32-byte buffer, cleared each frame
 *    - Don't double-buffer unless needed for flicker-free
 * 
 * 3. Use fixed-point instead of float
 *    - 16-bit fixed point for easing functions
 *    - Integer sine/cosine LUTs
 * 
 * 4. Limit text length
 *    - Pre-calculate bitmap only for visible portion
 *    - Stream characters for scrolling
 * 
 * 5. Easing LUTs
 *    - Store pre-computed easing curves (64-256 entries)
 *    - Trade 256 bytes Flash for faster runtime
 */
```

---

## License

This pseudocode is provided for educational purposes. Adapt for your specific microcontroller and LED matrix hardware.
