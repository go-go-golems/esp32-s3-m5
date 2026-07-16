#include "app_ui.h"

#include <cstdio>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "app_input.h"
#include "s3paper/text.h"

namespace reader {
namespace {

const char *kTag = "ui";

// Fixture state: one live clock region driven by the owner loop. The
// fixture keeps its own slots and the present count at its last present;
// a different count means another screen took the panel (same contract
// the JS layer uses).
bool s_fixture_active = false;
s3paper::PageSlots s_fixture_slots{};
s3paper::WidgetHandle s_fixture_clock{};
int64_t s_fixture_due_us = 0;
uint32_t s_fixture_interval_ms = 0;
uint32_t s_fixture_present_count = 0;

// Hello fixture: proves the generic pipeline end to end.
StatusCode BuildHelloFixture(s3paper::PageSlots *out) {
    s3paper::WidgetArena &a = UiArena();
    a.Reset();
    s3paper::WidgetHandle header = s3paper::NewCol(a).value;
    s3paper::WidgetNode *hn = a.Configure(header);
    hn->padding = s3paper::Insets{16, 40, 4, 40};
    hn->gap = 8;
    (void)a.AddChild(header,
                     s3paper::NewText(a, "Widget fixture: hello",
                                      s3paper::kFontUi, 0)
                         .value);
    (void)a.AddChild(header, s3paper::NewDivider(a, 2, 0).value);

    s3paper::WidgetHandle content = s3paper::NewCol(a).value;
    s3paper::WidgetNode *cn = a.Configure(content);
    cn->padding = s3paper::Insets{24, 40, 24, 40};
    cn->gap = 16;
    (void)a.AddChild(content, s3paper::NewText(a, "Hello, PaperS3.",
                                               s3paper::kFontBody, 0)
                                  .value);
    (void)a.AddChild(
        content,
        s3paper::NewText(a, "Rows, columns, dividers,", s3paper::kFontBody, 0)
            .value);
    s3paper::WidgetHandle row = s3paper::NewRow(a).value;
    a.Configure(row)->gap = 12;
    (void)a.AddChild(row, s3paper::NewText(a, "progress:",
                                           s3paper::kFontBody, 0)
                              .value);
    s3paper::WidgetHandle bar = s3paper::NewProgress(a, 640, 24, 0).value;
    a.Configure(bar)->fixed_h = 24;
    (void)a.AddChild(row, bar);
    (void)a.AddChild(content, row);
    (void)a.AddChild(content,
                     s3paper::NewText(a, "centered text",
                                      s3paper::kFontBody, 96,
                                      s3paper::TextAlign::Center)
                         .value);

    s3paper::WidgetHandle footer = s3paper::NewCol(a).value;
    s3paper::WidgetNode *fn = a.Configure(footer);
    fn->padding = s3paper::Insets{6, 40, 10, 40};
    fn->gap = 6;
    (void)a.AddChild(footer, s3paper::NewDivider(a, 1, 0).value);
    (void)a.AddChild(footer,
                     s3paper::NewText(a, "generic tree -> draw ops -> EPD",
                                      s3paper::kFontUi, 96)
                         .value);
    *out = s3paper::PageSlots{header, content, footer, s3paper::kNullWidget};
    return StatusCode::Ok;
}

// Status fixture: a live uptime clock inside an interval Region.
StatusCode BuildStatusFixture(s3paper::PageSlots *out) {
    s3paper::WidgetArena &a = UiArena();
    a.Reset();
    s3paper::WidgetHandle header = s3paper::NewCol(a).value;
    s3paper::WidgetNode *hn = a.Configure(header);
    hn->padding = s3paper::Insets{16, 40, 4, 40};
    hn->gap = 8;
    (void)a.AddChild(header,
                     s3paper::NewText(a, "Widget fixture: status",
                                      s3paper::kFontUi, 0)
                         .value);
    (void)a.AddChild(header, s3paper::NewDivider(a, 2, 0).value);

    s3paper::WidgetHandle content = s3paper::NewCol(a).value;
    s3paper::WidgetNode *cn = a.Configure(content);
    cn->padding = s3paper::Insets{24, 40, 24, 40};
    cn->gap = 14;
    char line[64];
    snprintf(line, sizeof(line), "free heap: %u KB",
             static_cast<unsigned>(
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
    (void)a.AddChild(content,
                     s3paper::NewText(a, line, s3paper::kFontBody, 0).value);
    snprintf(line, sizeof(line), "free PSRAM: %u KB",
             static_cast<unsigned>(
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    (void)a.AddChild(content,
                     s3paper::NewText(a, line, s3paper::kFontBody, 0).value);

    // The live region: quiet clock chip from the design doc (§12.3),
    // updating in place through diff-driven TextRegion presents.
    s3paper::WidgetHandle region = s3paper::NewRegion(a, 1, 2000, true).value;
    s3paper::WidgetNode *rn = a.Configure(region);
    rn->dependency = 1;
    rn->padding = s3paper::Insets{8, 0, 8, 0};
    s_fixture_clock =
        s3paper::NewText(a, "uptime: 0 s", s3paper::kFontBody, 0).value;
    a.Configure(s_fixture_clock)->dependency = 1;
    (void)a.AddChild(region, s_fixture_clock);
    (void)a.AddChild(content, region);

    s3paper::WidgetHandle footer = s3paper::NewCol(a).value;
    s3paper::WidgetNode *fn = a.Configure(footer);
    fn->padding = s3paper::Insets{6, 40, 10, 40};
    fn->gap = 6;
    (void)a.AddChild(footer, s3paper::NewDivider(a, 1, 0).value);
    (void)a.AddChild(footer,
                     s3paper::NewText(a,
                                      "region updates partially every 2 s",
                                      s3paper::kFontUi, 96)
                         .value);
    *out = s3paper::PageSlots{header, content, footer, s3paper::kNullWidget};
    return StatusCode::Ok;
}

}  // namespace

StatusCode UiBuildFixtureSlots(uint32_t which, s3paper::PageSlots *out) {
    UiInit();
    return which == 2 ? BuildStatusFixture(out) : BuildHelloFixture(out);
}

StatusCode UiRunFixture(uint32_t which) {
    UiInit();
    s3paper::PageSlots slots{};
    const StatusCode built = UiBuildFixtureSlots(which, &slots);
    if (built != StatusCode::Ok) {
        return built;
    }
    s_fixture_active = false;
    const UiPresentResult presented = UiPresentPage(
        slots, s3paper::PresentIntent::CleanFull, true, nullptr, 0, nullptr);
    if (presented.status != StatusCode::Ok) {
        return presented.status;
    }
    if (which == 2) {
        const s3paper::RegionSpec *clock_region =
            s3paper_runtime::FindRegion(1);
        s_fixture_interval_ms =
            clock_region != nullptr ? clock_region->interval_ms : 2000;
        s_fixture_due_us =
            esp_timer_get_time() +
            static_cast<int64_t>(s_fixture_interval_ms) * 1000;
        s_fixture_slots = slots;
        s_fixture_present_count = UiPresentCount();
        s_fixture_active = true;
        ESP_LOGI(kTag, "status fixture: clock region live (%u ms interval)",
                 static_cast<unsigned>(s_fixture_interval_ms));
    }
    return StatusCode::Ok;
}

void UiRegionTick(int64_t now_us) {
    if (!s_fixture_active || now_us < s_fixture_due_us) {
        return;
    }
    // Another screen presented since our last present: it owns the panel.
    if (UiPresentCount() != s_fixture_present_count) {
        s_fixture_active = false;
        return;
    }
    // Quiet-while-active regions defer while the user is interacting
    // (design §9.6): retry shortly after instead of fighting page turns.
    const s3paper::RegionSpec *spec = s3paper_runtime::FindRegion(1);
    if (spec != nullptr && spec->quiet_while_active) {
        const int64_t last_input = InputLastInputUs();
        if (last_input != 0 && now_us - last_input < 2'000'000) {
            s_fixture_due_us = now_us + 500'000;
            return;
        }
    }
    s_fixture_due_us =
        now_us + static_cast<int64_t>(s_fixture_interval_ms) * 1000;

    char text[32];
    snprintf(text, sizeof(text), "uptime: %lld s",
             static_cast<long long>(now_us / 1000000));
    if (!UiArena().SetText(s_fixture_clock, text).ok()) {
        s_fixture_active = false;  // tree was rebuilt under us
        return;
    }

    // Diff-driven update: only the clock's damage blits (zero rects when
    // the second didn't visibly change the text).
    const UiPresentResult updated =
        UiPresentPageUpdate(s_fixture_slots, nullptr, 0, nullptr);
    if (updated.status == StatusCode::Ok) {
        s_fixture_present_count = UiPresentCount();
    } else {
        s_fixture_active = false;
    }
}

}  // namespace reader
