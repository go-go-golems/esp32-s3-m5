#pragma once

#include <stdint.h>

#include "M5GFX.h"

struct PlasmaState {
    uint8_t sin8[256] = {};
    uint16_t palette[256] = {};
    uint32_t t = 0;
    bool inited = false;
};

void plasma_init(PlasmaState &s);
void plasma_render(M5Canvas &body, PlasmaState &s);
