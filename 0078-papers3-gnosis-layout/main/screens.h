#pragma once

#include "gnosis_types.h"

namespace gnosis {

// Screen preset builders. Each returns a populated Screen with nodes from the pool.

Screen BuildDashboard(NodePool& pool);
Screen BuildCalendar(NodePool& pool);
Screen BuildBoot(NodePool& pool);
Screen BuildWidgetGallery(NodePool& pool);
Screen BuildTelemetry(NodePool& pool);
Screen BuildReader(NodePool& pool);
Screen BuildMinimal(NodePool& pool);

struct PresetEntry {
    const char* name;
    Screen (*build)(NodePool& pool);
};

// All available presets, null-terminated.
extern const PresetEntry kPresets[];
extern const int kPresetCount;

}  // namespace gnosis
