#pragma once

#include "dirty_tracker.h"
#include "gnosis_types.h"
#include "screens.h"

#include <M5Unified.hpp>
#include <cstdint>

namespace gnosis {

class GnosisApp {
public:
    void Run();

    // Called from console command handler.
    void SwitchPreset(int index);
    void SwitchPresetByName(const char* name);
    void ListPresets();
    void ForceFullRefresh();
    void SetGaugeValue(const char* label, int value);

    int CurrentPreset() const { return current_preset_; }
    NodePool& Pool() { return pool_; }
    Screen& CurrentScreen() { return screen_; }

private:
    void InitBoard();
    void BuildCurrentScreen();
    void FullRefresh();
    void HandleTouch();
    void UpdateData();
    void ProcessDirtyRefresh();

    NodePool pool_;
    Screen screen_{};
    int current_preset_ = 0;

    bool touch_down_ = false;
    std::uint32_t last_data_update_ms_ = 0;
    std::uint32_t partial_refresh_count_ = 0;

    // Pointers to updatable nodes (set during build)
    Node* clock_label_ = nullptr;
    Node* sec_label_ = nullptr;
    Node* boot_bar_ = nullptr;
    Node* boot_pct_ = nullptr;
};

// Singleton access for console commands.
GnosisApp& GetApp();

}  // namespace gnosis
