// Host tests for s3paper_core: geometry overflow/edges, EPD alignment,
// arena capacity/lifetime, builder clipping and capacity, fake-backend
// normalized traces.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "s3paper/fake_backend.h"
#include "s3paper/frame_arena.h"
#include "s3paper/frame_builder.h"
#include "s3paper/content.h"
#include "s3paper/geometry.h"
#include "s3paper/input.h"
#include "s3paper/paginator.h"
#include "s3paper/refresh_planner.h"
#include "s3paper/page.h"
#include "s3paper/region.h"
#include "s3paper/status.h"
#include "s3paper/text.h"
#include "s3paper/widget.h"
#include "s3paper/widget_diff.h"
#include "s3paper/widget_layout.h"
#include "s3paper/widget_render.h"

using namespace s3paper;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                    \
    } while (0)

#define CHECK_RECT(res, ex, ey, ew, eh)                                      \
    do {                                                                     \
        CHECK((res).ok());                                                   \
        CHECK(RectEquals((res).value, Rect{ex, ey, ew, eh}));                \
    } while (0)

static const Size kViewport{540, 960};  // PaperS3 portrait

static void TestGeometryBasics() {
    CHECK(IsEmpty(Rect{0, 0, 0, 10}));
    CHECK(IsEmpty(Rect{0, 0, 10, 0}));
    CHECK(IsEmpty(Rect{5, 5, -1, 10}));
    CHECK(!IsEmpty(Rect{0, 0, 1, 1}));
    CHECK(Area(Rect{0, 0, 540, 960}) == 518400);
    CHECK(Area(Rect{0, 0, -5, 10}) == 0);
    // Half-open membership: right/bottom edges excluded.
    CHECK(Contains(Rect{10, 10, 5, 5}, Point{10, 10}));
    CHECK(Contains(Rect{10, 10, 5, 5}, Point{14, 14}));
    CHECK(!Contains(Rect{10, 10, 5, 5}, Point{15, 10}));
    CHECK(!Contains(Rect{10, 10, 5, 5}, Point{10, 15}));
    CHECK(RectEquals(Normalized(Rect{7, 9, 0, 3}), kEmptyRect));
}

static void TestIntersectUnion() {
    CHECK_RECT(Intersect(Rect{0, 0, 10, 10}, Rect{5, 5, 10, 10}), 5, 5, 5, 5);
    // Disjoint and touching-edge (half-open) intersections are empty.
    CHECK_RECT(Intersect(Rect{0, 0, 10, 10}, Rect{10, 0, 10, 10}), 0, 0, 0, 0);
    CHECK_RECT(Intersect(Rect{0, 0, 10, 10}, Rect{20, 20, 5, 5}), 0, 0, 0, 0);
    CHECK_RECT(Intersect(Rect{0, 0, 10, 10}, kEmptyRect), 0, 0, 0, 0);
    // Negative-origin rects.
    CHECK_RECT(Intersect(Rect{-5, -5, 10, 10}, Rect{0, 0, 10, 10}), 0, 0, 5, 5);
    CHECK_RECT(Union(Rect{0, 0, 1, 1}, Rect{9, 9, 1, 1}), 0, 0, 10, 10);
    CHECK_RECT(Union(kEmptyRect, Rect{3, 4, 5, 6}), 3, 4, 5, 6);
    CHECK_RECT(Union(Rect{3, 4, 5, 6}, kEmptyRect), 3, 4, 5, 6);
}

static void TestOverflow() {
    // Union spanning the full int32 range overflows width: explicit error.
    const Rect min_corner{INT32_MIN, INT32_MIN, 1, 1};
    const Rect max_corner{INT32_MAX - 1, INT32_MAX - 1, 1, 1};
    CHECK(!Union(min_corner, max_corner).ok());
    CHECK(Union(min_corner, max_corner).code == StatusCode::InvalidArgument);
    // Intersect of huge rects stays representable and succeeds.
    const Rect huge{INT32_MIN / 2, INT32_MIN / 2, INT32_MAX, INT32_MAX};
    CHECK(Intersect(huge, Rect{0, 0, 100, 100}).ok());
    // Translate overflow is explicit.
    CHECK(!Translate(Rect{INT32_MAX - 5, 0, 10, 10}, 10, 0).ok());
    CHECK(Translate(Rect{5, 5, 10, 10}, -10, -10).ok());
    // x + w overflowing int32 inside intersect must not wrap.
    const Rect wide{INT32_MAX - 10, 0, 20, 10};
    const Result<Rect> r = Intersect(wide, Rect{INT32_MAX - 5, 0, 3, 3});
    CHECK_RECT(r, INT32_MAX - 5, 0, 3, 3);
}

static void TestClampShrink() {
    CHECK_RECT(ClampTo(Rect{-20, -20, 600, 1000}, kViewport), 0, 0, 540, 960);
    CHECK_RECT(ClampTo(Rect{530, 950, 100, 100}, kViewport), 530, 950, 10, 10);
    CHECK_RECT(ClampTo(Rect{600, 0, 10, 10}, kViewport), 0, 0, 0, 0);
    CHECK_RECT(Shrink(Rect{0, 0, 100, 100}, Insets{10, 10, 10, 10}), 10, 10,
               80, 80);
    // Over-shrink collapses to empty, not negative.
    CHECK_RECT(Shrink(Rect{0, 0, 10, 10}, Insets{6, 6, 6, 6}), 0, 0, 0, 0);
}

static void TestRotation() {
    // 540x960 logical viewport, rect near the top-left corner.
    const Rect r{10, 20, 30, 40};
    CHECK_RECT(RotateInBounds(r, kViewport, 0), 10, 20, 30, 40);
    // 90° CW into a 960x540 physical space: x' = H - y1 = 960-60.
    CHECK_RECT(RotateInBounds(r, kViewport, 1), 900, 10, 40, 30);
    CHECK_RECT(RotateInBounds(r, kViewport, 2), 500, 900, 30, 40);
    CHECK_RECT(RotateInBounds(r, kViewport, 3), 20, 500, 40, 30);
    CHECK(!RotateInBounds(r, kViewport, 4).ok());
    // Full-viewport rect maps onto the full rotated viewport.
    const Rect full{0, 0, 540, 960};
    CHECK_RECT(RotateInBounds(full, kViewport, 1), 0, 0, 960, 540);
    // Rotation round trip: rotating a corner pixel 4x90° returns it.
    const Rect corner{539, 959, 1, 1};
    Rect cur = corner;
    Size bounds = kViewport;
    for (int i = 0; i < 4; ++i) {
        const Result<Rect> rot = RotateInBounds(cur, bounds, 1);
        CHECK(rot.ok());
        cur = rot.value;
        bounds = Size{bounds.h, bounds.w};
    }
    CHECK(RectEquals(cur, corner));
}

static void TestEpdAlignment() {
    // Widths 1..16 at x=13 all align to x=8 with right edge covered.
    for (int32_t w = 1; w <= 16; ++w) {
        const Result<Rect> a =
            AlignDamageForEpd(Rect{13, 100, w, 10}, kViewport, 8);
        CHECK(a.ok());
        CHECK(a.value.x == 8);
        CHECK(a.value.x % 8 == 0);
        CHECK(a.value.x <= 13 && a.value.x + a.value.w >= 13 + w);
        CHECK((a.value.x + a.value.w) % 8 == 0 ||
              a.value.x + a.value.w == kViewport.w);
        CHECK(a.value.y == 100 && a.value.h == 10);
    }
    // Right screen edge: 540 is not a multiple of 8; alignment must clamp.
    const Result<Rect> edge =
        AlignDamageForEpd(Rect{535, 0, 5, 5}, kViewport, 8);
    CHECK(edge.ok());
    CHECK(edge.value.x == 528);
    CHECK(edge.value.x + edge.value.w == 540);
    // Every corner.
    CHECK_RECT(AlignDamageForEpd(Rect{0, 0, 1, 1}, kViewport, 8), 0, 0, 8, 1);
    CHECK_RECT(AlignDamageForEpd(Rect{539, 0, 1, 1}, kViewport, 8), 536, 0, 4,
               1);
    CHECK_RECT(AlignDamageForEpd(Rect{0, 959, 1, 1}, kViewport, 8), 0, 959, 8,
               1);
    CHECK_RECT(AlignDamageForEpd(Rect{539, 959, 1, 1}, kViewport, 8), 536,
               959, 4, 1);
    // Out-of-bounds damage clamps to empty; bad alignment is explicit.
    CHECK_RECT(AlignDamageForEpd(Rect{600, 0, 10, 10}, kViewport, 8), 0, 0, 0,
               0);
    CHECK(!AlignDamageForEpd(Rect{0, 0, 8, 8}, kViewport, 6).ok());
    CHECK(!AlignDamageForEpd(Rect{0, 0, 8, 8}, kViewport, 0).ok());
}

static void TestArena() {
    uint8_t buf[64];
    FrameArena arena(buf, sizeof(buf));
    const Result<uint32_t> a = arena.Alloc(10, 4);
    CHECK(a.ok());
    CHECK(a.value == 0);
    const Result<uint32_t> b = arena.Alloc(1, 4);
    CHECK(b.ok());
    CHECK(b.value == 12);  // aligned past 10
    CHECK(arena.used() == 13);
    // Explicit overflow.
    CHECK(arena.Alloc(64, 1).code == StatusCode::CapacityExceeded);
    // Bad alignment is explicit.
    CHECK(arena.Alloc(4, 3).code == StatusCode::InvalidArgument);
    // PushBytes copies content.
    const char text[] = "hello";
    const Result<uint32_t> c = arena.PushBytes(text, 5, 1);
    CHECK(c.ok());
    CHECK(std::memcmp(arena.Data(c.value), "hello", 5) == 0);
    // Reset invalidates and reuses from zero; high water persists.
    const uint32_t high = arena.high_water();
    arena.Reset();
    CHECK(arena.used() == 0);
    CHECK(arena.high_water() == high);
    CHECK(arena.Alloc(64, 1).ok());
    // Null-buffer arena rejects allocations explicitly.
    FrameArena empty(nullptr, 128);
    CHECK(empty.Alloc(1, 1).code == StatusCode::CapacityExceeded);
}

