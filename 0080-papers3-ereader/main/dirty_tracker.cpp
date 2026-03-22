#include "dirty_tracker.h"

namespace gnosis {

void MarkDirty(Node* node)
{
    if (node) node->dirty = true;
}

void MarkAllDirty(Node* node)
{
    if (!node) return;
    node->dirty = true;
    for (int i = 0; i < node->n_children; ++i) {
        MarkAllDirty(node->children[i]);
    }
}

void DirtyCollector::Collect(Node* node)
{
    if (!node) return;

    if (node->dirty) {
        if (node->n_children == 0) {
            if (count < kMaxDirtyRects) {
                rects[count] = node->rect;
                waveforms[count] = node->waveform;
                ++count;
            }
            node->dirty = false;
        } else {
            for (int i = 0; i < node->n_children; ++i) {
                Collect(node->children[i]);
            }
            node->dirty = false;
        }
    } else {
        for (int i = 0; i < node->n_children; ++i) {
            if (node->children[i] && node->children[i]->dirty) {
                Collect(node->children[i]);
            }
        }
    }
}

void DirtyCollector::Merge()
{
    bool merged_any;
    do {
        merged_any = false;
        for (std::size_t i = 0; i < count && !merged_any; ++i) {
            for (std::size_t j = i + 1; j < count && !merged_any; ++j) {
                Rect u = rects[i].Union(rects[j]);
                int32_t waste = u.Area() - rects[i].Area() - rects[j].Area();
                if (waste < kMergeThreshold) {
                    rects[i] = u;
                    if (waveforms[j] > waveforms[i])
                        waveforms[i] = waveforms[j];
                    rects[j] = rects[count - 1];
                    waveforms[j] = waveforms[count - 1];
                    --count;
                    merged_any = true;
                }
            }
        }
    } while (merged_any);
}

}  // namespace gnosis
