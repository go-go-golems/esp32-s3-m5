// Bounded HTTP(S) fetch module (ESP-53 P4). One request slot; the
// transfer runs on a short-lived worker task (esp_http_client blocks) that
// writes the PSRAM body mailbox and posts ModuleDone{Http} — the worker
// NEVER touches JS, the arena, or storage. Owner-only entry points.
//
// Completion callback shape: fn(kDoneHttp, status, len) — status is the
// HTTP status code (0 on transport failure), len is bytes captured (or a
// negative esp_err on transport failure).
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "app_events.h"

namespace pulp {

constexpr uint32_t kHttpMaxUrl = 256;
constexpr uint32_t kHttpMaxHeaders = 4;
constexpr uint32_t kHttpMaxBody = 32768;      // hard cap for .limit()
constexpr uint32_t kHttpDefaultLimit = 16384;
constexpr uint32_t kHttpMaxLines = 1024;

// Builder mutations (owner). HttpBegin resets the slot with the URL;
// Busy while a request is in flight.
StatusCode HttpBegin(const char *url);
StatusCode HttpHeader(const char *key, const char *value);
StatusCode HttpLimit(uint32_t limit);
// Attach the RAM-only device bearer after trusted API origin/path checks.
StatusCode HttpBearer();
// Validates and launches the worker. The Http module callback must
// already be registered by the binding.
StatusCode HttpSend();
// Requests an early stop; the in-flight worker still posts its
// completion (with whatever was captured).
void HttpAbort();

bool HttpInFlight();

// Response mailbox accessors (valid after completion).
int32_t HttpStatusCode();
uint32_t HttpLength();                        // captured bytes
const char *HttpBody(uint32_t *len);          // nullptr before first use
uint32_t HttpLineCount();
const char *HttpLine(uint32_t i, uint32_t *len);

struct HttpSnapshot;
void FillHttpSnapshot(HttpSnapshot *out);

}  // namespace pulp
