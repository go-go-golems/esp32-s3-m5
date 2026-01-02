#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "cardputer_kb/scanner.h"

enum class WizardPhase {
    WaitingForPress,
    Stabilizing,
    CapturedAwaitConfirm,
    Done,
};

struct WizardBinding {
    std::string name;
    std::string prompt;
    std::vector<uint8_t> required_keynums;
    bool captured = false;
};

class CalibrationWizard {
  public:
    void reset();

    // Returns true if the wizard advanced state or updated capture.
    bool update(const cardputer_kb::ScanSnapshot &scan, const cardputer_kb::ScanSnapshot &prev_scan, int64_t now_us);

    WizardPhase phase() const { return phase_; }
    int index() const { return index_; }
    const std::vector<WizardBinding> &bindings() const { return bindings_; }
    const WizardBinding *current() const;

    bool printed_config() const { return printed_config_; }
    std::string config_json() const;
    const std::string &status_text() const { return status_text_; }

  private:
    WizardPhase phase_ = WizardPhase::WaitingForPress;
    int index_ = 0;
    std::vector<WizardBinding> bindings_;

    std::vector<uint8_t> stable_candidate_;
    int64_t stable_since_us_ = 0;
    bool printed_config_ = false;
    std::string status_text_;
};