static void TestBuilderClipping() {
    DrawOp ops[16];
    uint8_t arena_buf[256];
    FrameArena arena(arena_buf, sizeof(arena_buf));
    FrameBuilder fb(ops, 16, &arena, kViewport);
    fb.Begin();
    // Ops clip to the viewport by default.
    CHECK(fb.FillRect(Rect{-20, -20, 40, 40}, 0).ok());
    CHECK(fb.ops_used() == 1);
    CHECK(RectEquals(ops[0].bounds, Rect{0, 0, 20, 20}));
    // Nested clips intersect.
    CHECK(fb.PushClip(Rect{100, 100, 50, 50}).ok());
    CHECK(fb.PushClip(Rect{120, 120, 100, 100}).ok());
    CHECK(RectEquals(fb.CurrentClip(), Rect{120, 120, 30, 30}));
    CHECK(fb.FillRect(Rect{0, 0, 540, 960}, 128).ok());
    CHECK(RectEquals(ops[1].bounds, Rect{120, 120, 30, 30}));
    // Fully clipped ops are dropped and counted.
    CHECK(fb.FillRect(Rect{0, 0, 10, 10}, 0).ok());
    CHECK(fb.ops_dropped_clipped() == 1);
    CHECK(fb.PopClip().ok());
    CHECK(fb.PopClip().ok());
    // Popping the viewport clip is an error.
    CHECK(fb.PopClip().code == StatusCode::InvalidArgument);
    // Lines clip too.
    CHECK(fb.HLine(-10, 5, 30, 0).ok());
    CHECK(RectEquals(ops[2].bounds, Rect{0, 5, 20, 1}));
    CHECK(fb.VLine(5, 950, 30, 0).ok());
    CHECK(RectEquals(ops[3].bounds, Rect{5, 950, 1, 10}));
    // Damage is the union of emitted bounds.
    const Result<RenderFrame> frame = fb.Finish(7);
    CHECK(frame.ok());
    CHECK(frame.value.id == 7);
    CHECK(frame.value.op_count == 4);
    CHECK(RectEquals(frame.value.damage, Rect{0, 0, 150, 960}));
}

static void TestBuilderCapacityAndLifetime() {
    DrawOp ops[2];
    uint8_t arena_buf[16];
    FrameArena arena(arena_buf, sizeof(arena_buf));
    FrameBuilder fb(ops, 2, &arena, kViewport);
    fb.Begin();
    CHECK(fb.FillRect(Rect{0, 0, 1, 1}, 0).ok());
    CHECK(fb.FillRect(Rect{1, 0, 1, 1}, 0).ok());
    // Op capacity exceeded is explicit.
    CHECK(fb.FillRect(Rect{2, 0, 1, 1}, 0).code ==
          StatusCode::CapacityExceeded);
    // Glyph text overflowing the arena is explicit and emits no op.
    fb.Begin();
    CHECK(fb.GlyphRun(Rect{0, 0, 100, 20}, 16, 0, 16,
                      "this text is far too long for the arena", 39, 0)
              .code == StatusCode::CapacityExceeded);
    CHECK(fb.ops_used() == 0);
    // Unbalanced clip at Finish is CorruptData.
    fb.Begin();
    CHECK(fb.PushClip(Rect{0, 0, 10, 10}).ok());
    CHECK(fb.Finish(1).code == StatusCode::CorruptData);
    // Clip stack depth limit is explicit.
    fb.Begin();
    for (uint32_t i = 0; i < FrameBuilder::kMaxClipDepth - 1; ++i) {
        CHECK(fb.PushClip(Rect{0, 0, 540, 960}).ok());
    }
    CHECK(fb.PushClip(Rect{0, 0, 1, 1}).code == StatusCode::CapacityExceeded);
    // Invalid stroke thickness.
    fb.Begin();
    CHECK(fb.StrokeRect(Rect{0, 0, 10, 10}, 0, 0).code ==
          StatusCode::InvalidArgument);
}

static void TestFakeBackendTrace() {
    DrawOp ops[8];
    uint8_t arena_buf[128];
    FrameArena arena(arena_buf, sizeof(arena_buf));
    FrameBuilder fb(ops, 8, &arena, kViewport);
    fb.Begin();
    CHECK(fb.FillRect(Rect{0, 0, 540, 960}, 255).ok());
    CHECK(fb.StrokeRect(Rect{10, 10, 100, 50}, 0, 2).ok());
    CHECK(fb.GlyphRun(Rect{20, 30, 200, 24}, 50, 1, 16, "Hi \"there\"", 10, 0)
              .ok());
    const Result<RenderFrame> frame = fb.Finish(3);
    CHECK(frame.ok());

    char trace_buf[2048];
    FakeBackend backend(trace_buf, sizeof(trace_buf), Size{960, 540});
    // Present before Init is an explicit Busy.
    CHECK(backend.Present(frame.value, PresentIntent::TextPage).status ==
          StatusCode::Busy);
    CHECK(backend.Init().ok());
    const PresentResult r =
        backend.Present(frame.value, PresentIntent::TextPage);
    CHECK(r.status == StatusCode::Ok);
    CHECK(r.ops_drawn == 3);
    CHECK(RectEquals(r.damage, Rect{0, 0, 540, 960}));
    const char *expected =
        "init size=960x540\n"
        "present id=3 intent=TextPage ops=3 damage=0,0,540,960\n"
        "op kind=FillRect gray=255 bounds=0,0,540,960 clip=0,0,540,960\n"
        "op kind=StrokeRect gray=0 bounds=10,10,100,50 clip=0,0,540,960"
        " thickness=2\n"
        "op kind=GlyphRun gray=0 bounds=20,30,200,24 clip=0,0,540,960"
        " baseline=50 font=1 size=16 text=\"Hi \\x22there\\x22\"\n";
    if (std::strcmp(backend.trace(), expected) != 0) {
        g_failures++;
        std::printf("FAIL trace mismatch.\n--- expected ---\n%s--- actual "
                    "---\n%s---\n",
                    expected, backend.trace());
    }
    g_checks++;
    CHECK(!backend.trace_truncated());
    CHECK(backend.GetState().frames_presented == 1);
    // Determinism: the same frame yields the identical trace.
    backend.ClearTrace();
    const PresentResult r2 =
        backend.Present(frame.value, PresentIntent::TextPage);
    CHECK(r2.status == StatusCode::Ok);
    char first[2048];
    std::snprintf(first, sizeof(first), "%s", backend.trace());
    backend.ClearTrace();
    backend.Present(frame.value, PresentIntent::TextPage);
    CHECK(std::strcmp(first, backend.trace()) == 0);
    // Truncation is explicit.
    char tiny[32];
    FakeBackend small(tiny, sizeof(tiny), Size{960, 540});
    CHECK(small.Init().ok());
    small.Present(frame.value, PresentIntent::CleanFull);
    CHECK(small.trace_truncated());
}

static RefreshPlan PresentOnce(RefreshPlanner &planner, PresentIntent intent,
                               int64_t now_us) {
    const RefreshPlan plan = planner.Plan(intent, now_us);
    planner.RecordPresent(plan, now_us);
    return plan;
}

static void TestPlannerDamageMerge() {
    RefreshPlanner planner(kViewport);
    // Damage aligns to x-multiples of 8.
    CHECK(planner.AddDamage(Rect{13, 10, 5, 5}).ok());
    CHECK(planner.pending_damage_count() == 1);
    // Nearby rect (aligned gap 8 < merge_distance 16) merges instead of
    // adding. A gap of exactly merge_distance does NOT merge (half-open).
    CHECK(planner.AddDamage(Rect{36, 10, 5, 5}).ok());
    CHECK(planner.pending_damage_count() == 1);
    // Distant rect occupies a second slot.
    CHECK(planner.AddDamage(Rect{300, 700, 10, 10}).ok());
    CHECK(planner.pending_damage_count() == 2);
    // Off-screen damage is a no-op, not an error.
    CHECK(planner.AddDamage(Rect{600, 0, 10, 10}).ok());
    CHECK(planner.pending_damage_count() == 2);
    // A rect bridging both existing ones cascades into a single merge.
    CHECK(planner.AddDamage(Rect{8, 8, 400, 800}).ok());
    CHECK(planner.pending_damage_count() == 1);
    planner.ClearDamage();
    CHECK(planner.pending_damage_count() == 0);
}

static void TestPlannerCapacityFallback() {
    RefreshPolicy policy;
    policy.merge_distance = 0;  // prevent proximity merging
    RefreshPlanner planner(kViewport, policy);
    // Fill all 8 slots with far-apart 8px rects (merge_distance 0 still
    // merges touching rects, so space them well apart).
    for (uint32_t i = 0; i < RefreshPlanner::kMaxDamageRects; ++i) {
        CHECK(planner.AddDamage(
                     Rect{static_cast<int32_t>(i * 64),
                          static_cast<int32_t>(i * 100), 8, 8})
                  .ok());
    }
    CHECK(planner.pending_damage_count() == RefreshPlanner::kMaxDamageRects);
    CHECK(planner.history().merge_fallbacks == 0);
    // One more distinct rect forces the explicit collapse-to-bounding-box.
    CHECK(planner.AddDamage(Rect{528, 900, 8, 8}).ok());
    CHECK(planner.pending_damage_count() == 1);
    CHECK(planner.history().merge_fallbacks == 1);
}

