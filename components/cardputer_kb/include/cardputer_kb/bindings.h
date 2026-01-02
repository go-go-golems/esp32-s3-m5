#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

namespace cardputer_kb {

enum class Action : uint8_t {
    NavUp,
    NavDown,
    NavLeft,
    NavRight,
    Back,
    Del,
    Enter,
    Tab,
    Space,
};

struct Binding {
    Action action;
    uint8_t n;
    uint8_t keynums[4];
};

inline const char *action_name(Action a) {
    switch (a) {
    case Action::NavUp: return "NavUp";
    case Action::NavDown: return "NavDown";
    case Action::NavLeft: return "NavLeft";
    case Action::NavRight: return "NavRight";
    case Action::Back: return "Back";
    case Action::Del: return "Del";
    case Action::Enter: return "Enter";
    case Action::Tab: return "Tab";
    case Action::Space: return "Space";
    }
    return "Unknown";
}

inline bool pressed_contains_all(const std::vector<uint8_t> &pressed_keynums, const Binding &b) {
    for (uint8_t i = 0; i < b.n; i++) {
        bool found = false;
        for (auto kn : pressed_keynums) {
            if (kn == b.keynums[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

// Returns the most-specific matching binding (largest chord wins) or nullptr.
inline const Binding *decode_best(const std::vector<uint8_t> &pressed_keynums, const Binding *bindings,
                                  size_t bindings_count) {
    const Binding *best = nullptr;
    for (size_t i = 0; i < bindings_count; i++) {
        const Binding &b = bindings[i];
        if (!pressed_contains_all(pressed_keynums, b)) {
            continue;
        }
        if (best == nullptr || b.n > best->n) {
            best = &b;
        }
    }
    return best;
}

} // namespace cardputer_kb
