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
#include "s3paper/geometry.h"
#include "s3paper/input.h"
#include "s3paper/refresh_planner.h"
#include "s3paper/status.h"

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

int main() {
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
    std::printf("%s: %d checks, %d failures\n",
                g_failures == 0 ? "PASS" : "FAIL", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
