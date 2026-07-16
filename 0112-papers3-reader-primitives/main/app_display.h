// Display service (Phase 2). Owner-task-only: every function here must be
// called from the UI owner task; backends and frame storage are application
// state.
//
// The primary development backend is the deterministic fake; the M5 backend
// is the transaction shell behind the same boundary, initialized lazily on
// first use.
#pragma once

#include "s3paper/display_backend.h"
#include "s3paper/frame_builder.h"
#include "s3paper/input.h"
#include "s3paper/refresh_planner.h"

namespace reader {

// A present that went through the refresh planner: backend result plus the
// plan that produced it.
struct PlannedPresent {
    s3paper::PresentResult present;
    s3paper::RefreshPlan plan;
};

// Allocates frame storage (PSRAM) and initializes the fake backend.
void DisplayServiceInit();

// Builds the deterministic Phase 2 primitive fixture and presents it via the
// refresh planner. use_m5 selects the backend; M5 initializes on first use.
PlannedPresent RunFixture(bool use_m5);

// One soak-step: builds a small deterministic step frame (varying region,
// cycling intent), plans it, and presents through the M5 backend.
PlannedPresent RunSoakStep(uint32_t step_index);

// Renders a measured body-text page (Phase 5 fixture: decode -> paragraph
// split -> measured line breaking -> GlyphRun ops) through a backend.
PlannedPresent RunTextFixture(bool use_m5);

// Prints the fake backend's trace of the last present via printf.
void PrintFakeTrace();

// The fake backend's normalized trace of the last fake present ("" when
// unavailable). Owner-task-only; valid until the next fake present.
const char *FakeTrace();

s3paper::BackendState FakeBackendState();
s3paper::BackendState M5BackendState();

// The single refresh planner (owner-task-only, like everything here).
s3paper::RefreshPlanner &Planner();

// Frame-building hooks for owner-side controllers (owner-task-only).
s3paper::FrameBuilder &FrameBuilderRef();
s3paper::Result<s3paper::RenderFrame> FinishFrame();
PlannedPresent PresentFramePlanned(const s3paper::RenderFrame &frame,
                                   s3paper::PresentIntent intent, bool use_m5);

// Initializes the M5 backend if it isn't yet (owner-task-only).
s3paper::Status EnsureM5Init();

// Polls the touch controller via the M5 backend (owner-task-only).
// Returns false when the backend is uninitialized.
bool ReadM5Touch(s3paper::PointerSample *out);

}  // namespace reader
