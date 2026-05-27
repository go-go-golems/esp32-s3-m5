#pragma once

#include <stdint.h>

// 4-color palette definition (RGB565)
typedef struct {
    uint16_t colors[4];  // [0]=black, [1]=warm, [2]=cool, [3]=high
    const char* name;
} palette_t;

// Palette enumeration
enum PaletteId {
    PALETTE_CLASSIC = 0,
    PALETTE_INVERTED,
    PALETTE_RED,
    PALETTE_BLUE,
    PALETTE_AMBER,
    PALETTE_COUNT
};

// Get palette by ID
const palette_t* palette_get(PaletteId id);

// Get current active palette
const palette_t* palette_current(void);

// Set active palette
void palette_set(PaletteId id);

// Cycle to next palette
void palette_cycle_next(void);

// Get current palette ID
PaletteId palette_current_id(void);
