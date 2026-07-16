// Native home page (Phase 4 skeleton). Owner-task-only.
//
// Until the JS launcher exists (Phase 5+), boot presents this native page
// through s3paper_runtime so the render pipeline is proven end to end.
#pragma once

#include "app_events.h"

namespace pulp {

// Builds and presents the native home page (CleanFull, screen change).
StatusCode HomeShowNative();

}  // namespace pulp
