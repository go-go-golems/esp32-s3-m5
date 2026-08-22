// SPDX-License-Identifier: MIT
//
// Component version accessors. Declared here so the example, host tests, and
// future integration code can report the linked component revision without
// reaching into build metadata.

#pragma once

namespace gogolem::nfc {

// Opaque version string, e.g. "0.1.0-dev". Stable for the lifetime of a build.
const char* version();
unsigned version_major();
unsigned version_minor();
unsigned version_patch();
const char* version_suffix();

}  // namespace gogolem::nfc
