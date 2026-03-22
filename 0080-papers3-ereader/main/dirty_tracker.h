#pragma once

#include "gnosis_types.h"

namespace gnosis {

static constexpr std::size_t kMaxDirtyRects = 32;
static constexpr int32_t kMergeThreshold = 1024;

struct DirtyCollector {
    Rect rects[kMaxDirtyRects]{};
    Waveform waveforms[kMaxDirtyRects]{};
    std::size_t count = 0;

    void Reset() { count = 0; }
    void Collect(Node* node);
    void Merge();
};

void MarkDirty(Node* node);
void MarkAllDirty(Node* node);

}  // namespace gnosis
