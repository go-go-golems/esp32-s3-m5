#include "gnosis_app.h"
#include "layout_engine.h"
#include "widget_renderer.h"

#include <cstdio>
#include <cstring>
#include <esp_timer.h>

namespace gnosis {

static constexpr int16_t kScreenW = 960;
static constexpr int16_t kScreenH = 540;
static constexpr std::uint32_t kLoopDelayMs = 16;
static constexpr std::uint32_t kDataUpdateIntervalMs = 1000;
static constexpr std::uint32_t kFullRefreshEveryN = 60;

static GnosisApp s_app;

GnosisApp& GetApp() { return s_app; }

void GnosisApp::InitBoard()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorFg, kColorBg);
}

void GnosisApp::BuildCurrentScreen()
{
    pool_.Reset();
    clock_label_ = nullptr;
    sec_label_ = nullptr;
    boot_bar_ = nullptr;
    boot_pct_ = nullptr;

    if (current_preset_ >= 0 && current_preset_ < kPresetCount) {
        screen_ = kPresets[current_preset_].build(pool_);
    }
    LayoutScreen(screen_, kScreenW, kScreenH);

    // Try to find updatable nodes for the dashboard
    // Clock label: look for "14:37" in the body tree
    if (screen_.body) {
        // Walk FIXED children in left panel of dashboard
        if (screen_.body->type == NodeType::HBOX && screen_.body->n_children >= 1) {
            Node* left = screen_.body->children[0];
            if (left && left->type == NodeType::FIXED) {
                for (int i = 0; i < left->n_children; ++i) {
                    Node* c = left->children[i];
                    if (c && c->type == NodeType::LABEL && c->props[0] == 4) {
                        clock_label_ = c;
                    }
                    if (c && c->type == NodeType::LABEL && std::strstr(c->text, "SEC")) {
                        sec_label_ = c;
                    }
                }
            }
        }
        // Boot bar: look for BAR in body
        if (screen_.body->type == NodeType::FIXED) {
            for (int i = 0; i < screen_.body->n_children; ++i) {
                Node* c = screen_.body->children[i];
                if (c && c->type == NodeType::BAR) {
                    boot_bar_ = c;
                }
                if (c && c->type == NodeType::LABEL && std::strstr(c->text, "%")) {
                    boot_pct_ = c;
                }
            }
        }
    }
}

void GnosisApp::FullRefresh()
{
    M5.Display.waitDisplay();
    // Use epd_quality for a true full-flash refresh that clears ghosting.
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.startWrite();
    M5.Display.fillScreen(kColorBg);

    Rect full = {0, 0, kScreenW, kScreenH};
    RenderSubtree(M5.Display, screen_.bar, full);
    RenderSubtree(M5.Display, screen_.body, full);
    RenderSubtree(M5.Display, screen_.nav, full);

    M5.Display.endWrite();
    M5.Display.waitDisplay();

    // Clear all dirty flags after full refresh
    auto clear_dirty = [](Node* n, auto& self) -> void {
        if (!n) return;
        n->dirty = false;
        for (int i = 0; i < n->n_children; ++i) self(n->children[i], self);
    };
    clear_dirty(screen_.bar, clear_dirty);
    clear_dirty(screen_.body, clear_dirty);
    clear_dirty(screen_.nav, clear_dirty);

    partial_refresh_count_ = 0;
}

void GnosisApp::HandleTouch()
{
    const bool has_touch = M5.Touch.getCount() > 0;

    if (has_touch && !touch_down_) {
        touch_down_ = true;
        const auto& detail = M5.Touch.getDetail();
        int16_t tx = static_cast<int16_t>(detail.x);
        int16_t ty = static_cast<int16_t>(detail.y);

        // Check nav bar icons for screen switching
        if (screen_.nav && screen_.nav->rect.Contains(tx, ty)) {
            // Find which icon was tapped
            for (int i = 0; i < screen_.nav->n_children; ++i) {
                Node* c = screen_.nav->children[i];
                if (c && c->type == NodeType::ICON && c->rect.Contains(tx, ty)) {
                    int target = c->props[0];  // shape index 0-3 maps to presets
                    if (target >= 0 && target < kPresetCount && target != current_preset_) {
                        SwitchPreset(target);
                        return;
                    }
                }
            }
            // Check badge for cycling through remaining presets
            for (int i = 0; i < screen_.nav->n_children; ++i) {
                Node* c = screen_.nav->children[i];
                if (c && c->type == NodeType::BADGE && c->rect.Contains(tx, ty)) {
                    int next = (current_preset_ + 1) % kPresetCount;
                    SwitchPreset(next);
                    return;
                }
            }
        }
    } else if (!has_touch) {
        touch_down_ = false;
    }
}