static void TestPlannerFullTriggers() {
    RefreshPolicy policy;
    policy.max_turns_between_full = 4;
    policy.max_partial_area_between_full = 100000;
    policy.max_elapsed_us_between_full = 1000000;  // 1 s
    RefreshPlanner planner(kViewport, policy);
    int64_t now = 0;

    // First render is always a clean full with Quality waveform.
    CHECK(planner.AddDamage(Rect{0, 0, 8, 8}).ok());
    RefreshPlan plan = PresentOnce(planner, PresentIntent::TextRegion, now);
    CHECK(plan.full_refresh);
    CHECK(plan.reason == RefreshReason::FirstRender);
    CHECK(plan.waveform == EpdWaveform::Quality);
    CHECK(plan.region_count == 1);
    CHECK(RectEquals(plan.regions[0], Rect{0, 0, 540, 960}));

    // Ordinary partials follow intent waveforms.
    CHECK(planner.AddDamage(Rect{0, 0, 8, 8}).ok());
    plan = PresentOnce(planner, PresentIntent::InteractiveInk, now += 1000);
    CHECK(!plan.full_refresh);
    CHECK(plan.reason == RefreshReason::PartialDamage);
    CHECK(plan.waveform == EpdWaveform::Fastest);
    CHECK(plan.aligned_area == 64);

    // Turn budget: with max 4 turns, the 5th partial becomes a full.
    for (int i = 0; i < 3; ++i) {
        CHECK(planner.AddDamage(Rect{0, 0, 8, 8}).ok());
        plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
        CHECK(!plan.full_refresh);
    }
    CHECK(planner.AddDamage(Rect{0, 0, 8, 8}).ok());
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(plan.full_refresh);
    CHECK(plan.reason == RefreshReason::BudgetTurns);
    CHECK(planner.history().turns_since_full == 0);

    // Area budget: one huge partial then a tiny one trips the area check.
    CHECK(planner.AddDamage(Rect{0, 0, 540, 200}).ok());
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(!plan.full_refresh);
    CHECK(planner.AddDamage(Rect{0, 0, 8, 8}).ok());
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(plan.full_refresh);
    CHECK(plan.reason == RefreshReason::BudgetPartialArea);

    // Elapsed budget.
    CHECK(planner.AddDamage(Rect{0, 0, 8, 8}).ok());
    plan = PresentOnce(planner, PresentIntent::TextRegion, now + 2000000);
    CHECK(plan.full_refresh);
    CHECK(plan.reason == RefreshReason::BudgetElapsed);
    now += 2000000;

    // One-shot triggers: wake, screen change, explicit request.
    planner.NoteWake();
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(plan.full_refresh);
    CHECK(plan.reason == RefreshReason::Wake);
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(!plan.full_refresh);  // trigger consumed

    planner.NoteScreenChange();
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(plan.reason == RefreshReason::ScreenChange);

    planner.RequestFull();
    plan = PresentOnce(planner, PresentIntent::TextRegion, now += 1000);
    CHECK(plan.reason == RefreshReason::ExplicitRequest);

    // CleanFull intent is an explicit request too.
    plan = PresentOnce(planner, PresentIntent::CleanFull, now += 1000);
    CHECK(plan.full_refresh);
    CHECK(plan.reason == RefreshReason::ExplicitRequest);

    // History accounting adds up.
    CHECK(planner.history().fulls_total == 8);
    CHECK(planner.history().partials_total == 6);
}

static void TestTouchTransforms() {
    // Consistency with RotateInBounds: a 1x1 logical rect rotated into
    // physical space must map back to its logical origin.
    const Size logical{540, 960};
    const Point samples[] = {{0, 0}, {539, 0}, {0, 959}, {539, 959},
                             {123, 456}};
    for (uint8_t rot = 0; rot < 4; ++rot) {
        const Size physical =
            (rot % 2 == 1) ? Size{logical.h, logical.w} : logical;
        for (const Point &lp : samples) {
            const Result<Rect> pr =
                RotateInBounds(Rect{lp.x, lp.y, 1, 1}, logical, rot);
            CHECK(pr.ok());
            const Result<Point> back = TouchToLogical(
                Point{pr.value.x, pr.value.y}, physical, rot);
            CHECK(back.ok());
            CHECK(back.value.x == lp.x && back.value.y == lp.y);
        }
    }
    // Out-of-range samples and rotations are explicit errors.
    CHECK(!TouchToLogical(Point{540, 0}, Size{540, 960}, 0).ok());
    CHECK(!TouchToLogical(Point{-1, 0}, Size{540, 960}, 0).ok());
    CHECK(!TouchToLogical(Point{0, 0}, Size{540, 960}, 4).ok());
}

static void TestPointerTracker() {
    PointerTracker tracker(100000);  // 100 ms stale timeout
    PointerEvent events[2];
    // Down.
    CHECK(tracker.Feed({{10, 10}, true, 1000}, events) == 1);
    CHECK(events[0].kind == PointerEventKind::Down);
    // Identical position: no duplicate event.
    CHECK(tracker.Feed({{10, 10}, true, 2000}, events) == 0);
    // Moved: one Move.
    CHECK(tracker.Feed({{12, 10}, true, 3000}, events) == 1);
    CHECK(events[0].kind == PointerEventKind::Move);
    // Release: Up at last position.
    CHECK(tracker.Feed({{12, 10}, false, 4000}, events) == 1);
    CHECK(events[0].kind == PointerEventKind::Up);
    CHECK(events[0].pos.x == 12);
    // No-touch idle: nothing.
    CHECK(tracker.Feed({{0, 0}, false, 5000}, events) == 0);
    // Stale sequence: Down, then a gap > timeout with a new touch produces
    // Cancel followed by a fresh Down.
    CHECK(tracker.Feed({{50, 50}, true, 10000}, events) == 1);
    CHECK(tracker.Feed({{60, 60}, true, 200000}, events) == 2);
    CHECK(events[0].kind == PointerEventKind::Cancel);
    CHECK(events[1].kind == PointerEventKind::Down);
    // Stale then no-touch: Cancel only, no Up.
    CHECK(tracker.Feed({{0, 0}, false, 400000}, events) == 1);
    CHECK(events[0].kind == PointerEventKind::Cancel);
}

static uint32_t RunGestureTrace(GestureDetector &detector,
                                const PointerEvent *trace, uint32_t count,
                                GestureEvent *out, uint32_t out_cap) {
    uint32_t emitted = 0;
    for (uint32_t i = 0; i < count; ++i) {
        GestureEvent g;
        if (detector.Feed(trace[i], &g) == 1 && emitted < out_cap) {
            out[emitted++] = g;
        }
    }
    return emitted;
}

static void TestGestures() {
    GestureDetector detector;
    GestureEvent out[4];
    // Tap: quick down/up without movement.
    const PointerEvent tap[] = {
        {PointerEventKind::Down, {100, 100}, 0},
        {PointerEventKind::Up, {102, 101}, 150000},
    };
    CHECK(RunGestureTrace(detector, tap, 2, out, 4) == 1);
    CHECK(out[0].kind == GestureKind::Tap);
    CHECK(out[0].pos.x == 100);
    // Slow release: no tap.
    const PointerEvent slow[] = {
        {PointerEventKind::Down, {100, 100}, 0},
        {PointerEventKind::Up, {100, 100}, 400000},
    };
    CHECK(RunGestureTrace(detector, slow, 2, out, 4) == 0);
    // Swipe right: long horizontal, small off-axis.
    const PointerEvent swipe_r[] = {
        {PointerEventKind::Down, {100, 500}, 0},
        {PointerEventKind::Move, {180, 510}, 100000},
        {PointerEventKind::Up, {220, 520}, 200000},
    };
    CHECK(RunGestureTrace(detector, swipe_r, 3, out, 4) == 1);
    CHECK(out[0].kind == GestureKind::SwipeRight);
    // Swipe up.
    const PointerEvent swipe_u[] = {
        {PointerEventKind::Down, {270, 800}, 0},
        {PointerEventKind::Up, {275, 700}, 200000},
    };
    CHECK(RunGestureTrace(detector, swipe_u, 2, out, 4) == 1);
    CHECK(out[0].kind == GestureKind::SwipeUp);
    // Diagonal movement: rejected (off-axis too large), and no tap either
    // because it moved beyond tap distance.
    const PointerEvent diag[] = {
        {PointerEventKind::Down, {100, 100}, 0},
        {PointerEventKind::Up, {200, 190}, 200000},
    };
    CHECK(RunGestureTrace(detector, diag, 2, out, 4) == 0);
    // Long-press: fires from Update() while held, and swallows the Up.
    GestureEvent g;
    CHECK(detector.Feed({PointerEventKind::Down, {50, 50}, 1000000}, &g) == 0);
    CHECK(detector.Update(1300000, &g) == 0);  // not yet
    CHECK(detector.Update(1700000, &g) == 1);
    CHECK(g.kind == GestureKind::LongPress);
    CHECK(detector.Update(1800000, &g) == 0);  // fires once
    CHECK(detector.Feed({PointerEventKind::Up, {50, 50}, 1900000}, &g) == 0);
    // Cancel suppresses everything.
    CHECK(detector.Feed({PointerEventKind::Down, {50, 50}, 2000000}, &g) == 0);
    CHECK(detector.Feed({PointerEventKind::Cancel, {50, 50}, 2050000}, &g) ==
          0);
    CHECK(detector.Update(9000000, &g) == 0);
}

static void TestHitRegions() {
    const HitRegion regions[] = {
        {{0, 0, 540, 960}, 1, 0},     // background
        {{0, 0, 540, 100}, 2, 1},     // header
        {{440, 0, 100, 100}, 3, 2},   // header button above header
        {{440, 0, 100, 100}, 4, 2},   // same z, later in paint order
    };
    Result<uint32_t> hit = HitTest(regions, 4, Point{10, 500});
    CHECK(hit.ok());
    CHECK(hit.value == 1);
    hit = HitTest(regions, 4, Point{10, 50});
    CHECK(hit.ok());
    CHECK(hit.value == 2);
    // Highest z wins; equal z resolves to the later (paint-order) entry.
    hit = HitTest(regions, 4, Point{450, 50});
    CHECK(hit.ok());
    CHECK(hit.value == 4);
    // Miss is explicit.
    CHECK(!HitTest(regions, 4, Point{-5, -5}).ok());
    CHECK(!HitTest(regions, 0, Point{0, 0}).ok());
}

