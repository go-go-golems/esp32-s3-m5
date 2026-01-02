#include "wizard.h"

#include <algorithm>
#include <sstream>

static constexpr int64_t kStableHoldUs = 400 * 1000;

static bool contains_keynum(const std::vector<uint8_t> &v, uint8_t n) {
    for (auto x : v) {
        if (x == n) return true;
    }
    return false;
}

static bool edge_pressed(const std::vector<uint8_t> &now, const std::vector<uint8_t> &prev, uint8_t keynum) {
    return contains_keynum(now, keynum) && !contains_keynum(prev, keynum);
}

static std::string join_nums(const std::vector<uint8_t> &nums) {
    std::ostringstream ss;
    for (size_t i = 0; i < nums.size(); i++) {
        if (i) ss << ",";
        ss << (unsigned)nums[i];
    }
    return ss.str();
}

void CalibrationWizard::reset() {
    phase_ = WizardPhase::WaitingForPress;
    index_ = 0;
    stable_candidate_.clear();
    stable_since_us_ = 0;
    printed_config_ = false;

    bindings_.clear();
    bindings_.push_back({"NavUp", "Press: Up (Fn+?)", {}, false});
    bindings_.push_back({"NavDown", "Press: Down (Fn+?)", {}, false});
    bindings_.push_back({"NavLeft", "Press: Left (Fn+?)", {}, false});
    bindings_.push_back({"NavRight", "Press: Right (Fn+?)", {}, false});
    bindings_.push_back({"Back", "Press: Back/Esc", {}, false});
    bindings_.push_back({"Enter", "Press: Enter", {}, false});
    bindings_.push_back({"Tab", "Press: Tab", {}, false});
    bindings_.push_back({"Space", "Press: Space", {}, false});
}

const WizardBinding *CalibrationWizard::current() const {
    if (index_ < 0 || index_ >= (int)bindings_.size()) {
        return nullptr;
    }
    return &bindings_[(size_t)index_];
}

bool CalibrationWizard::update(const cardputer_kb::ScanSnapshot &scan, const cardputer_kb::ScanSnapshot &prev_scan,
                               int64_t now_us) {
    // Control keys are physical positions, so we can use keyNum constants.
    // Enter=42, Del(backspace)=14, Tab=15.
    static constexpr uint8_t kAcceptKey = 42;
    static constexpr uint8_t kRedoKey = 14;
    static constexpr uint8_t kSkipKey = 15;

    bool changed = false;

    if (phase_ == WizardPhase::Done) {
        // Print config once when user presses Enter.
        if (!printed_config_ && edge_pressed(scan.pressed_keynums, prev_scan.pressed_keynums, kAcceptKey)) {
            printed_config_ = true;
            changed = true;
        }
        return changed;
    }

    if (index_ >= (int)bindings_.size()) {
        phase_ = WizardPhase::Done;
        changed = true;
        return changed;
    }

    if (phase_ == WizardPhase::CapturedAwaitConfirm) {
        if (edge_pressed(scan.pressed_keynums, prev_scan.pressed_keynums, kRedoKey)) {
            stable_candidate_.clear();
            stable_since_us_ = 0;
            bindings_[(size_t)index_].required_keynums.clear();
            bindings_[(size_t)index_].captured = false;
            phase_ = WizardPhase::WaitingForPress;
            return true;
        }
        if (edge_pressed(scan.pressed_keynums, prev_scan.pressed_keynums, kSkipKey)) {
            index_++;
            stable_candidate_.clear();
            stable_since_us_ = 0;
            phase_ = WizardPhase::WaitingForPress;
            return true;
        }
        if (edge_pressed(scan.pressed_keynums, prev_scan.pressed_keynums, kAcceptKey)) {
            index_++;
            stable_candidate_.clear();
            stable_since_us_ = 0;
            phase_ = WizardPhase::WaitingForPress;
            return true;
        }
        return false;
    }

    if (scan.pressed_keynums.empty()) {
        if (!stable_candidate_.empty() || phase_ != WizardPhase::WaitingForPress) {
            stable_candidate_.clear();
            stable_since_us_ = 0;
            phase_ = WizardPhase::WaitingForPress;
            return true;
        }
        return false;
    }

    if (phase_ == WizardPhase::WaitingForPress) {
        stable_candidate_ = scan.pressed_keynums;
        stable_since_us_ = now_us;
        phase_ = WizardPhase::Stabilizing;
        return true;
    }

    // Stabilizing
    if (scan.pressed_keynums != stable_candidate_) {
        stable_candidate_ = scan.pressed_keynums;
        stable_since_us_ = now_us;
        return true;
    }

    if (now_us - stable_since_us_ >= kStableHoldUs) {
        bindings_[(size_t)index_].required_keynums = stable_candidate_;
        bindings_[(size_t)index_].captured = true;
        phase_ = WizardPhase::CapturedAwaitConfirm;
        return true;
    }

    return changed;
}

std::string CalibrationWizard::config_json() const {
    std::ostringstream ss;
    ss << "{\\n";
    ss << "  \\\"device\\\": \\\"M5Cardputer\\\",\\n";
    ss << "  \\\"matrix\\\": {\\\"rows\\\": 4, \\\"cols\\\": 14},\\n";
    ss << "  \\\"bindings\\\": [\\n";

    bool first = true;
    for (const auto &b : bindings_) {
        if (!b.captured) continue;
        if (!first) ss << ",\\n";
        first = false;
        ss << "    {\\\"name\\\":\\\"" << b.name << "\\\", \\\"required_keynums\\\":[" << join_nums(b.required_keynums)
           << "]}";
    }
    ss << "\\n  ]\\n";
    ss << "}\\n";
    return ss.str();
}

