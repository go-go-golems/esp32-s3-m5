// RFC 8628 device authorization service (ESP-54).
//
// The owner task owns all state transitions and accessors. A short-lived HTTP
// worker writes one bounded response mailbox and posts ModuleDone{Auth}; it
// never touches JS or widgets. Device/access tokens are RAM-only and never
// exposed to JavaScript or console snapshots.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "app_events.h"

namespace pulp {

enum class AuthState : uint8_t {
    Unconfigured = 0,
    Idle,
    RequestingCode,
    WaitingForUser,
    PollingToken,
    Authorized,
    Expired,
    Error,
};

StatusCode AuthConfigure(const char *issuer, const char *client_id,
                         const char *scopes, const char *resource);
StatusCode AuthStart();
void AuthTick(int64_t now_us);
void AuthOwnerOnModuleDone(int32_t kind, int32_t value, int32_t err);
void AuthClear();

int32_t AuthStatus();
const char *AuthStateName();
const char *AuthUserCode();
const char *AuthVerificationUri();
const char *AuthVerificationUriComplete();
const char *AuthErrorName();
int32_t AuthGrantSecondsLeft();
int32_t AuthTokenSecondsLeft();
int32_t AuthPollSecondsLeft();
bool AuthAuthorized();

// Copies "Bearer <opaque>" only for the configured API origin/path.
StatusCode AuthCopyAuthorization(const char *url, char *out, size_t cap);
bool AuthTrustedApiUrl(const char *url);
bool AuthTrustedSocketUrl(const char *url);
const char *PulpDemoCACert();

struct AuthSnapshot;
void FillAuthSnapshot(AuthSnapshot *out);

}  // namespace pulp
