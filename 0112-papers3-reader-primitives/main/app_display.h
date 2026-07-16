// Display shim (ESP-51 Phase 3): the present pipeline lives in the shared
// s3paper_runtime component; this header re-exports it under the reader::
// names call sites use, and keeps the 0112-only console fixtures (primitive
// scene, typography page, soak step).
#pragma once

#include "s3paper_runtime/runtime.h"

namespace reader {

using s3paper_runtime::PlannedPresent;

// Initializes the shared runtime (frame storage, backends, fonts).
void DisplayServiceInit();

// Builds the deterministic Phase 2 primitive fixture and presents it via
// the refresh planner. use_m5 selects the backend.
PlannedPresent RunFixture(bool use_m5);

// One soak-step: builds a small deterministic step frame (varying region,
// cycling intent), plans it, and presents through the M5 backend.
PlannedPresent RunSoakStep(uint32_t step_index);

// Renders a measured body-text page (Phase 5 typography fixture).
PlannedPresent RunTextFixture(bool use_m5);

using s3paper_runtime::PrintFakeTrace;
using s3paper_runtime::FakeTrace;
using s3paper_runtime::FakeBackendState;
using s3paper_runtime::M5BackendState;
using s3paper_runtime::Planner;
using s3paper_runtime::FrameBuilderRef;
using s3paper_runtime::FinishFrame;
using s3paper_runtime::PresentFramePlanned;
using s3paper_runtime::EnsureM5Init;
using s3paper_runtime::ReadM5Touch;

}  // namespace reader