void GnosisApp::UpdateData()
{
    std::uint32_t now = static_cast<std::uint32_t>(esp_timer_get_time() / 1000ULL);
    if (now - last_data_update_ms_ < kDataUpdateIntervalMs) return;
    last_data_update_ms_ = now;

    // Update clock on dashboard
    if (clock_label_) {
        uint32_t secs = now / 1000;
        uint32_t mins = (secs / 60) % 60;
        uint32_t hrs = (secs / 3600) % 24;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)hrs, (unsigned long)mins);
        if (std::strcmp(clock_label_->text, buf) != 0) {
            std::strncpy(clock_label_->text, buf, kMaxTextLen - 1);
            MarkDirty(clock_label_);
        }
    }
    if (sec_label_) {
        uint32_t secs = (now / 1000) % 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "SEC %02lu", (unsigned long)secs);
        if (std::strcmp(sec_label_->text, buf) != 0) {
            std::strncpy(sec_label_->text, buf, kMaxTextLen - 1);
            MarkDirty(sec_label_);
        }
    }

    // Animate boot bar
    if (boot_bar_) {
        int16_t val = boot_bar_->props[1];
        val = static_cast<int16_t>((val + 2) % 101);
        boot_bar_->props[1] = val;
        MarkDirty(boot_bar_);

        if (boot_pct_) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%d%%", val);
            std::strncpy(boot_pct_->text, buf, kMaxTextLen - 1);
            MarkDirty(boot_pct_);
        }
    }

    // Animate dashboard gauges
    if (screen_.body && screen_.body->type == NodeType::HBOX && screen_.body->n_children >= 2) {
        Node* right = screen_.body->children[1];
        if (right && right->type == NodeType::FIXED) {
            for (int i = 0; i < right->n_children; ++i) {
                Node* c = right->children[i];
                if (c && c->type == NodeType::GAUGE) {
                    c->props[1] = static_cast<int16_t>((c->props[1] + 3) % (c->props[0] + 1));
                    MarkDirty(c);
                }
            }
        }
    }
}

void GnosisApp::ProcessDirtyRefresh()
{
    DirtyCollector dc;
    dc.Collect(screen_.bar);
    dc.Collect(screen_.body);
    dc.Collect(screen_.nav);

    if (dc.count == 0) return;
    dc.Merge();

    for (std::size_t i = 0; i < dc.count; ++i) {
        Rect& r = dc.rects[i];
        epd_mode_t mode = epd_mode_t::epd_text;
        if (dc.waveforms[i] == Waveform::FAST)
            mode = epd_mode_t::epd_fast;
        else if (dc.waveforms[i] == Waveform::FULL)
            mode = epd_mode_t::epd_quality;

        M5.Display.waitDisplay();
        M5.Display.setEpdMode(mode);
        M5.Display.startWrite();
        M5.Display.setClipRect(r.x, r.y, r.w, r.h);
        M5.Display.fillRect(r.x, r.y, r.w, r.h, kColorBg);

        RenderSubtree(M5.Display, screen_.bar, r);
        RenderSubtree(M5.Display, screen_.body, r);
        RenderSubtree(M5.Display, screen_.nav, r);

        M5.Display.clearClipRect();
        M5.Display.endWrite();
    }

    partial_refresh_count_ += dc.count;

    // Periodic full refresh to clear ghosting
    if (partial_refresh_count_ >= kFullRefreshEveryN) {
        FullRefresh();
    }
}

void GnosisApp::SwitchPreset(int index)
{
    if (index < 0 || index >= kPresetCount) return;
    current_preset_ = index;
    std::printf("gnosis: switching to preset '%s'\n", kPresets[index].name);
    BuildCurrentScreen();
    FullRefresh();
}

void GnosisApp::SwitchPresetByName(const char* name)
{
    for (int i = 0; i < kPresetCount; ++i) {
        if (std::strcmp(kPresets[i].name, name) == 0) {
            SwitchPreset(i);
            return;
        }
    }
    std::printf("gnosis: unknown preset '%s'\n", name);
    ListPresets();
}

void GnosisApp::ListPresets()
{
    std::printf("available presets:\n");
    for (int i = 0; i < kPresetCount; ++i) {
        std::printf("  [%d] %s%s\n", i, kPresets[i].name,
                    i == current_preset_ ? " (active)" : "");
    }
    std::printf("nodes used: %zu / %zu\n", pool_.Used(), kMaxNodes);
}

void GnosisApp::ForceFullRefresh()
{
    FullRefresh();
    std::printf("gnosis: full refresh done\n");
}

void GnosisApp::SetGaugeValue(const char* label, int value)
{
    if (!screen_.body) return;
    // Walk tree looking for GAUGE nodes
    auto find_gauge = [&](Node* n, auto& self) -> bool {
        if (!n) return false;
        if (n->type == NodeType::GAUGE && n->text[0] == label[0]) {
            n->props[1] = static_cast<int16_t>(value);
            MarkDirty(n);
            std::printf("gnosis: gauge %s = %d\n", label, value);
            return true;
        }
        for (int i = 0; i < n->n_children; ++i) {
            if (self(n->children[i], self)) return true;
        }
        return false;
    };
    if (!find_gauge(screen_.body, find_gauge)) {
        std::printf("gnosis: gauge '%s' not found\n", label);
    }
}

void GnosisApp::Run()
{
    InitBoard();
    BuildCurrentScreen();
    FullRefresh();

    while (true) {
        M5.update();
        HandleTouch();
        UpdateData();
        ProcessDirtyRefresh();
        M5.delay(kLoopDelayMs);
    }
}

}  // namespace gnosis
