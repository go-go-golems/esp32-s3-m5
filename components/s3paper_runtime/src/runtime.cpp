#include "s3paper_runtime/runtime.h"

#include <cstdio>
#include <cstring>
#include <new>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "s3paper/fake_backend.h"
#include "s3paper/frame_arena.h"
#include "s3paper/text.h"
#include "s3paper/widget_diff.h"
#include "s3paper_m5/m5_backend.h"

namespace s3paper_runtime {
namespace {

const char *kTag = "runtime";

// ---- Frame storage / backends / planner ----

s3paper::Size s_viewport{540, 960};
s3paper::Rect s_page_bounds{0, 0, 540, 960};
s3paper::DrawOp *s_ops = nullptr;
uint8_t *s_arena_buf = nullptr;
char *s_trace_buf = nullptr;
s3paper::FrameArena *s_frame_arena = nullptr;
s3paper::FrameBuilder *s_builder = nullptr;
s3paper::FakeBackend *s_fake = nullptr;
s3paper::M5Backend *s_m5 = nullptr;
s3paper::RefreshPlanner *s_planner = nullptr;
s3paper::FrameId s_next_frame_id = 1;

// ---- Widget pipeline state ----

constexpr uint32_t kMaxRegionSpecs = s3paper::RegionTable::kCapacity;

s3paper::WidgetArena *s_widgets = nullptr;
s3paper::PageRouter s_router;
s3paper::RenderStateDiff s_diff;
s3paper::RegionTable s_regions;
s3paper::LayoutEntry s_entries[s3paper::WidgetArena::kCapacity];

// Last presented page (updates re-render it with a damage clip).
s3paper::PageSlots s_last_slots{};
ExtraOps s_last_extra = nullptr;
bool s_last_valid = false;

uint32_t s_present_count = 0;
bool s_trace_present = false;

// Presents a frozen frame through the planner on the chosen backend and
// commits the result to refresh history.
PlannedPresent PresentPlanned(const s3paper::RenderFrame &frame,
                              s3paper::PresentIntent intent, bool use_m5) {
    PlannedPresent out{};
    s3paper::Status st = s_planner->AddDamage(frame.damage);
    if (!st.ok()) {
        out.present.status = st.code;
        return out;
    }
    const int64_t now_us = esp_timer_get_time();
    out.plan = s_planner->Plan(intent, now_us);
    // The backend still consumes per-op clip rects; the plan chooses the
    // effective intent (a planner-forced full becomes CleanFull).
    const s3paper::PresentIntent effective =
        out.plan.full_refresh ? s3paper::PresentIntent::CleanFull : intent;
    if (use_m5) {
        out.present = s_m5->Present(frame, effective);
    } else {
        s_fake->ClearTrace();
        out.present = s_fake->Present(frame, effective);
    }
    if (out.present.status == s3paper::StatusCode::Ok) {
        s_planner->RecordPresent(out.plan, esp_timer_get_time());
    } else {
        s_planner->ClearDamage();
    }
    return out;
}

// Layout + compile into the current frame builder. Returns entry count.
s3paper::Result<uint32_t> BuildPageOps(const s3paper::PageSlots &slots,
                                       s3paper::HitRegion *hits,
                                       uint32_t hit_cap,
                                       uint32_t *out_hit_count) {
    s3paper::FrameBuilder &fb = *s_builder;
    const s3paper::Result<uint32_t> laid = s3paper::LayoutPage(
        *s_widgets, slots, s_page_bounds, s_entries,
        s3paper::WidgetArena::kCapacity);
    if (!laid.ok()) {
        return laid;
    }
    s3paper::RegionSpec specs[kMaxRegionSpecs];
    const s3paper::Result<s3paper::CompileResult> compiled =
        s3paper::CompileTree(*s_widgets, s_entries, laid.value, fb, hits,
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

// Embedded font subsets from s3paper_core (Latin + Ukrainian). PT Serif
// carries the reading faces; Liberation Sans Bold the display faces.
extern "C" const uint8_t _binary_PTSerifUkr_ttf_start[];
extern "C" const uint8_t _binary_PTSerifUkr_ttf_end[];
extern "C" const uint8_t _binary_LibSansBoldUkr_ttf_start[];
extern "C" const uint8_t _binary_LibSansBoldUkr_ttf_end[];

void RuntimeInit(const RuntimeConfig &config) {
    if (s_builder != nullptr) {
        return;
    }
    if (config.register_default_fonts) {
        const uint32_t serif_size = static_cast<uint32_t>(
            _binary_PTSerifUkr_ttf_end - _binary_PTSerifUkr_ttf_start);
        const s3paper::Status ui_font = s3paper::RegisterTtfFont(
            s3paper::kFontUi, _binary_PTSerifUkr_ttf_start, serif_size, 22);
        const s3paper::Status body_font = s3paper::RegisterTtfFont(
            s3paper::kFontBody, _binary_PTSerifUkr_ttf_start, serif_size,
            34);
        const uint32_t sans_size = static_cast<uint32_t>(
            _binary_LibSansBoldUkr_ttf_end -
            _binary_LibSansBoldUkr_ttf_start);
        const s3paper::Status display_font = s3paper::RegisterTtfFont(
            s3paper::kFontDisplay, _binary_LibSansBoldUkr_ttf_start,
            sans_size, 44);
        const s3paper::Status xl_font = s3paper::RegisterTtfFont(
            s3paper::kFontXL, _binary_LibSansBoldUkr_ttf_start, sans_size,
            84);
        if (!ui_font.ok() || !body_font.ok() || !display_font.ok() ||
            !xl_font.ok()) {
            ESP_LOGE(kTag, "font registration failed (%s/%s/%s/%s)",
                     s3paper::StatusCodeName(ui_font.code),
                     s3paper::StatusCodeName(body_font.code),
                     s3paper::StatusCodeName(display_font.code),
                     s3paper::StatusCodeName(xl_font.code));
        } else {
            const s3paper::FontLineMetrics body =
                s3paper::GetFontLineMetrics(s3paper::kFontBody).value;
            ESP_LOGI(kTag,
                     "fonts registered: serif=%uB sans=%uB body "
                     "line_height=%d",
                     static_cast<unsigned>(serif_size),
                     static_cast<unsigned>(sans_size),
                     static_cast<int>(body.line_height));
        }
    }
    s_viewport = config.viewport;
    s_page_bounds = s3paper::Rect{0, 0, config.viewport.w,
                                  config.viewport.h};
    s_ops = static_cast<s3paper::DrawOp *>(heap_caps_malloc(
        config.op_capacity * sizeof(s3paper::DrawOp), MALLOC_CAP_SPIRAM));
    s_arena_buf = static_cast<uint8_t *>(
        heap_caps_malloc(config.arena_capacity, MALLOC_CAP_SPIRAM));
    s_trace_buf = static_cast<char *>(
        heap_caps_malloc(config.trace_capacity, MALLOC_CAP_SPIRAM));
    void *widgets_mem = heap_caps_malloc(sizeof(s3paper::WidgetArena),
                                         MALLOC_CAP_SPIRAM);
    if (widgets_mem == nullptr) {
        widgets_mem = malloc(sizeof(s3paper::WidgetArena));
    }
    if (s_ops == nullptr || s_arena_buf == nullptr ||
        s_trace_buf == nullptr || widgets_mem == nullptr) {
        ESP_LOGE(kTag, "PSRAM allocation for frame storage failed");
        abort();
    }
    s3paper::RefreshPolicy policy;
    policy.max_turns_between_full = config.max_turns_between_full;
    static s3paper::FrameArena arena(s_arena_buf, config.arena_capacity);
    static s3paper::FrameBuilder builder(s_ops, config.op_capacity, &arena,
                                         s_viewport);
    static s3paper::FakeBackend fake(s_trace_buf, config.trace_capacity,
                                     s_viewport);
    static s3paper::M5Backend m5;
    static s3paper::RefreshPlanner planner(s_viewport, policy);
    s_frame_arena = &arena;
    s_builder = &builder;
    s_fake = &fake;
    s_m5 = &m5;
    s_planner = &planner;
    s_widgets = new (widgets_mem) s3paper::WidgetArena();
    s_router.Reset();
    s_diff.Reset();
    (void)s_fake->Init();
    ESP_LOGI(kTag,
             "runtime ready: ops=%u arena=%uB trace=%uB widgets=%u "
             "(%u bytes, PSRAM)",
             static_cast<unsigned>(config.op_capacity),
             static_cast<unsigned>(config.arena_capacity),
             static_cast<unsigned>(config.trace_capacity),
             static_cast<unsigned>(s3paper::WidgetArena::kCapacity),
             static_cast<unsigned>(sizeof(s3paper::WidgetArena)));
}

void RuntimeInit() { RuntimeInit(RuntimeConfig{}); }

// ---- Low-level frame hooks ----

s3paper::FrameBuilder &FrameBuilderRef() { return *s_builder; }

s3paper::Result<s3paper::RenderFrame> FinishFrame() {
    return s_builder->Finish(s_next_frame_id++);
}

PlannedPresent PresentFramePlanned(const s3paper::RenderFrame &frame,
                                   s3paper::PresentIntent intent,
                                   bool use_m5) {
    return PresentPlanned(frame, intent, use_m5);
}

s3paper::RefreshPlanner &Planner() { return *s_planner; }

// ---- Backends ----

s3paper::Status EnsureM5Init() {
    if (s_m5 == nullptr) {
        return s3paper::ErrStatus(s3paper::StatusCode::Busy);
    }
    return s_m5->Init();
}

bool ReadM5Touch(s3paper::PointerSample *out) {
    return s_m5 != nullptr && s_m5->ReadTouch(out);
}

s3paper::BackendState FakeBackendState() {
    return s_fake ? s_fake->GetState() : s3paper::BackendState{};
}

s3paper::BackendState M5BackendState() {
    return s_m5 ? s_m5->GetState() : s3paper::BackendState{};
}

const char *FakeTrace() {
    return s_fake != nullptr ? s_fake->trace() : "";
}

void PrintFakeTrace() {
    if (s_fake == nullptr) {
        return;
    }
    printf("%s", s_fake->trace());
    if (s_fake->trace_truncated()) {
        printf("(trace truncated at %u bytes)\n",
               static_cast<unsigned>(s_fake->trace_len()));
    }
}

// ---- Widget pipeline ----

s3paper::WidgetArena &Arena() {
    RuntimeInit();
    return *s_widgets;
}

s3paper::PageRouter &Router() { return s_router; }

PresentPageResult PresentPage(const s3paper::PageSlots &slots,
                              s3paper::PresentIntent intent,
                              bool screen_change, s3paper::HitRegion *hits,
                              uint32_t hit_cap, ExtraOps extra_ops) {
    RuntimeInit();
    PresentPageResult out{StatusCode::Busy, 0, false};
    s3paper::FrameBuilder &fb = *s_builder;
    fb.Begin();
    s3paper::Status st = fb.FillRect(s_page_bounds, 255);
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
        s_planner->NoteScreenChange();
    }
    const PlannedPresent presented =
        PresentPlanned(frame.value, intent, !s_trace_present);
    out.status = presented.present.status;
    out.full_refresh = presented.plan.full_refresh;
    if (out.status == StatusCode::Ok) {
        s_diff.Capture(*s_widgets, s_entries, laid.value);
        s_last_slots = slots;
        s_last_extra = extra_ops;
        s_last_valid = true;
        s_present_count++;
    }
    return out;
}

PresentPageResult PresentPageUpdate(const s3paper::PageSlots &slots,
                                    s3paper::HitRegion *hits,
                                    uint32_t hit_cap, ExtraOps extra_ops) {
    RuntimeInit();
    if (!s_last_valid) {
        return PresentPage(slots, s3paper::PresentIntent::TextPage, false,
                           hits, hit_cap, extra_ops);
    }
    // Layout first: the diff compares new frames + content versions
    // against the capture of the last present.
    const s3paper::Result<uint32_t> laid = s3paper::LayoutPage(
        *s_widgets, slots, s_page_bounds, s_entries,
        s3paper::WidgetArena::kCapacity);
    if (!laid.ok()) {
        PresentPageResult out{laid.code, 0, false};
        return out;
    }
    s3paper::Rect damage[16];
    const s3paper::Result<uint32_t> rects =
        s_diff.Diff(*s_widgets, s_entries, laid.value, damage, 16);
    if (!rects.ok()) {
        // More damage than the budget: one full-tree partial is cheaper
        // and simpler than many rects.
        return PresentPage(slots, s3paper::PresentIntent::TextPage, false,
                           hits, hit_cap, extra_ops);
    }
    PresentPageResult out{StatusCode::Ok, 0, false};
    if (rects.value == 0) {
        return out;  // nothing visible changed: no EPD work at all
    }
    s3paper::Rect clip = damage[0];
    for (uint32_t i = 1; i < rects.value; ++i) {
        const s3paper::Result<s3paper::Rect> u = Union(clip, damage[i]);
        if (u.ok()) {
            clip = u.value;
        }
    }
    s3paper::FrameBuilder &fb = *s_builder;
    fb.Begin();
    if (!fb.PushClip(clip).ok()) {
        out.status = StatusCode::CapacityExceeded;
        return out;
    }
    (void)fb.FillRect(s_page_bounds, 255);
    // Hit regions are NOT propagated: compiled under the damage clip they
    // would shrink to the clip, so the caller keeps its previous hits.
    // CompileTree still needs an output array (hit nodes are an error
    // otherwise); this scratch is discarded.
    (void)hits;
    (void)hit_cap;
    s3paper::HitRegion hit_scratch[64];
    uint32_t hit_scratch_count = 0;
    const s3paper::Result<uint32_t> built =
        BuildPageOps(slots, hit_scratch, 64, &hit_scratch_count);
    out.hit_count = 0;
    if (!built.ok()) {
        (void)fb.PopClip();
        out.status = built.code;
        return out;
    }
    if (extra_ops != nullptr) {
        const StatusCode extra = extra_ops(fb, s_entries, built.value);
        if (extra != StatusCode::Ok) {
            (void)fb.PopClip();
            out.status = extra;
            return out;
        }
    }
    (void)fb.PopClip();
    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) {
        out.status = frame.code;
        return out;
    }
    const PlannedPresent presented = PresentPlanned(
        frame.value, s3paper::PresentIntent::TextRegion, !s_trace_present);
    out.status = presented.present.status;
    out.full_refresh = presented.plan.full_refresh;
    if (out.status == StatusCode::Ok) {
        s_diff.Capture(*s_widgets, s_entries, built.value);
        s_last_slots = slots;
        s_last_extra = extra_ops;
        s_present_count++;
        ESP_LOGI(kTag, "update present: %u rect(s), damage %dx%d at %d,%d",
                 static_cast<unsigned>(rects.value),
                 static_cast<int>(frame.value.damage.w),
                 static_cast<int>(frame.value.damage.h),
                 static_cast<int>(frame.value.damage.x),
                 static_cast<int>(frame.value.damage.y));
    }
    return out;
}

uint32_t PresentCount() { return s_present_count; }

void SetTracePresent(bool enabled) { s_trace_present = enabled; }

const s3paper::RegionSpec *FindRegion(uint32_t region_id) {
    return s_regions.Find(region_id);
}

}  // namespace s3paper_runtime