static void TestScheduler() {
    Scheduler sched;
    CHECK(!sched.NextDue().ok());
    CHECK(sched.Add(1, 1000).ok());
    CHECK(sched.Add(2, 500).ok());
    CHECK(sched.Add(3, 2000).ok());
    CHECK(sched.NextDue().ok());
    CHECK(sched.NextDue().value == 500);
    // Nothing due yet.
    CHECK(!sched.PopDue(400).ok());
    // Due items pop in deadline order.
    Result<uint32_t> due = sched.PopDue(1500);
    CHECK(due.ok());
    CHECK(due.value == 2);
    due = sched.PopDue(1500);
    CHECK(due.ok());
    CHECK(due.value == 1);
    CHECK(!sched.PopDue(1500).ok());
    // Re-adding an id reschedules (quiet/deferred pattern).
    CHECK(sched.Add(3, 5000).ok());
    CHECK(sched.pending() == 1);
    CHECK(!sched.PopDue(2500).ok());
    CHECK(sched.PopDue(5000).value == 3);
    // Cancel and capacity are explicit.
    CHECK(sched.Cancel(99).code == StatusCode::InvalidArgument);
    for (uint32_t i = 0; i < Scheduler::kCapacity; ++i) {
        CHECK(sched.Add(100 + i, 1000 + i).ok());
    }
    CHECK(sched.Add(999, 1).code == StatusCode::CapacityExceeded);
    CHECK(sched.Cancel(100).ok());
    CHECK(sched.Add(999, 1).ok());
}

// Recorded-trace replay: raw samples (as the touch poller would produce)
// through tracker + detector, asserting the exact event/gesture sequence.
static void TestInputReplayFixture() {
    struct Expect {
        PointerEventKind kind;
    };
    const PointerSample trace[] = {
        // A tap.
        {{200, 300}, true, 10000},
        {{200, 300}, true, 30000},
        {{201, 300}, true, 50000},
        {{201, 300}, false, 70000},
        // A left swipe (page turn).
        {{400, 480}, true, 200000},
        {{340, 482}, true, 240000},
        {{280, 484}, true, 280000},
        {{250, 485}, false, 320000},
        // A stale sequence: touch that never got a release sample, then a
        // fresh tap 1s later.
        {{100, 100}, true, 400000},
        {{500, 900}, true, 1500000},
        {{500, 900}, false, 1550000},
    };
    PointerTracker tracker;
    GestureDetector detector;
    GestureEvent gestures[8];
    PointerEvent events[8];
    uint32_t gesture_count = 0;
    uint32_t event_log_count = 0;
    PointerEventKind event_log[16];
    for (const PointerSample &sample : trace) {
        PointerEvent out[2];
        const uint32_t n = tracker.Feed(sample, out);
        for (uint32_t i = 0; i < n; ++i) {
            if (event_log_count < 16) {
                event_log[event_log_count++] = out[i].kind;
            }
            GestureEvent g;
            if (detector.Feed(out[i], &g) == 1 && gesture_count < 8) {
                gestures[gesture_count++] = g;
            }
        }
        (void)events;
    }
    const PointerEventKind expected_events[] = {
        PointerEventKind::Down, PointerEventKind::Move, PointerEventKind::Up,
        PointerEventKind::Down, PointerEventKind::Move, PointerEventKind::Move,
        PointerEventKind::Up,   PointerEventKind::Down,
        PointerEventKind::Cancel, PointerEventKind::Down,
        PointerEventKind::Up,
    };
    CHECK(event_log_count == 11);
    for (uint32_t i = 0; i < 11 && i < event_log_count; ++i) {
        CHECK(event_log[i] == expected_events[i]);
    }
    CHECK(gesture_count == 3);
    CHECK(gestures[0].kind == GestureKind::Tap);
    CHECK(gestures[1].kind == GestureKind::SwipeLeft);
    CHECK(gestures[2].kind == GestureKind::Tap);
}

static void TestUtf8() {
    uint32_t pos = 0, cp = 0;
    // ASCII.
    const char ascii[] = "Ab";
    CHECK(Utf8Next(ascii, 2, &pos, &cp) && cp == 'A' && pos == 1);
    CHECK(Utf8Next(ascii, 2, &pos, &cp) && cp == 'b' && pos == 2);
    CHECK(!Utf8Next(ascii, 2, &pos, &cp));
    // 2-byte (é U+00E9), 3-byte (€ U+20AC), 4-byte (😀 U+1F600).
    const char multi[] = "\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    pos = 0;
    CHECK(Utf8Next(multi, 9, &pos, &cp) && cp == 0xE9 && pos == 2);
    CHECK(Utf8Next(multi, 9, &pos, &cp) && cp == 0x20AC && pos == 5);
    CHECK(Utf8Next(multi, 9, &pos, &cp) && cp == 0x1F600 && pos == 9);
    // Malformed: stray continuation, truncated sequence, overlong, and a
    // surrogate half all yield U+FFFD advancing exactly one byte.
    const char bad1[] = "\x80x";
    pos = 0;
    CHECK(Utf8Next(bad1, 2, &pos, &cp) && cp == kReplacementChar && pos == 1);
    CHECK(Utf8Next(bad1, 2, &pos, &cp) && cp == 'x');
    const char bad2[] = "\xE2\x82";  // truncated €
    pos = 0;
    CHECK(Utf8Next(bad2, 2, &pos, &cp) && cp == kReplacementChar && pos == 1);
    const char overlong[] = "\xC0\xAF";  // overlong '/'
    pos = 0;
    CHECK(Utf8Next(overlong, 2, &pos, &cp) && cp == kReplacementChar);
    const char surrogate[] = "\xED\xA0\x80";  // U+D800
    pos = 0;
    CHECK(Utf8Next(surrogate, 3, &pos, &cp) && cp == kReplacementChar);
}

static void TestFontMetrics() {
    CHECK(GetFont(kFontUi) != nullptr);
    CHECK(GetFont(kFontBody) != nullptr);
    CHECK(GetFont(99) == nullptr);
    // Lowercase coverage (the 0080 reader font had none).
    const Result<GlyphMetrics> a = GetGlyphMetrics(kFontBody, 'a');
    CHECK(a.ok());
    CHECK(!a.value.fallback);
    CHECK(a.value.advance > 0);
    // Body font is larger than UI font.
    const Result<GlyphMetrics> a_ui = GetGlyphMetrics(kFontUi, 'a');
    CHECK(a_ui.ok());
    CHECK(a.value.advance > a_ui.value.advance);
    // é is covered by the PT Serif subset; CJK is the deterministic fallback.
    const Result<GlyphMetrics> accent = GetGlyphMetrics(kFontBody, 0xE9);
    CHECK(accent.ok());
    CHECK(!accent.value.fallback);
    const Result<GlyphMetrics> cjk = GetGlyphMetrics(kFontBody, 0x4E00);
    CHECK(cjk.ok());
    CHECK(cjk.value.fallback);
    CHECK(cjk.value.advance > 0);
    // Measurement sums advances; wider strings measure wider.
    const Result<int32_t> hello =
        MeasureText(kFontBody, "hello", 5);
    const Result<int32_t> hello_w =
        MeasureText(kFontBody, "hello world", 11);
    CHECK(hello.ok() && hello_w.ok());
    CHECK(hello.value > 0);
    CHECK(hello_w.value > hello.value);
    CHECK(MeasureText(kFontBody, "", 0).ok());
    CHECK(MeasureText(kFontBody, "", 0).value == 0);
}

static void TestParagraphs() {
    TextSpan spans[8];
    const char text[] = "first\nsecond\r\nthird\n\nfifth";
    const uint32_t n = SplitParagraphs(text, sizeof(text) - 1, spans, 8);
    CHECK(n == 5);
    CHECK(spans[0].byte_start == 0 && spans[0].byte_len == 5);
    CHECK(spans[1].byte_len == 6);   // "second" (CR stripped)
    CHECK(spans[2].byte_len == 5);   // "third"
    CHECK(spans[3].byte_len == 0);   // empty paragraph
    CHECK(spans[4].byte_len == 5);   // "fifth"
    // Count exceeding cap is reported, extra spans simply unwritten.
    CHECK(SplitParagraphs(text, sizeof(text) - 1, spans, 2) == 5);
}

static void TestLineBreaking() {
    LineSpan lines[32];
    const char para[] =
        "The quick brown fox jumps over the lazy dog and keeps running";
    const uint32_t len = sizeof(para) - 1;
    // Break at 200px in the body font: every line fits, breaks at spaces,
    // and concatenating the spans (plus separators) restores every word.
    const Result<uint32_t> n =
        BreakLines(kFontBody, para, len, 200, lines, 32);
    CHECK(n.ok());
    CHECK(n.value >= 3);
    for (uint32_t i = 0; i < n.value; ++i) {
        CHECK(lines[i].width <= 200);
        CHECK(lines[i].byte_len > 0);
        // No line starts or ends with a space.
        CHECK(para[lines[i].byte_start] != ' ');
        CHECK(para[lines[i].byte_start + lines[i].byte_len - 1] != ' ');
        // Width matches an independent measurement (same metrics source).
        const Result<int32_t> w = MeasureText(
            kFontBody, para + lines[i].byte_start, lines[i].byte_len);
        CHECK(w.ok());
        CHECK(w.value == lines[i].width);
        if (i > 0) {
            CHECK(lines[i].byte_start >
                  lines[i - 1].byte_start + lines[i - 1].byte_len);
        }
    }
    // A word wider than the line hard-breaks at codepoint boundaries.
    const char longword[] = "abc Supercalifragilisticexpialidocious xyz";
    const Result<uint32_t> hard =
        BreakLines(kFontBody, longword, sizeof(longword) - 1, 120, lines, 32);
    CHECK(hard.ok());
    CHECK(hard.value >= 3);
    for (uint32_t i = 0; i < hard.value; ++i) {
        CHECK(lines[i].width <= 120 || lines[i].byte_len <= 4);
    }
    // Multi-byte text never splits inside a UTF-8 sequence: every line
    // boundary decodes cleanly from its own start.
    const char utf8para[] =
        "\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC \xE2\x82\xAC\xE2\x82\xAC "
        "\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC";
    const Result<uint32_t> un =
        BreakLines(kFontBody, utf8para, sizeof(utf8para) - 1, 60, lines, 32);
    CHECK(un.ok());
    for (uint32_t i = 0; i < un.value; ++i) {
        uint32_t pos = 0, cp = 0, count = 0;
        while (Utf8Next(utf8para + lines[i].byte_start, lines[i].byte_len,
                        &pos, &cp)) {
            CHECK(cp != kReplacementChar);
            count++;
        }
        CHECK(count > 0);
    }
    // Empty and capacity cases are explicit.
    CHECK(BreakLines(kFontBody, "", 0, 200, lines, 32).ok());
    CHECK(BreakLines(kFontBody, "", 0, 200, lines, 32).value == 0);
    CHECK(BreakLines(kFontBody, para, len, 200, lines, 1).code ==
          StatusCode::CapacityExceeded);
    CHECK(BreakLines(kFontBody, para, len, 0, lines, 32).code ==
          StatusCode::InvalidArgument);
}

