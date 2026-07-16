#include "app_ui.h"

#include <cstdio>
#include <cstring>
#include <new>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "app_display.h"
#include "app_input.h"
#include "s3paper/text.h"
#include "s3paper/widget_diff.h"

namespace reader {
namespace {

const char *kTag = "ui";

constexpr s3paper::Rect kPageBounds{0, 0, 540, 960};
constexpr uint32_t kMaxRegionSpecs = s3paper::RegionTable::kCapacity;

s3paper::WidgetArena *s_arena = nullptr;
s3paper::PageRouter s_router;
s3paper::RenderStateDiff s_diff;
s3paper::RegionTable s_regions;
s3paper::LayoutEntry s_entries[s3paper::WidgetArena::kCapacity];

// Last presented page (region ticks re-render it with a damage clip).
s3paper::PageSlots s_last_slots{};
UiExtraOps s_last_extra = nullptr;
bool s_last_valid = false;

uint32_t s_present_count = 0;
bool s_trace_present = false;

// Fixture state: one live clock region driven by the owner loop.
bool s_fixture_active = false;
s3paper::WidgetHandle s_fixture_clock{};
int64_t s_fixture_due_us = 0;
uint32_t s_fixture_interval_ms = 0;

// Layout + compile into the current frame builder. Returns entry count.
s3paper::Result<uint32_t> BuildPageOps(const s3paper::PageSlots &slots,
                                       s3paper::HitRegion *hits,
                                       uint32_t hit_cap,
                                       uint32_t *out_hit_count) {
    s3paper::FrameBuilder &fb = FrameBuilderRef();
    const s3paper::Result<uint32_t> laid = s3paper::LayoutPage(
        *s_arena, slots, kPageBounds, s_entries,
        s3paper::WidgetArena::kCapacity);
    if (!laid.ok()) {
        return laid;
    }
    s3paper::RegionSpec specs[kMaxRegionSpecs];
    const s3paper::Result<s3paper::CompileResult> compiled =
        s3paper::CompileTree(*s_arena, s_entries, laid.value, fb, hits,
                             hit_cap, specs, kMaxRegionSpecs);
    if (!compiled.ok()) {
        return s3paper::Result<uint32_t>::Err(compiled.code);
    }
    s_regions.Clear();
    for (uint32_t i = 0; i < compiled.value.region_count; ++i) {
        (void)s_regions.Add(specs[i]);
    }
    if (out_hit_count != nullptr) {
        *out_hit_count = compiled.value.hit_count;
    }
    return laid;
}

}  // namespace

void UiInit() {
    if (s_arena != nullptr) {
        return;
    }
    void *mem = heap_caps_malloc(sizeof(s3paper::WidgetArena),
                                 MALLOC_CAP_SPIRAM);
    if (mem == nullptr) {
        mem = malloc(sizeof(s3paper::WidgetArena));
    }
    s_arena = new (mem) s3paper::WidgetArena();
    s_router.Reset();
    s_diff.Reset();
    ESP_LOGI(kTag, "widget arena ready (%u nodes, %u bytes)",
             static_cast<unsigned>(s3paper::WidgetArena::kCapacity),
             static_cast<unsigned>(sizeof(s3paper::WidgetArena)));
}

s3paper::WidgetArena &UiArena() {
    UiInit();
    return *s_arena;
}

s3paper::PageRouter &UiRouter() { return s_router; }

UiPresentResult UiPresentPage(const s3paper::PageSlots &slots,
                              s3paper::PresentIntent intent,
                              bool screen_change, s3paper::HitRegion *hits,
                              uint32_t hit_cap, UiExtraOps extra_ops) {
    UiInit();
    s_fixture_active = false;  // whoever presents now owns the panel
    UiPresentResult out{StatusCode::Busy, 0, false};
    s3paper::FrameBuilder &fb = FrameBuilderRef();
    fb.Begin();
    s3paper::Status st = fb.FillRect(kPageBounds, 255);
    if (!st.ok()) {
        out.status = st.code;
        return out;
    }
    const s3paper::Result<uint32_t> laid =
        BuildPageOps(slots, hits, hit_cap, &out.hit_count);
    if (!laid.ok()) {
        out.status = laid.code;
        return out;
    }
    if (extra_ops != nullptr) {
        const StatusCode extra = extra_ops(fb, s_entries, laid.value);
        if (extra != StatusCode::Ok) {
            out.status = extra;
            return out;
        }
    }
    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) {
        out.status = frame.code;
        return out;
    }
    const s3paper::Status init = EnsureM5Init();
    if (!init.ok()) {
        out.status = init.code;
        return out;
    }
    if (screen_change) {
        Planner().NoteScreenChange();
    }
    const PlannedPresent presented =
        PresentFramePlanned(frame.value, intent, !s_trace_present);
    out.status = presented.present.status;
    out.full_refresh = presented.plan.full_refresh;
    if (out.status == StatusCode::Ok) {
        s_diff.Capture(*s_arena, s_entries, laid.value);
        s_last_slots = slots;
        s_last_extra = extra_ops;
        s_last_valid = true;
        s_present_count++;
    }
    return out;
}

