#pragma once

#include "esp_err.h"

namespace cardcore::meshcore_compat {

// Starts the Arduino runtime exactly once for the temporary MeshCore adapter.
// Native BSP, UI, app model, and storage must not include Arduino headers.
esp_err_t initialize_runtime();

} // namespace cardcore::meshcore_compat