// Golden line breaks for a fixed paragraph (P5.10 host reference).
static void TestGoldenLineBreaks() {
    const char para[] =
        "It was a bright cold day in April, and the clocks were striking "
        "thirteen.";
    LineSpan lines[16];
    const Result<uint32_t> n =
        BreakLines(kFontBody, para, sizeof(para) - 1, 460, lines, 16);
    CHECK(n.ok());
    // Pin the exact segmentation; any metrics or breaker change must be a
    // deliberate golden update. Re-pinned 2026-07-15 for PT Serif 34 px
    // (previous golden was FreeSerif18pt7b bitmap metrics).
    static const char *kExpected[] = {
        "It was a bright cold day in April, and the",
        "clocks were striking thirteen.",
    };
    CHECK(n.value == 2);
    for (uint32_t i = 0; i < n.value && i < 2; ++i) {
        std::string got(para + lines[i].byte_start, lines[i].byte_len);
        if (got != kExpected[i]) {
            g_failures++;
            std::printf("FAIL golden line %u: got \"%s\" want \"%s\"\n", i,
                        got.c_str(), kExpected[i]);
        }
        g_checks++;
    }
}

static LayoutKey MakeTestKey(ContentSource &source) {
    LayoutKey key{};
    key.content = source.Hash().value;
    key.font_id = kFontBody;
    key.viewport_w = 540;
    key.viewport_h = 960;
    key.margin_x = 40;
    key.margin_top = 40;
    key.margin_bottom = 40;
    key.engine_version = kLayoutEngineVersion;
    return key;
}

static std::string MakeTestBook(uint32_t paragraphs) {
    std::string book;
    for (uint32_t i = 0; i < paragraphs; ++i) {
        book += "Paragraph " + std::to_string(i) +
                ": the quick brown fox jumps over the lazy dog while the "
                "clocks strike thirteen and the reader keeps turning pages "
                "without losing a single measured word.";
        book += "\n";
    }
    return book;
}

static void TestContentSource() {
    const std::string book = MakeTestBook(3);
    MemoryContentSource src(book.data(), book.size());
    CHECK(src.Size().value == book.size());
    uint8_t buf[16];
    CHECK(src.ReadAt(0, buf, 16).value == 16);
    CHECK(std::memcmp(buf, "Paragraph 0", 11) == 0);
    // Short read at end; zero read past end.
    CHECK(src.ReadAt(book.size() - 4, buf, 16).value == 4);
    CHECK(src.ReadAt(book.size() + 10, buf, 16).value == 0);
    // Identity changes with content and with size.
    MemoryContentSource other(book.data(), book.size() - 1);
    CHECK(src.Hash().value != other.Hash().value);
}

static void TestPaginatorForward() {
    const std::string book = MakeTestBook(200);  // ~35 KB, several windows
    MemoryContentSource src(book.data(), book.size());
    Paginator paginator(&src, MakeTestKey(src));
    Result<TextLocator> cursor = paginator.Begin();
    CHECK(cursor.ok());
    CHECK(paginator.Validate(cursor.value).ok());

    PageLayout page;
    uint32_t pages = 0;
    uint64_t last_offset = 0;
    uint64_t covered_to = 0;
    while (pages < 500) {
        CHECK(paginator.ComposePage(cursor.value, &page).ok());
        CHECK(page.line_count > 0);
        // Lines are within the content and the viewport, in reading order.
        for (uint32_t i = 0; i < page.line_count; ++i) {
            CHECK(page.lines[i].byte_start + page.lines[i].byte_len <=
                  book.size());
            CHECK(page.lines[i].baseline_y <= 960 - 40);
            if (i > 0) {
                CHECK(page.lines[i].byte_start >
                      page.lines[i - 1].byte_start);
            }
        }
        // Forward progress, no gaps between what pages cover.
        CHECK(page.next.byte_offset > page.start.byte_offset || page.at_end);
        CHECK(page.start.byte_offset >= covered_to ||
              page.start.byte_offset == last_offset);
        covered_to = page.next.byte_offset;
        pages++;
        if (page.at_end) {
            break;
        }
        last_offset = page.next.byte_offset;
        cursor = Result<TextLocator>::Ok(page.next);
    }
    CHECK(page.at_end);
    CHECK(pages > 10);
    CHECK(pages < 500);
    // Progress is monotone and ends at 1000 permille.
    CHECK(paginator.ProgressPermille(page.next).value == 1000);
}

static void TestPaginatorPrevRoundTrip() {
    const std::string book = MakeTestBook(120);
    MemoryContentSource src(book.data(), book.size());
    Paginator paginator(&src, MakeTestKey(src));
    // Walk five pages forward, remembering starts.
    TextLocator starts[6];
    Result<TextLocator> cursor = paginator.Begin();
    CHECK(cursor.ok());
    PageLayout page;
    for (int i = 0; i < 6; ++i) {
        starts[i] = cursor.value;
        CHECK(paginator.ComposePage(cursor.value, &page).ok());
        cursor = Result<TextLocator>::Ok(page.next);
    }
    // Previous from each page start returns exactly the prior start
    // (checkpoints recorded during the forward walk).
    for (int i = 5; i >= 1; --i) {
        const Result<TextLocator> prev =
            paginator.PreviousPageStart(starts[i]);
        CHECK(prev.ok());
        CHECK(prev.value.byte_offset == starts[i - 1].byte_offset);
    }
    // Previous at the beginning stays put.
    const Result<TextLocator> at_start =
        paginator.PreviousPageStart(starts[0]);
    CHECK(at_start.ok());
    CHECK(at_start.value.byte_offset == 0);
    // Previous works without checkpoints too (fresh paginator, cold cache):
    Paginator cold(&src, MakeTestKey(src));
    const Result<TextLocator> cold_prev =
        cold.PreviousPageStart(starts[3]);
    CHECK(cold_prev.ok());
    CHECK(cold_prev.value.byte_offset < starts[3].byte_offset);
    // Composing from the cold result reaches starts[3] exactly.
    PageLayout probe;
    CHECK(cold.ComposePage(cold_prev.value, &probe).ok());
    CHECK(probe.next.byte_offset >= starts[3].byte_offset);
}

static void TestPaginatorEdgeCases() {
    // Empty book: one empty page, at_end immediately.
    MemoryContentSource empty("", 0);
    Paginator pe(&empty, MakeTestKey(empty));
    PageLayout page;
    CHECK(pe.ComposePage(pe.Begin().value, &page).ok() == false ||
          page.at_end);  // compose of empty content must not loop
    // One-line book.
    const char tiny[] = "hello world";
    MemoryContentSource small(tiny, sizeof(tiny) - 1);
    Paginator ps(&small, MakeTestKey(small));
    CHECK(ps.ComposePage(ps.Begin().value, &page).ok());
    CHECK(page.line_count == 1);
    CHECK(page.at_end);
    CHECK(ps.ProgressPermille(page.next).value == 1000);
    // Huge single paragraph (larger than the 8 KiB window): pages stay
    // bounded, progress continues, and every page's lines decode cleanly.
    std::string huge;
    while (huge.size() < 40000) {
        huge += "megaparagraph without any newline separators keeps going ";
    }
    MemoryContentSource hs(huge.data(), huge.size());
    Paginator ph(&hs, MakeTestKey(hs));
    Result<TextLocator> cursor = ph.Begin();
    uint32_t pages = 0;
    while (pages < 200) {
        CHECK(ph.ComposePage(cursor.value, &page).ok());
        pages++;
        if (page.at_end) {
            break;
        }
        CHECK(page.next.byte_offset > page.start.byte_offset);
        cursor = Result<TextLocator>::Ok(page.next);
    }
    CHECK(page.at_end);
    // Malformed UTF-8 content still paginates (replacement glyph metrics).
    std::string bad = "prefix \xFF\xFE\x80 suffix\nmore \xC3 text";
    MemoryContentSource bs(bad.data(), bad.size());
    Paginator pb(&bs, MakeTestKey(bs));
    CHECK(pb.ComposePage(pb.Begin().value, &page).ok());
    CHECK(page.line_count >= 1);
    // Locator validation detects content change.
    Paginator pv(&small, MakeTestKey(small));
    TextLocator loc = pv.Begin().value;
    loc.context_hash ^= 0xdeadbeef;
    CHECK(pv.Validate(loc).code == StatusCode::CorruptData);
}

