#pragma once

// Generated from 0023 wizard capture (Cardputer scancode calibrator).
//
// IMPORTANT: This is a *semantic binding* layer (actions -> physical chords).
// It may vary across firmware UX expectations and/or keyboard variants.

#include "cardputer_kb/bindings.h"

namespace cardputer_kb {

// Captured bindings:
// - NavUp    = keynum 40   (";" physical key)
// - NavDown  = keynum 54   ("." physical key)
// - NavLeft  = keynum 53   ("," physical key)
// - NavRight = keynum 55   ("/" physical key)
// - Back     = keynum 1    ("`" physical key)
// - Enter    = keynum 42
// - Tab      = keynum 15
// - Space    = keynum 56
static constexpr Binding kCapturedBindingsM5Cardputer[] = {
    {Action::NavUp, 1, {40, 0, 0, 0}},
    {Action::NavDown, 1, {54, 0, 0, 0}},
    {Action::NavLeft, 1, {53, 0, 0, 0}},
    {Action::NavRight, 1, {55, 0, 0, 0}},
    {Action::Back, 1, {1, 0, 0, 0}},
    {Action::Enter, 1, {42, 0, 0, 0}},
    {Action::Tab, 1, {15, 0, 0, 0}},
    {Action::Space, 1, {56, 0, 0, 0}},
};

} // namespace cardputer_kb