uint32_t UiPresentCount() { return s_present_count; }

void UiSetTracePresent(bool enabled) { s_trace_present = enabled; }

// ---- Fixtures ----

namespace {

// Hello fixture: proves the generic pipeline end to end.
StatusCode BuildHelloFixture(s3paper::PageSlots *out) {
    s3paper::WidgetArena &a = *s_arena;
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
    s3paper::WidgetArena &a = *s_arena;
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
    const UiPresentResult presented = UiPresentPage(
        slots, s3paper::PresentIntent::CleanFull, true, nullptr, 0, nullptr);
    if (presented.status != StatusCode::Ok) {
        return presented.status;
    }
    if (which == 2) {
        const s3paper::RegionSpec *clock_region = s_regions.Find(1);
        s_fixture_interval_ms =
            clock_region != nullptr ? clock_region->interval_ms : 2000;
        s_fixture_due_us =
            esp_timer_get_time() +
            static_cast<int64_t>(s_fixture_interval_ms) * 1000;
        s_fixture_active = true;
        ESP_LOGI(kTag, "status fixture: clock region live (%u ms interval)",
                 static_cast<unsigned>(s_fixture_interval_ms));
    }
    return StatusCode::Ok;
}

void UiRegionTick(int64_t now_us) {
    if (!s_fixture_active || now_us < s_fixture_due_us || !s_last_valid) {
        return;
    }
    // Quiet-while-active regions defer while the user is interacting
    // (design §9.6): retry shortly after instead of fighting page turns.
    const s3paper::RegionSpec *spec = s_regions.Find(1);
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
    if (!s_arena->SetText(s_fixture_clock, text).ok()) {
        s_fixture_active = false;  // tree was rebuilt under us
        return;
    }

    // Re-layout and diff against the presented snapshot for exact damage.
    const s3paper::Result<uint32_t> laid = s3paper::LayoutPage(
        *s_arena, s_last_slots, kPageBounds, s_entries,
        s3paper::WidgetArena::kCapacity);
    if (!laid.ok()) {
        return;
    }
    s3paper::Rect damage[8];
    const s3paper::Result<uint32_t> rects =
        s_diff.Diff(*s_arena, s_entries, laid.value, damage, 8);
    s3paper::Rect clip = kPageBounds;  // fallback: whole page
    if (rects.ok()) {
        if (rects.value == 0) {
            return;  // nothing visible changed
        }
        clip = damage[0];
        for (uint32_t i = 1; i < rects.value; ++i) {
            const s3paper::Result<s3paper::Rect> u = Union(clip, damage[i]);
            if (u.ok()) {
                clip = u.value;
            }
        }
    }

    // Clipped re-render: only ops intersecting the damage survive, so the
    // frame's damage equals the region and the planner refreshes just it.
    s3paper::FrameBuilder &fb = FrameBuilderRef();
    fb.Begin();
    if (!fb.PushClip(clip).ok()) {
        return;
    }
    (void)fb.FillRect(kPageBounds, 255);
    uint32_t hit_count = 0;
    const s3paper::Result<uint32_t> rebuilt =
        BuildPageOps(s_last_slots, nullptr, 0, &hit_count);
    if (!rebuilt.ok()) {
        (void)fb.PopClip();
        return;
    }
    if (s_last_extra != nullptr) {
        (void)s_last_extra(fb, s_entries, rebuilt.value);
    }
    (void)fb.PopClip();
    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) {
        return;
    }
    const PlannedPresent presented = PresentFramePlanned(
        frame.value, s3paper::PresentIntent::TextRegion, true);
    if (presented.present.status == StatusCode::Ok) {
        s_diff.Capture(*s_arena, s_entries, rebuilt.value);
        ESP_LOGI(kTag,
                 "region update: damage %dx%d at (%d,%d) full=%d ops=%u",
                 static_cast<int>(frame.value.damage.w),
                 static_cast<int>(frame.value.damage.h),
                 static_cast<int>(frame.value.damage.x),
                 static_cast<int>(frame.value.damage.y),
                 presented.plan.full_refresh ? 1 : 0,
                 static_cast<unsigned>(frame.value.op_count));
    }
}

}  // namespace reader
