// Embedded web server (ESP-53 P5): JS-defined routes + static files.
//
// The hard part is the cross-task handoff (intern guide §6.2): httpd
// invokes handlers on ITS OWN task, JS must run on the owner. For a JS
// route the httpd task claims the single request slot, fills it, posts
// ModuleDone{Serve}, and blocks on the response semaphore (5 s). The
// owner dispatches the route callback; serve.text()/json()/status()
// write the response slot; the owner gives the semaphore. Timeouts on
// both sides: the semaphore protects httpd from a wedged owner, the JS
// deadline protects the owner from a wedged route.
//
// Static files bypass JS entirely: the httpd task streams from the
// mounted directory itself. This is the ONE sanctioned off-owner storage
// read (VFS file reads only — never state files).
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "app_events.h"

namespace pulp {

constexpr uint32_t kServeMaxRoutes = 8;
constexpr uint32_t kServeMaxPath = 64;
constexpr uint32_t kServeMaxQuery = 256;
constexpr uint32_t kServeMaxBody = 4096;  // request and response caps

// ---- owner-only API ----

// Registers an exact-match GET route bound to a rooted JS callback id.
StatusCode ServeRouteAdd(const char *path, int32_t cb_id);
// Clears every JS route (resetTree: the callback ids just died).
void ServeRoutesClear();
// Mounts a static directory (v1: one mount, url_prefix must be "/").
// Writes a default index.html into dir when none exists.
StatusCode ServeFilesMount(const char *url_prefix, const char *dir);
StatusCode ServeStart(uint16_t port);
StatusCode ServeStop();
bool ServeRunning();
// "http://<ip>:<port>" or "" (needs wifi up + server running).
void ServeUrl(char *out, size_t cap);

// Owner-loop dispatch for ModuleDone{Serve}: runs the route callback and
// releases the waiting httpd task.
void ServeOwnerDispatch(int32_t route_index, int32_t gen);

// Response writers used by the serve.text/json/status bindings; only
// valid while a dispatch is active and the generation matches (a late
// write after an httpd timeout is dropped).
bool ServeRespond(int32_t status, uint8_t content_type, const char *body,
                  uint32_t len);

// Request accessors for route handlers (valid during dispatch).
const char *ServeRequestQuery();
const char *ServeRequestUri();

struct ServeSnapshot;
void FillServeSnapshot(ServeSnapshot *out);

// ESP-55 P5: name of the last /apps/upload (httpd task writes, owner
// reads after the completion; same relaxed discipline as the images
// upload result). Empty string before any upload.
const char *ServeAppsUploadName();

}  // namespace pulp