static void TestUkrainianAndKerning() {
    CHECK(IsTtfFont(kFontBody));
    CHECK(IsTtfFont(kFontUi));
    // Ukrainian-critical codepoints are real glyphs, not fallbacks.
    for (const uint32_t cp : {0x490u, 0x491u, 0x404u, 0x454u, 0x406u,
                              0x456u, 0x407u, 0x457u, 0x2019u}) {
        const Result<GlyphMetrics> m = GetGlyphMetrics(kFontBody, cp);
        CHECK(m.ok());
        CHECK(!m.value.fallback);
        CHECK(m.value.advance > 0);
    }
    // Line metrics are sane for the registered pixel sizes.
    const Result<FontLineMetrics> body = GetFontLineMetrics(kFontBody);
    CHECK(body.ok());
    CHECK(body.value.ascent > 0);
    CHECK(body.value.descent > 0);
    CHECK(body.value.line_height >= body.value.ascent + body.value.descent);
    // Kerning exists and measurement includes it: "AV" measures tighter
    // than the sum of the isolated advances.
    const int32_t kern_av = GetKernAdvance(kFontBody, 'A', 'V');
    CHECK(kern_av < 0);
    const int32_t a = GetGlyphMetrics(kFontBody, 'A').value.advance;
    const int32_t v = GetGlyphMetrics(kFontBody, 'V').value.advance;
    const Result<int32_t> av = MeasureText(kFontBody, "AV", 2);
    CHECK(av.ok());
    CHECK(av.value == a + v + kern_av);
    // Ukrainian pangram: measures, breaks, and every line width matches an
    // independent re-measurement (kerned accumulation == MeasureText).
    const char pangram[] =
        "\xD0\xA7\xD1\x83\xD1\x94\xD1\x88 \xD1\x97\xD1\x85, \xD0\xB4\xD0\xBE"
        "\xD1\x86\xD1\x8E, \xD0\xB3\xD0\xB0? \xD0\x9A\xD1\x83\xD0\xBC\xD0\xB5"
        "\xD0\xB4\xD0\xBD\xD0\xB0 \xD0\xB6 \xD1\x82\xD0\xB8, \xD0\xBF\xD1\x80"
        "\xD0\xBE\xD1\x89\xD0\xB0\xD0\xB9\xD1\x81\xD1\x8F \xD0\xB1\xD0\xB5"
        "\xD0\xB7 \xD2\x91\xD0\xBE\xD0\xBB\xD1\x8C\xD1\x84\xD1\x96\xD0\xB2!";
    LineSpan lines[16];
    const Result<uint32_t> n = BreakLines(kFontBody, pangram,
                                          sizeof(pangram) - 1, 220, lines, 16);
    CHECK(n.ok());
    CHECK(n.value >= 2);
    for (uint32_t i = 0; i < n.value; ++i) {
        CHECK(lines[i].width <= 220);
        const Result<int32_t> w = MeasureText(
            kFontBody, pangram + lines[i].byte_start, lines[i].byte_len);
        CHECK(w.ok());
        CHECK(w.value == lines[i].width);
        // Every line decodes cleanly (no mid-UTF-8 splits).
        uint32_t pos = 0, cp = 0;
        while (Utf8Next(pangram + lines[i].byte_start, lines[i].byte_len,
                        &pos, &cp)) {
            CHECK(cp != kReplacementChar);
        }
    }
    // Rasterization: ghe-with-upturn produces a non-empty coverage bitmap.
    uint8_t buf[4096];
    const Result<GlyphRaster> raster =
        RasterizeGlyph(kFontBody, 0x491, buf, sizeof(buf));
    CHECK(raster.ok());
    CHECK(raster.value.width > 0);
    CHECK(raster.value.height > 0);
    bool any_dark = false;
    for (int32_t i = 0; i < raster.value.width * raster.value.height; ++i) {
        if (buf[i] > 128) {
            any_dark = true;
            break;
        }
    }
    CHECK(any_dark);
}

// ---- Phase 9: widgets, layout, render compilation, diff, pages ----

static void TestWidgetArena() {
    WidgetArena arena;
    Result<WidgetHandle> a = arena.Create(WidgetKind::Row);
    CHECK(a.ok());
    CHECK(arena.Get(a.value) != nullptr);
    CHECK(arena.Get(a.value)->kind == WidgetKind::Row);

    // Stale handle after destroy; new node in the slot is not aliased.
    const WidgetHandle old = a.value;
    CHECK(arena.Destroy(old).ok());
    CHECK(arena.Get(old) == nullptr);
    CHECK(!arena.Destroy(old).ok());
    Result<WidgetHandle> b = arena.Create(WidgetKind::Col);
    CHECK(b.ok());
    CHECK(b.value.index == old.index);
    CHECK(b.value.generation != old.generation);
    CHECK(arena.Get(old) == nullptr);

    // Subtree destroy releases children; AddChild rejects double-linking.
    Result<WidgetHandle> child1 = NewText(arena, "x", kFontUi, 0);
    Result<WidgetHandle> child2 = NewText(arena, "y", kFontUi, 0);
    CHECK(arena.AddChild(b.value, child1.value).ok());
    CHECK(arena.AddChild(b.value, child2.value).ok());
    CHECK(!arena.AddChild(b.value, child1.value).ok());
    CHECK(arena.live_count() == 3);
    CHECK(arena.Destroy(b.value).ok());
    CHECK(arena.live_count() == 0);
    CHECK(arena.Get(child1.value) == nullptr);

    // Capacity is explicit.
    for (uint32_t i = 0; i < WidgetArena::kCapacity; ++i) {
        CHECK(arena.Create(WidgetKind::Spacer).ok());
    }
    CHECK(arena.Create(WidgetKind::Spacer).code ==
          StatusCode::CapacityExceeded);

    // SetText bumps the version only on change.
    arena.Reset();
    Result<WidgetHandle> t = NewText(arena, "hello", kFontUi, 0);
    const uint32_t v0 = arena.Get(t.value)->content_version;
    CHECK(arena.SetText(t.value, "hello").ok());
    CHECK(arena.Get(t.value)->content_version == v0);
    CHECK(arena.SetText(t.value, "world").ok());
    CHECK(arena.Get(t.value)->content_version == v0 + 1);
    CHECK(!arena.SetText(b.value, "nope").ok());  // stale after Reset
}

static void TestWidgetLayoutRow() {
    WidgetArena arena;
    WidgetHandle row = NewRow(arena).value;
    WidgetNode *r = arena.Configure(row);
    r->padding = Insets{5, 5, 5, 10};
    r->gap = 5;
    WidgetHandle a = NewSpacer(arena, 40).value;
    WidgetHandle b = NewSpacer(arena, 0, 1).value;  // flex
    WidgetHandle c = NewDivider(arena, 3, 0).value;
    CHECK(arena.AddChild(row, a).ok());
    CHECK(arena.AddChild(row, b).ok());
    CHECK(arena.AddChild(row, c).ok());

    LayoutEntry entries[16];
    Result<uint32_t> n =
        LayoutTree(arena, row, Rect{0, 0, 200, 50}, entries, 16);
    CHECK(n.ok());
    CHECK(n.value == 4);
    CHECK(RectEquals(entries[0].frame, Rect{0, 0, 200, 50}));
    // content = {10,5,185,40}; A=40, C=3, gaps=10, flex B=132.
    CHECK(RectEquals(entries[1].frame, Rect{10, 5, 40, 40}));
    CHECK(RectEquals(entries[2].frame, Rect{55, 5, 132, 40}));
    CHECK(RectEquals(entries[3].frame, Rect{192, 5, 3, 40}));
    CHECK(entries[1].parent_index == 0);

    // Intrinsic measurement of the row (flex contributes zero).
    Result<Size> m = MeasureWidget(arena, row, false);
    CHECK(m.ok());
    CHECK(m.value.w == 40 + 0 + 3 + 5 * 2 + 15);  // children+gaps+padding
}

static void TestWidgetLayoutAlign() {
    WidgetArena arena;
    WidgetHandle col = NewCol(arena).value;
    arena.Configure(col)->main_align = MainAlign::Center;
    WidgetHandle a = NewSpacer(arena, 20).value;
    WidgetHandle b = NewSpacer(arena, 20).value;
    arena.Configure(b)->fixed_w = 40;
    arena.Configure(col)->cross_align = CrossAlign::Center;
    CHECK(arena.AddChild(col, a).ok());
    CHECK(arena.AddChild(col, b).ok());
    LayoutEntry entries[8];
    Result<uint32_t> n =
        LayoutTree(arena, col, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n.ok());
    // Main centered: leftover 60 -> children start at y=30. Cross Center:
    // a has intrinsic cross 0 -> x=50; b fixed_w 40 -> x=30.
    CHECK(RectEquals(entries[1].frame, Rect{50, 30, 0, 20}));
    CHECK(RectEquals(entries[2].frame, Rect{30, 50, 40, 20}));

    // SpaceBetween pushes children to the edges.
    arena.Configure(col)->main_align = MainAlign::SpaceBetween;
    arena.Configure(col)->cross_align = CrossAlign::Stretch;
    n = LayoutTree(arena, col, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n.ok());
    CHECK(RectEquals(entries[1].frame, Rect{0, 0, 100, 20}));
    // b keeps its fixed_w=40: fixed cross size wins over Stretch.
    CHECK(RectEquals(entries[2].frame, Rect{0, 80, 40, 20}));
}

static void TestWidgetListPagination() {
    WidgetArena arena;
    WidgetHandle list = NewList(arena).value;
    WidgetHandle rows[5];
    for (int i = 0; i < 5; ++i) {
        rows[i] = NewSpacer(arena, 30).value;
        CHECK(arena.AddChild(list, rows[i]).ok());
    }
    LayoutEntry entries[8];
    Result<uint32_t> n =
        LayoutTree(arena, list, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n.ok());
    CHECK(n.value == 4);  // list + 3 fitting children
    CHECK(entries[0].list_shown == 3);
    CHECK(RectEquals(entries[3].frame, Rect{0, 60, 100, 30}));

    // Pagination: skip the first four, the fifth renders at the top.
    CHECK(arena.SetListFirstVisible(list, 4).ok());
    n = LayoutTree(arena, list, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n.ok());
    CHECK(n.value == 2);
    CHECK(entries[0].list_shown == 1);
    CHECK(RectEquals(entries[1].frame, Rect{0, 0, 100, 30}));
    CHECK(entries[1].widget.index == rows[4].index);
}

