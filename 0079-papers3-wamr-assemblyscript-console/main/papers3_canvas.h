#pragma once

#include <cstdint>

namespace papers3_wasm {

void InitializePaperCanvas();

int32_t PaperCanvasWidth();

int32_t PaperCanvasHeight();

void PaperCanvasResetFrame();

void PaperCanvasScreenClear(uint32_t color);

void PaperCanvasDrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void PaperCanvasFillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void PaperCanvasPresent(int32_t mode);

}  // namespace papers3_wasm
