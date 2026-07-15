// Display service (Phase 2). Owner-task-only: every function here must be
// called from the UI owner task; backends and frame storage are application
// state.
//
// The primary development backend is the deterministic fake; the M5 backend
// is the transaction shell behind the same boundary, initialized lazily on
// first use.
#pragma once

#include "s3paper/display_backend.h"

namespace reader {

// Allocates frame storage (PSRAM) and initializes the fake backend.
void DisplayServiceInit();

// Builds the deterministic Phase 2 primitive fixture and presents it.
// use_m5 selects the backend; the M5 backend is initialized on first use.
s3paper::PresentResult RunFixture(bool use_m5);

// Prints the fake backend's trace of the last present via printf.
void PrintFakeTrace();

s3paper::BackendState FakeBackendState();
s3paper::BackendState M5BackendState();

}  // namespace reader
