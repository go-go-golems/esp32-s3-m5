// ESP-55 P3: ROM app assets. The built-in app modules (tools/js/apps/*.js)
// are embedded as NUL-terminated flash text (EMBED_TXTFILES) and served to
// load("rom:<id>") through this registry. Flash text is directly
// JS_Eval-able: no copy, no SD dependency.
#pragma once

#include <stdint.h>

namespace pulp {

// Looks up an embedded app source by id ("dice"). Returns true and fills
// *src/*len (len excludes the trailing NUL) or returns false.
bool AssetsFind(const char *name, const char **src, uint32_t *len);

// Registry size (for the seeding pass and probes).
uint32_t AssetsCount();
const char *AssetsName(uint32_t i);

// ESP-55 P4: owner-side sync writes for seeding and manifests. Both
// resolve vpath through the files sanitizer, create the parent directory
// (one level) and fsync. Return s3paper::StatusCode as int32 (0 = Ok).
int32_t AssetsCopy(const char *name, const char *vpath);
int32_t AssetsWriteText(const char *vpath, const char *body, uint32_t len);

}  // namespace pulp