static void TestWidgetCompile() {
    WidgetArena arena;
    WidgetHandle col = NewCol(arena).value;
    WidgetHandle title = NewText(arena, "Title", kFontUi, 0).value;
    WidgetHandle divider = NewDivider(arena, 2, 128).value;
    WidgetHandle progress = NewProgress(arena, 500, 10, 0).value;
    arena.Configure(progress)->fixed_h = 10;
    arena.Configure(title)->hit_id = 42;
    WidgetHandle region = NewRegion(arena, 7, 60000, true).value;
    arena.Configure(region)->dependency = 9;
    WidgetHandle clock = NewText(arena, "12:00", kFontUi, 0).value;
    CHECK(arena.AddChild(region, clock).ok());
    CHECK(arena.AddChild(col, title).ok());
    CHECK(arena.AddChild(col, divider).ok());
    CHECK(arena.AddChild(col, progress).ok());
    CHECK(arena.AddChild(col, region).ok());

    LayoutEntry entries[16];
    Result<uint32_t> n =
        LayoutTree(arena, col, Rect{0, 0, 540, 200}, entries, 16);
    CHECK(n.ok());

    DrawOp ops[32];
    uint8_t arena_buf[2048];
    FrameArena frame_arena(arena_buf, sizeof(arena_buf));
    FrameBuilder fb(ops, 32, &frame_arena, Size{540, 960});
    fb.Begin();
    HitRegion hits[4];
    RegionSpec regions[4];
    Result<CompileResult> compiled =
        CompileTree(arena, entries, n.value, fb, hits, 4, regions, 4);
    CHECK(compiled.ok());
    CHECK(compiled.value.hit_count == 1);
    CHECK(hits[0].region_id == 42);
    CHECK(compiled.value.region_count == 1);
    CHECK(regions[0].id == 7);
    CHECK(regions[0].dependency == 9);
    CHECK(regions[0].interval_ms == 60000);
    CHECK(regions[0].quiet_while_active);
    // Ops: title glyph run, divider fill, progress stroke+fill, clock run.
    CHECK(fb.ops_used() == 5);
    Result<RenderFrame> frame = fb.Finish(1);
    CHECK(frame.ok());

    // FindFrame exposes reserved rects (Book compositing contract).
    Result<Rect> title_frame = FindFrame(entries, n.value, title);
    CHECK(title_frame.ok());
    CHECK(RectEquals(hits[0].rect, title_frame.value));
}

static void TestWidgetDiff() {
    WidgetArena arena;
    WidgetHandle col = NewCol(arena).value;
    WidgetHandle text = NewText(arena, "before", kFontUi, 0).value;
    arena.Configure(text)->fixed_h = 20;
    WidgetHandle bar = NewProgress(arena, 100, 8, 0).value;
    arena.Configure(bar)->fixed_h = 8;
    CHECK(arena.AddChild(col, text).ok());
    CHECK(arena.AddChild(col, bar).ok());

    LayoutEntry entries[8];
    Result<uint32_t> n =
        LayoutTree(arena, col, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n.ok());

    RenderStateDiff diff;
    diff.Reset();
    diff.Capture(arena, entries, n.value);

    // No change -> no damage.
    Rect damage[8];
    Result<uint32_t> d =
        diff.Diff(arena, entries, n.value, damage, 8);
    CHECK(d.ok());
    CHECK(d.value == 0);

    // Content change -> that node's frame only.
    CHECK(arena.SetProgress(bar, 900).ok());
    d = diff.Diff(arena, entries, n.value, damage, 8);
    CHECK(d.ok());
    CHECK(d.value == 1);
    CHECK(RectEquals(damage[0], entries[2].frame));
    diff.Capture(arena, entries, n.value);

    // Geometry change -> old and new frames.
    arena.Configure(text)->fixed_h = 40;
    Result<uint32_t> n2 =
        LayoutTree(arena, col, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n2.ok());
    d = diff.Diff(arena, entries, n2.value, damage, 8);
    CHECK(d.ok());
    CHECK(d.value >= 2);  // text old+new, bar moved old+new

    // Disappearance -> old frame damage.
    diff.Capture(arena, entries, n2.value);
    CHECK(arena.RemoveChild(col, bar).ok());
    CHECK(!arena.RemoveChild(col, bar).ok());  // already unlinked
    CHECK(arena.Destroy(bar).ok());
    Result<uint32_t> n3 =
        LayoutTree(arena, col, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n3.ok());
    d = diff.Diff(arena, entries, n3.value, damage, 8);
    CHECK(d.ok());
    CHECK(d.value == 1);

    // Tiny caps degrade to an explicit CapacityExceeded, never silence.
    arena.Configure(text)->fixed_h = 60;
    Result<uint32_t> n4 =
        LayoutTree(arena, col, Rect{0, 0, 100, 100}, entries, 8);
    CHECK(n4.ok());
    CHECK(diff.Diff(arena, entries, n4.value, damage, 0).code ==
          StatusCode::CapacityExceeded);
}

static void TestDependencyTracker() {
    WidgetArena arena;
    WidgetHandle a = NewText(arena, "clock", kFontUi, 0).value;
    WidgetHandle b = NewText(arena, "battery", kFontUi, 0).value;
    arena.Configure(a)->dependency = 7;
    arena.Configure(b)->dependency = 8;

    DependencyTracker tracker;
    tracker.Clear();
    CHECK(!tracker.MarkDirty(0).ok());
    CHECK(tracker.MarkDirty(7).ok());
    CHECK(tracker.MarkDirty(7).ok());  // idempotent
    CHECK(tracker.dirty_count() == 1);
    WidgetHandle dirty[4];
    CHECK(tracker.CollectDirtyWidgets(arena, dirty, 4) == 1);
    CHECK(dirty[0].index == a.index);
    tracker.Clear();
    CHECK(tracker.CollectDirtyWidgets(arena, dirty, 4) == 0);
}

static void TestRegionTable() {
    RegionTable table;
    table.Clear();
    CHECK(table.Add(RegionSpec{1, Rect{0, 0, 10, 10}, 7, 60000, true}).ok());
    CHECK(table.Add(RegionSpec{2, Rect{0, 20, 10, 10}, 8, 0, false}).ok());
    // Replacing by id keeps registration order.
    CHECK(table.Add(RegionSpec{1, Rect{0, 0, 20, 20}, 7, 30000, true}).ok());
    CHECK(table.count() == 2);
    CHECK(table.Find(1)->interval_ms == 30000);

    table.Invalidate(8);
    uint32_t ids[4];
    CHECK(table.TakeInvalid(ids, 4) == 1);
    CHECK(ids[0] == 2);
    CHECK(table.TakeInvalid(ids, 4) == 0);  // marks cleared
    CHECK(table.InvalidateRegion(1).ok());
    CHECK(!table.InvalidateRegion(99).ok());
    CHECK(table.TakeInvalid(ids, 4) == 1);
    CHECK(ids[0] == 1);
}

// Golden trace for a small widget page (P9.11): layout -> compile -> fake
// backend. Pins PT Serif 22px metrics like the line-break goldens; a
// mismatch means widget layout, compilation, or font metrics changed.
static void TestWidgetGoldenTrace() {
    WidgetArena arena;
    WidgetHandle header = NewCol(arena).value;
    WidgetNode *hn = arena.Configure(header);
    hn->padding = Insets{10, 20, 4, 20};
    hn->gap = 6;
    CHECK(arena.AddChild(header,
                         NewText(arena, "Golden", kFontUi, 0).value)
              .ok());
    CHECK(arena.AddChild(header, NewDivider(arena, 2, 0).value).ok());
    WidgetHandle content = NewCol(arena).value;
    arena.Configure(content)->padding = Insets{8, 20, 8, 20};
    WidgetHandle bar = NewProgress(arena, 250, 12, 0).value;
    arena.Configure(bar)->fixed_h = 12;
    CHECK(arena.AddChild(content, bar).ok());
    const PageSlots slots{header, content, kNullWidget, kNullWidget};

    LayoutEntry entries[16];
    const Result<uint32_t> n =
        LayoutPage(arena, slots, Rect{0, 0, 540, 960}, entries, 16);
    CHECK(n.ok());

    DrawOp ops[16];
    uint8_t arena_buf[1024];
    FrameArena frame_arena(arena_buf, sizeof(arena_buf));
    FrameBuilder fb(ops, 16, &frame_arena, Size{540, 960});
    fb.Begin();
    HitRegion hits[2];
    RegionSpec regions[2];
    const Result<CompileResult> compiled =
        CompileTree(arena, entries, n.value, fb, hits, 2, regions, 2);
    CHECK(compiled.ok());
    const Result<RenderFrame> frame = fb.Finish(9);
    CHECK(frame.ok());

    char trace_buf[2048];
    FakeBackend backend(trace_buf, sizeof(trace_buf), Size{960, 540});
    CHECK(backend.Init().ok());
    CHECK(backend.Present(frame.value, PresentIntent::TextPage).status ==
          StatusCode::Ok);
    // Pinned 2026-07-15 (PT Serif ui=22px: line_height 22, ascent 17;
    // clips are ancestor frames; progress fill = 496 * 250/1000 = 124).
    const char *expected =
        "init size=960x540\n"
        "present id=9 intent=TextPage ops=4 damage=20,10,500,54\n"
        "op kind=GlyphRun gray=0 bounds=20,10,500,22 clip=0,0,540,44"
        " baseline=27 font=0 size=0 text=\"Golden\"\n"
        "op kind=FillRect gray=0 bounds=20,38,500,2 clip=0,0,540,44\n"
        "op kind=StrokeRect gray=0 bounds=20,52,500,12 clip=0,44,540,916"
        " thickness=1\n"
        "op kind=FillRect gray=0 bounds=22,54,124,8 clip=0,44,540,916\n";
    if (std::strcmp(backend.trace(), expected) != 0) {
        g_failures++;
        std::printf("FAIL widget trace mismatch.\n--- expected ---\n%s--- "
                    "actual ---\n%s---\n",
                    expected, backend.trace());
    }
    g_checks++;
}

