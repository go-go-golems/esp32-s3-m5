#pragma once

// Generated from 0023 wizard capture (Cardputer scancode calibrator).
//
// IMPORTANT: This is a *semantic binding* layer (actions -> physical chords).
// It may vary across firmware UX expectations and/or keyboard variants.

#include "cardputer_kb/bindings.h"

namespace cardputer_kb {

// Captured bindings:
// - NavUp    = fn(29) + keynum 40   (";")
// - NavDown  = fn(29) + keynum 54   (".")
// - NavLeft  = fn(29) + keynum 53   (",")
// - NavRight = fn(29) + keynum 55   ("/")
// - Back     = fn(29) + keynum 1    ("`" physical key; used as Esc/back)
// - Del      = fn(29) + keynum 14   ("del" physical key)
// - Enter    = keynum 42
// - Tab      = keynum 15
// - Space    = keynum 56
static constexpr Binding kCapturedBindingsM5Cardputer[] = {
    {Action::NavUp, 2, {29, 40, 0, 0}},
    {Action::NavDown, 2, {29, 54, 0, 0}},
    {Action::NavLeft, 2, {29, 53, 0, 0}},
    {Action::NavRight, 2, {29, 55, 0, 0}},
    {Action::Back, 2, {29, 1, 0, 0}},
    {Action::Del, 2, {29, 14, 0, 0}},
    {Action::Enter, 1, {42, 0, 0, 0}},
    {Action::Tab, 1, {15, 0, 0, 0}},
    {Action::Space, 1, {56, 0, 0, 0}},
};

} // namespace cardputer_kb
