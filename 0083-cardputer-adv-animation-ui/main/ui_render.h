#pragma once

#include <M5Unified.hpp>

#include "ui_kb.h"
#include "ui_model.h"

void ui_render_frame(M5Canvas *canvas, const UiState *ui, const ui_kb_debug_state_t *kb_dbg);