static void TestPageRouter() {
    WidgetArena arena;
    // Header/footer with deterministic intrinsic heights.
    WidgetHandle header = NewCol(arena).value;
    CHECK(arena.AddChild(header, NewSpacer(arena, 30).value).ok());
    WidgetHandle footer = NewCol(arena).value;
    CHECK(arena.AddChild(footer, NewSpacer(arena, 20).value).ok());
    WidgetHandle content = NewCol(arena).value;
    WidgetHandle body = NewSpacer(arena, 0, 1).value;
    CHECK(arena.AddChild(content, body).ok());

    PageRouter router;
    router.Reset();
    Result<PageId> reading = router.Register(
        "reading", PageSlots{header, content, footer, kNullWidget});
    Result<PageId> library = router.Register(
        "library", PageSlots{kNullWidget, content, kNullWidget, kNullWidget});
    CHECK(reading.ok());
    CHECK(library.ok());
    CHECK(!router.Current().ok());
    CHECK(router.Push(reading.value).ok());
    CHECK(router.Push(library.value).ok());
    CHECK(router.Push(library.value).ok());  // no duplicate growth
    CHECK(router.stack_depth() == 2);
    CHECK(router.Current().value == library.value);
    Result<PageId> back = router.Back();
    CHECK(back.ok());
    CHECK(back.value == reading.value);
    CHECK(!router.Back().ok());  // bottom of stack
    CHECK(std::strcmp(router.Name(reading.value), "reading") == 0);
    CHECK(!router.Push(99).ok());

    // Slot layout: header 30 on top, footer 20 at bottom, content between.
    LayoutEntry entries[16];
    Result<uint32_t> n = LayoutPage(arena, *router.Slots(reading.value),
                                    Rect{0, 0, 540, 960}, entries, 16);
    CHECK(n.ok());
    Result<Rect> header_frame = FindFrame(entries, n.value, header);
    Result<Rect> content_frame = FindFrame(entries, n.value, content);
    Result<Rect> footer_frame = FindFrame(entries, n.value, footer);
    CHECK(header_frame.ok());
    CHECK(RectEquals(header_frame.value, Rect{0, 0, 540, 30}));
    CHECK(RectEquals(content_frame.value, Rect{0, 30, 540, 910}));
    CHECK(RectEquals(footer_frame.value, Rect{0, 940, 540, 20}));
    // The flexible body fills the content slot.
    Result<Rect> body_frame = FindFrame(entries, n.value, body);
    CHECK(RectEquals(body_frame.value, Rect{0, 30, 540, 910}));
    // Parent indices survive the multi-slot fixup: the footer's spacer
    // points at the footer entry, not at some header-relative index.
    for (uint32_t i = 0; i < n.value; ++i) {
        if (entries[i].parent_index != kNoWidgetIndex) {
            CHECK(entries[i].parent_index < i);
        }
    }
}

// ---- Phase 13: deterministic fuzz/regression corpora (task 666s) ----

static uint32_t g_fuzz_state = 0x2026'0715;
static uint32_t FuzzNext() {
    // xorshift32: deterministic across runs, no seed-time dependency.
    uint32_t x = g_fuzz_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_fuzz_state = x;
    return x;
}

// Malformed UTF-8 through the whole text pipeline: decode, measure,
// break lines. ASan/UBSan enforce memory safety; the checks enforce the
// documented contracts (progress guaranteed, byte-accurate spans).
static void TestFuzzMalformedUtf8() {
    for (int round = 0; round < 400; ++round) {
        char buf[128];
        const uint32_t len = FuzzNext() % sizeof(buf);
        for (uint32_t i = 0; i < len; ++i) {
            buf[i] = static_cast<char>(FuzzNext() & 0xFF);
        }
        // Decoder: always advances, never past the end.
        uint32_t pos = 0;
        uint32_t cp = 0;
        uint32_t steps = 0;
        while (Utf8Next(buf, len, &pos, &cp)) {
            CHECK(pos <= len);
            steps++;
            if (steps > len + 1) {
                break;  // progress violated; the CHECK below fails loudly
            }
        }
        CHECK(steps <= len);
        CHECK(pos == len);
        // Measurement and line breaking must stay in-bounds and agree
        // with the span invariants on garbage input.
        const Result<int32_t> w = MeasureText(kFontBody, buf, len);
        CHECK(w.ok());
        LineSpan lines[32];
        const Result<uint32_t> broken =
            BreakLines(kFontBody, buf, len, 300, lines, 32);
        if (broken.ok()) {
            for (uint32_t i = 0; i < broken.value; ++i) {
                CHECK(lines[i].byte_start <= len);
                CHECK(lines[i].byte_start + lines[i].byte_len <= len);
            }
        }
    }
}

// Random content through the paginator: compose forward to the end, then
// walk backward; every locator must validate and re-compose identically.
static void TestFuzzPaginatorRoundTrip() {
    for (int round = 0; round < 12; ++round) {
        static char content[4096];
        const uint32_t len = 512 + FuzzNext() % (sizeof(content) - 512);
        for (uint32_t i = 0; i < len; ++i) {
            const uint32_t r = FuzzNext() % 100;
            if (r < 15) {
                content[i] = ' ';
            } else if (r < 18) {
                content[i] = '\n';
            } else if (r < 22) {
                content[i] = static_cast<char>(0x80 + (FuzzNext() % 0x40));
            } else {
                content[i] = static_cast<char>('a' + (FuzzNext() % 26));
            }
        }
        MemoryContentSource source(content, len);
        LayoutKey key{};
        key.content = source.Hash().value;
        key.font_id = kFontBody;
        key.viewport_w = 540;
        key.viewport_h = 960;
        key.margin_x = 40;
        key.margin_top = 72;
        key.margin_bottom = 56;
        key.engine_version = kLayoutEngineVersion;
        Paginator paginator(&source, key);
        TextLocator at = paginator.Begin().value;
        PageLayout page;
        uint64_t starts[64];
        uint32_t pages = 0;
        while (pages < 64) {
            CHECK(paginator.ComposePage(at, &page).ok());
            starts[pages++] = at.byte_offset;
            if (page.at_end) {
                break;
            }
            CHECK(page.next.byte_offset > at.byte_offset);
            at = page.next;
        }
        CHECK(pages >= 1);
        // Backward walk must land exactly on the recorded page starts.
        for (uint32_t p = pages; p-- > 1;) {
            TextLocator here{};
            here.byte_offset = starts[p];
            const Result<TextLocator> prev =
                paginator.PreviousPageStart(here);
            if (!prev.ok()) {
                CHECK(prev.ok());
                break;
            }
            CHECK(prev.value.byte_offset == starts[p - 1]);
        }
    }
}

// Random widget-tree operations against arena invariants.
static void TestFuzzWidgetOps() {
    WidgetArena arena;
    WidgetHandle live[64];
    uint32_t live_count = 0;
    for (int step = 0; step < 4000; ++step) {
        const uint32_t op = FuzzNext() % 100;
        if (op < 45 || live_count == 0) {
            const WidgetKind kind =
                static_cast<WidgetKind>(FuzzNext() % 9);
            const Result<WidgetHandle> h = arena.Create(kind);
            if (h.ok() && live_count < 64) {
                live[live_count++] = h.value;
            }
        } else if (op < 70 && live_count >= 2) {
            const uint32_t p = FuzzNext() % live_count;
            const uint32_t c = FuzzNext() % live_count;
            (void)arena.AddChild(live[p], live[c]);  // may legally fail
        } else if (op < 85) {
            const uint32_t i = FuzzNext() % live_count;
            // Destroy may cascade to children still in `live`; stale
            // handles afterwards must fail cleanly, never crash.
            (void)arena.Destroy(live[i]);
            live[i] = live[--live_count];
        } else {
            // Layout of a random live root must never crash even with
            // odd trees (cycles are impossible: AddChild rejects linked
            // children, but diamond attempts were attempted above).
            const uint32_t i = FuzzNext() % live_count;
            LayoutEntry entries[WidgetArena::kCapacity];
            (void)LayoutTree(arena, live[i], Rect{0, 0, 540, 960}, entries,
                             WidgetArena::kCapacity);
        }
        CHECK(arena.live_count() <= WidgetArena::kCapacity);
    }
    arena.Reset();
    CHECK(arena.live_count() == 0);
}

int main() {
    // Register the embedded TTF exactly as the firmware does (same file,
    // same pixel sizes) so pagination goldens match the device.
    static std::vector<uint8_t> font_bytes;
    {
        FILE *f = fopen("../../fonts/PTSerifUkr.ttf", "rb");
        if (f == nullptr) {
            std::printf("FAIL: cannot open fonts/PTSerifUkr.ttf\n");
            return 1;
        }
        fseek(f, 0, SEEK_END);
        font_bytes.resize(static_cast<size_t>(ftell(f)));
        fseek(f, 0, SEEK_SET);
        if (fread(font_bytes.data(), 1, font_bytes.size(), f) !=
            font_bytes.size()) {
            std::printf("FAIL: short read of PTSerifUkr.ttf\n");
            fclose(f);
            return 1;
        }
        fclose(f);
    }
    if (!RegisterTtfFont(kFontUi, font_bytes.data(),
                         static_cast<uint32_t>(font_bytes.size()), 22)
             .ok() ||
        !RegisterTtfFont(kFontBody, font_bytes.data(),
                         static_cast<uint32_t>(font_bytes.size()), 34)
             .ok()) {
        std::printf("FAIL: TTF registration failed\n");
        return 1;
    }

    TestGeometryBasics();
    TestIntersectUnion();
    TestOverflow();
    TestClampShrink();
    TestRotation();
    TestEpdAlignment();
    TestArena();
    TestBuilderClipping();
    TestBuilderCapacityAndLifetime();
    TestFakeBackendTrace();
    TestPlannerDamageMerge();
    TestPlannerCapacityFallback();
    TestPlannerFullTriggers();
    TestTouchTransforms();
    TestPointerTracker();
    TestGestures();
    TestHitRegions();
    TestScheduler();
    TestInputReplayFixture();
    TestUtf8();
    TestFontMetrics();
    TestParagraphs();
    TestLineBreaking();
    TestGoldenLineBreaks();
    TestUkrainianAndKerning();
    TestContentSource();
    TestPaginatorForward();
    TestPaginatorPrevRoundTrip();
    TestPaginatorEdgeCases();
    TestWidgetArena();
    TestWidgetLayoutRow();
    TestWidgetLayoutAlign();
    TestWidgetListPagination();
    TestWidgetCompile();
    TestWidgetDiff();
    TestDependencyTracker();
    TestRegionTable();
    TestWidgetGoldenTrace();
    TestPageRouter();
    TestFuzzMalformedUtf8();
    TestFuzzPaginatorRoundTrip();
    TestFuzzWidgetOps();
    std::printf("%s: %d checks, %d failures\n",
                g_failures == 0 ? "PASS" : "FAIL", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
