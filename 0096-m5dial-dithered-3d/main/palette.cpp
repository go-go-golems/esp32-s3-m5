#include "palette.h"

// RGB565 palette definitions
// Computed from hex colors in m5dial.jsx
// #ff2940 → RGB565: R=31,G=2,B=4 → 0xF804
// #3050ff → RGB565: R=6,G=40,B=31 → 0x185F (approx, verifying: 0x30=48→48/8=6, 0x50=80→80/4=20, 0xff=255→255/8=31 → 6<<11|20<<5|31 = 0xC41F... let me recalculate properly)

// Proper RGB565: R=5bit, G=6bit, B=5bit
// #ff2940: R=255→31, G=41→2, B=64→4  → (31<<11)|(2<<5)|4 = 0xF804
// #3050ff: R=48→6,  G=80→20, B=255→31 → (6<<11)|(20<<5)|31 = 0xC41F
//   Wait, let me be more careful:
//   R=0x30=48 → 48*31/255 = 5.8 → 6
//   G=0x50=80 → 80*63/255 = 19.8 → 20
//   B=0xFF=255 → 31
//   = (6<<11)|(20<<5)|31 = 0x3020|0x0280|0x001F = 0x32BF
//   Hmm, that doesn't match my earlier calculation. Let me just use the standard formula:
//   RGB565 = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3)
//   #ff2940: R=0xFF→0xF8<<8=0xF800, G=0x29=41→0x28<<3=0x0140, B=0x40=64>>3=8
//   = 0xF800|0x0140|0x0008 = 0xF948
//   Hmm. Let me just be precise:
//   #ff2940: R=255, G=41, B=64
//     R5 = 255*31/255 = 31 = 0x1F
//     G6 = 41*63/255 = 10.1 ≈ 10 = 0x0A
//     B5 = 64*31/255 = 7.8 ≈ 8 = 0x08
//     RGB565 = (31<<11)|(10<<5)|8 = 0xF800|0x0140|0x0008 = 0xF948

// Actually let me just compute them properly once:
// RGB565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
// #ff2940: r=0xFF g=0x29 b=0x40
//   r>>3=31, g>>2=10, b>>3=8  → (31<<11)|(10<<5)|8 = 0xF800+0x0140+0x0008 = 0xF948
// #3050ff: r=0x30 g=0x50 b=0xFF
//   r>>3=6, g>>2=20, b>>3=31 → (6<<11)|(20<<5)|31 = 0x3000+0x0280+0x001F = 0x329F
// #ffffff: 0xFFFF
// #7a1020: r=0x7A g=0x10 b=0x20 → r>>3=15, g>>2=4, b>>3=4 → (15<<11)|(4<<5)|4 = 0x7800+0x0080+0x0004 = 0x7884
// #5a78ff: r=0x5A g=0x78 b=0xFF → r>>3=11, g>>2=30, b>>3=31 → (11<<11)|(30<<5)|31 = 0x5800+0x03C0+0x001F = 0x5BDF
// #ffae20: r=0xFF g=0xAE b=0x20 → r>>3=31, g>>2=43, b>>3=4 → (31<<11)|(43<<5)|4 = 0xF800+0x0560+0x0004 = 0xFD64
// #5a3010: r=0x5A g=0x30 b=0x10 → r>>3=11, g>>2=12, b>>3=2 → (11<<11)|(12<<5)|2 = 0x5800+0x0180+0x0002 = 0x5982
// #ffe080: r=0xFF g=0xE0 b=0x80 → r>>3=31, g>>2=56, b>>3=16 → (31<<11)|(56<<5)|16 = 0xF800+0x0700+0x0010 = 0xFF10

static const palette_t s_palettes[PALETTE_COUNT] = {
    // CLASSIC
    {{
        0x0000,  // black
        0xF948,  // warm  #ff2940
        0x329F,  // cool  #3050ff
        0xFFFF,  // high  #ffffff
    }, "CLASSIC"},
    // INVERTED (warm ↔ cool swapped)
    {{
        0x0000,  // black
        0x329F,  // warm  #3050ff (was cool)
        0xF948,  // cool  #ff2940 (was warm)
        0xFFFF,  // high  #ffffff
    }, "INVERTED"},
    // RED MONO
    {{
        0x0000,  // black
        0xF948,  // warm  #ff2940
        0x7884,  // cool  #7a1020
        0xFFFF,  // high  #ffffff
    }, "RED MONO"},
    // BLUE MONO
    {{
        0x0000,  // black
        0x5BDF,  // warm  #5a78ff
        0x329F,  // cool  #3050ff
        0xFFFF,  // high  #ffffff
    }, "BLUE MONO"},
    // AMBER CRT
    {{
        0x0000,  // black
        0xFD64,  // warm  #ffae20
        0x5982,  // cool  #5a3010
        0xFF10,  // high  #ffe080
    }, "AMBER CRT"},
};

static PaletteId s_current = PALETTE_CLASSIC;

const palette_t* palette_get(PaletteId id) {
    if (id < 0 || id >= PALETTE_COUNT) id = PALETTE_CLASSIC;
    return &s_palettes[id];
}

const palette_t* palette_current(void) {
    return &s_palettes[s_current];
}

void palette_set(PaletteId id) {
    if (id >= 0 && id < PALETTE_COUNT) {
        s_current = id;
    }
}

void palette_cycle_next(void) {
    s_current = (PaletteId)((s_current + 1) % PALETTE_COUNT);
}

PaletteId palette_current_id(void) {
    return s_current;
}
