---
Title: Device Authorization and Realtime Demo - Analysis Design and Intern Implementation Guide
Ticket: ESP-54-PULP-DEVICE-AUTH
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
    - wifi
    - websocket
    - webserver
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/tiny-idp/pkg/embeddedidp/bootstrap.go
      Note: |-
        Public browser and device client bootstrap API
        Public device-client bootstrap API
    - Path: abs:///home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/tiny-idp/pkg/embeddedidp/provider.go
      Note: |-
        Public embedded provider construction and lifecycle API
        Embedded provider handler and lifecycle
    - Path: repo://0114-papers3-pulp-os/main/app_events.h
      Note: |-
        POD event and ModuleDone contracts that constrain new native services
        POD owner-event and ModuleDone contracts
    - Path: repo://0114-papers3-pulp-os/main/app_js.cpp
      Note: |-
        Owner-only JavaScript callback and resetTree lifecycle rules
        Callback and resetTree lifecycle constraints
    - Path: repo://0114-papers3-pulp-os/main/net_http.cpp
      Note: |-
        Existing bounded HTTPS worker and PSRAM mailbox pattern to reuse
        Existing bounded HTTPS worker and PSRAM mailbox pattern
    - Path: repo://0114-papers3-pulp-os/tools/js/apps/pulp.js
      Note: |-
        Product application where authorization and sensor screens will be added
        Product integration target
    - Path: repo://components/s3paper_core/include/s3paper/widget.h
      Note: |-
        Canvas command limits and retained-tree constraints for the plot
        Canvas capacity and plot primitives
ExternalSources:
    - https://datatracker.ietf.org/doc/html/rfc8628
    - https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html
Summary: Evidence-backed design for adding RFC 8628 device authorization, bearer-authenticated REST and WebSocket clients, an embedded tiny-idp Go demo service, and an e-ink-aware live sensor plot to PULP OS.
LastUpdated: 2026-07-23T20:36:51.97058722-04:00
WhatFor: Implementing ESP-54 without rediscovering PULP ownership constraints, tiny-idp composition, OAuth polling semantics, or e-ink refresh limits.
WhenToUse: Read after the ESP-53 onboarding and connectivity guides, before changing firmware or creating the demo server.
---


# Device Authorization and Realtime Demo — Analysis, Design, and Intern Implementation Guide

## 1. Executive summary

This ticket adds an authenticated client/server demonstration to PULP OS on the M5Stack PaperS3. The PaperS3 will use the OAuth 2.0 Device Authorization Grant (RFC 8628): it asks a Go service for a device code, displays a short user code and browser URL, waits while the user signs in and approves from another device, then receives an access token. The access token is used to call protected REST endpoints and to authenticate a WebSocket handshake. The WebSocket carries synthetic sensor samples that PULP renders as a bounded, slowly refreshed strip chart on the 540×960 e-ink panel.

The Go service will live in `0114-papers3-pulp-os/demo-device-auth-server/`. It will import tiny-idp's supported public embedding packages from the existing checkout; **no tiny-idp source change is required**. The service uses the strict Fosite-backed provider in `embeddedidp.DevMode`, a durable SQLite store, a public device client, a confidential introspection client, and a native `net/http` resource API. Development mode is chosen because the first hardware gate runs over plain HTTP on a trusted LAN. That is a demo deployment, not a production security profile.

On the firmware, protocol credentials remain native. JavaScript can see the user code, verification URL, state, expiry, and sensor messages, but it cannot retrieve the access token. A new native `auth` service owns device-code polling and the bearer credential; `http.bearer()` and `socket.bearer()` ask that service to attach the token only after same-origin/path checks. This avoids turning the MicroQuickJS heap, console, or app logs into bearer-token storage.

The proposed work has four independently testable slices:

1. A Go service that embeds tiny-idp and proves device authorization with `curl`.
2. A native PULP `auth` state machine that completes RFC 8628 on the hardware.
3. Protected REST calls and a bearer-authenticated WebSocket client.
4. A PULP app that drains a bounded sample ring and redraws a 60-point canvas every two seconds, not every network packet.

## 2. Outcome, scope, and definition of done

### 2.1 User-visible outcome

From the PaperS3 launcher, the user opens **SENSOR LINK** and sees one of these screens:

- **Not connected:** a CONNECT action and the configured server address.
- **Approval required:** `ABCD-EFGH`, a short browser URL, and a countdown.
- **Authorized:** subject, token lifetime, latest sensor values, REST-demo results, and a CONNECT STREAM action.
- **Streaming:** a live strip chart, latest temperature/humidity, message sequence, dropped-message count, and socket state.
- **Recoverable failure:** a stable error label with RETRY or REAUTHORIZE.

The browser side is deliberately ordinary: the user opens the displayed URL on a phone or workstation, signs in as the seeded demo user, verifies the code, and approves the requested scopes.

### 2.2 In scope

- RFC 8628 device authorization against tiny-idp's strict embedded provider.
- Native handling of `authorization_pending`, `slow_down`, denial, expiry, transport failures, and success.
- Memory-only access-token ownership on the PaperS3.
- Bearer-protected REST GETs.
- Bearer-protected WebSocket handshake and server-to-device JSON samples.
- A deterministic synthetic sensor source in Go.
- E-ink-aware plotting through the existing canvas and refresh planner.
- Console status commands, device probes, host tests, and a hardware acceptance transcript.
- A development deployment over the local LAN and a documented path to HTTPS/WSS.

### 2.3 Explicit non-goals for v1

- Modifying tiny-idp internals or adding a new grant to tiny-idp.
- Production deployment, Internet exposure, formal security certification, or public DNS automation.
- Refresh tokens or `offline_access`. The public `embeddedidp.DeviceClient` currently permits the device-code grant only (`pkg/embeddedidp/bootstrap.go:41-46,205-214`). The one-hour access token is intentionally followed by reauthorization.
- Persisting bearer, device, or refresh tokens on SD/NVS.
- Browser-origin WebSocket clients; the ESP client can send an `Authorization` header, while browser WebSocket APIs cannot.
- High-frame-rate animation. The network can stream at 2 Hz; the panel is refreshed at 0.5 Hz.
- Arbitrary WebSocket JavaScript callbacks, binary protocols, compression, or client-to-server commands.
- A generic OAuth library in MicroQuickJS.

### 2.4 Definition of done

The ticket is implemented when all of the following are evidenced:

1. The demo service starts from an empty state directory, creates SQLite state, an RSA signing key, a public device client, a protected-resource client, and a seeded user.
2. A complete `curl` device flow returns an audience-bound access token; too-fast polling returns `slow_down`; denial and expiry are demonstrated.
3. The PaperS3 obtains a user code, displays it, waits without blocking touch/console, and becomes authorized after browser approval.
4. The access token never appears in console output, JavaScript-visible accessors, ticket transcripts, or persisted files.
5. At least two protected REST endpoints return 200 with the token and 401/403 without the required credential/scope.
6. `/api/v1/sensors/ws` rejects an unauthenticated handshake and accepts an authorized one.
7. The device receives ordered sensor samples, reports sequence gaps/drops, and plots at least 60 points without exceeding the 96-command canvas cap.
8. A 30-minute stream soak has zero JS exceptions, no owner queue runaway, bounded drops, and flat internal/PSRAM minima after warm-up.
9. Radio, auth, and socket stop cleanly during sleep quiesce.
10. Host tests, firmware build, probes, `docmgr doctor`, and the implementation diary are green.

## 3. System orientation: what already exists

ESP-53 is complete. Do not rebuild networking from scratch. The current firmware already has Wi-Fi, bounded HTTP(S), a device-hosted HTTP server, general SD access, a completion-mailbox event path, a JavaScript product, and the canvas primitives needed for a graph.

### 3.1 Existing layer map

```text
pulp.js product apps
    |
    | MicroQuickJS singleton bindings
    v
main/js_*.cpp -------------------- owner task only
    |
    +-- app_js.cpp callback registry and timed dispatch
    +-- net_wifi.cpp station state machine and credentials
    +-- net_http.cpp bounded HTTP(S) worker + PSRAM body mailbox
    +-- net_serve.cpp local server + owner/httpd handoff
    |
    v
AppEvent queue (fixed POD payloads)
    |
    v
s3paper runtime -> widget diff -> refresh planner -> M5 e-ink backend
```

Observed source facts:

- `main/net_http.cpp:35-53` owns one request slot and a PSRAM response mailbox. The worker performs HTTPS with the certificate bundle at lines 110-147 and posts `ModuleDone` only after the mailbox is complete.
- `main/app_events.h:26-60` defines the POD `ModuleDone` vocabulary. Events carry integers, never strings, pointers, or JS values.
- `main/app_js.cpp:269-290` permits one completion callback per module. `JsModuleDone` clears the callback before invoking it (`526-540`). `resetTree()` cancels pending module delivery (`609-621`).
- The owner is the only JS/UI mutator. `app_owner.cpp:422-438` consumes module events; `451-458` runs JS, buzzer, Wi-Fi, and power ticks.
- `http` currently supports GET only, four 32/64-byte headers, a 32 KiB response cap, a ten-second timeout, and three redirects (`net_http.h:18-22`, `net_http.cpp:21-23`).
- The current app already performs network bring-up through `netUp()` (`pulp.js:765-769`) and parses a bounded JSON response in the Radio app (`976-983`).
- The canvas store allows 96 commands per canvas (`components/s3paper_core/include/s3paper/widget.h:183-191`). Its actual line signature is `line(x0,y0,x1,y1,gray,t?)` (`main/js_widgets.cpp:527-532`).
- The refresh planner defaults to a full refresh after 16 turns, accumulated area, or 15 minutes (`refresh_planner.h:44-53`). `paper.refreshTurns(n)` changes the turn budget (`js_pages.cpp:201-213`).

### 3.2 Constraints that dominate this ticket

#### One owner task

Network callbacks and worker tasks must not call JavaScript, mutate widgets, present a page, or access owner-only storage. They may fill bounded native mailboxes/rings and post POD events. This applies equally to HTTP and WebSocket callbacks.

#### Moving-GC boundary

MicroQuickJS uses a compacting GC. Native code must not retain pointer-valued `JSValue`s. Long-lived callbacks are integer IDs rooted in `__cbs`, and callback arguments are integers. Rich results are exposed through owner-side native accessors.

#### App-switch cancellation

`enter()` calls `resetTree()`. Any ordinary module callback waiting for an async operation disappears. Device authorization can last minutes, so it must be an OS service with native state, not a one-shot JavaScript callback chain.

#### E-ink is not a monitor

A network packet every 500 ms must not imply a panel update every 500 ms. Network ingress and display cadence are separate clocks. The design buffers 2 Hz samples but renders every two seconds, with one damaged chart rectangle and a bounded clean-refresh budget.

#### Internal RAM is scarce

The Wi-Fi/TLS/WebSocket stacks consume internal memory. Message rings, JSON response buffers, and chart history belong in PSRAM or fixed compact POD arrays. Worker stacks must be measured; an arbitrary increase is not a design.

## 4. tiny-idp audit: the server capability already exists

### 4.1 Supported public packages

The embedding boundary is documented in `tiny-idp/docs/embedding-foundations.md`. The demo service should import only:

- `pkg/sqlitestore` — durable identity/protocol state.
- `pkg/idpstore` — client and store contracts.
- `pkg/idpaccounts` — demo user creation and password authentication.
- `pkg/embeddedidp` — client/key bootstrap and provider lifecycle.
- `pkg/idp` — optional policy/audit contracts.

Do not import `internal/fositeadapter`, `internal/authn`, or `cmd/tinyidp-xapp/internal/resourceauth`. Go's `internal` rule forbids the last package from an external demo module anyway. Its source is useful prior art, but the demo owns a small introspection middleware.

### 4.2 Provider composition that has been verified

`embeddedidp.Bootstrap` creates or validates declared clients and creates the first 2048-bit RSA signing key when none exists (`pkg/embeddedidp/bootstrap.go:77-186`). `embeddedidp.DeviceClient` creates a public client with no redirects, explicit scopes, and the device-code grant (`41-46`). `embeddedidp.New` validates options and returns a provider whose `Handler()` can be mounted on a standard `http.ServeMux` (`pkg/embeddedidp/provider.go:43-95`).

The strict provider already mounts:

```text
/idp/.well-known/openid-configuration
/idp/jwks
/idp/authorize
/idp/device_authorization
/idp/device
/idp/token
/idp/userinfo
/idp/introspect
/idp/end-session
/idp/healthz
/idp/readyz
```

The device endpoint implementation has a 10-minute grant TTL and a five-second starting poll interval (`internal/fositeadapter/provider.go:550-564`). It verifies client grant capability, scopes, and resource indicators, persists hashed codes, and returns both verification URIs (`566-676`). The token endpoint uses the ordinary Fosite path and issues bearer tokens (`1296-1352`).

Focused verification run during this investigation:

```text
GOWORK=off go test ./pkg/embeddedidp ./internal/fositeadapter \
  -run 'Device|BootstrapCreatesBrowserDevice' -count=1
ok github.com/go-go-golems/tiny-idp/pkg/embeddedidp       0.053s
ok github.com/go-go-golems/tiny-idp/internal/fositeadapter 5.540s
```

Therefore: **the ESP-54 server consumes tiny-idp; it does not modify tiny-idp**.

### 4.3 Why use strict/Fosite in development mode

The mock engine can demonstrate device flow, but it is not the right composition target. This ticket specifically needs an embeddable Go handler, durable grants, an audience-bound access token, and RFC 7662 introspection for the resource API. Those are already present in the strict embedded provider.

`embeddedidp.DevMode` means:

- a loopback/LAN HTTP issuer is accepted for development;
- production-only audit, TLS-cookie, and production-ready control checks are relaxed;
- the provider remains the strict Fosite-backed implementation;
- SQLite can still be used and should be used.

It does **not** mean the HTTP deployment is safe for untrusted networks.

## 5. Protocol walkthrough: RFC 8628 on this device

### 5.1 Sequence

```text
PaperS3                    Go service / tiny-idp                    Browser/user
   |                                |                                    |
   | POST /idp/device_authorization |                                    |
   | client_id, scope, resource     |                                    |
   |------------------------------->|                                    |
   | device_code, user_code,        |                                    |
   | verification_uri(_complete),   |                                    |
   | expires_in, interval           |                                    |
   |<-------------------------------|                                    |
   |                                |                                    |
   | show URL + ABCD-EFGH           |<----- GET /idp/device?... ----------|
   |                                |<----- login/password + approve ------|
   |                                | durable grant = approved             |
   |                                |                                    |
   | POST /idp/token every interval |                                    |
   | grant_type, client_id, code    |                                    |
   |------------------------------->|                                    |
   | access_token, expires_in, ...  |                                    |
   |<-------------------------------|                                    |
   |                                |                                    |
   | GET /api/v1/me                 |                                    |
   | Authorization: Bearer ...      | introspect -> scopes/audience       |
   |------------------------------->|                                    |
   | principal JSON                 |                                    |
   |<-------------------------------|                                    |
   |                                |                                    |
   | WS /api/v1/sensors/ws          | validate handshake bearer           |
   |===============================>|                                    |
   |<========== sensor JSON ========|                                    |
```

### 5.2 Device authorization request

The firmware sends exactly:

```http
POST /idp/device_authorization HTTP/1.1
Content-Type: application/x-www-form-urlencoded

client_id=pulp-papers3&scope=openid%20profile%20demo.read%20sensors.read&resource=http%3A%2F%2FHOST%3A8787%2Fapi
```

The `resource` parameter follows RFC 8707 and becomes the access-token audience. tiny-idp accepts a legacy `audience` parameter, but mixing `resource` and `audience` is rejected (`provider.go:702-729`). This design uses only `resource`.

### 5.3 Polling rules

The firmware enforces these rules natively, even if JavaScript ticks incorrectly:

- If `interval` is absent, use five seconds.
- Do not poll before `next_poll_us`.
- `authorization_pending`: keep the same interval.
- `slow_down`: add five seconds to the current interval for all later requests.
- Network timeout: increase transport backoff (bounded exponential backoff) without discarding the grant.
- `access_denied`, `expired_token`, `invalid_grant`, invalid client/scope/target: terminal failure.
- Stop at the monotonic deadline derived from `expires_in`, regardless of server response.
- On success, clear `device_code` immediately after installing the token.

RFC 8628 explicitly requires the five-second increase for `slow_down` and recommends backing off after connection timeouts. It also says the `device_code` is not for display. Only `user_code` and the verification URI appear in the UI.

### 5.4 Token response and lifetime

The native parser needs only:

```json
{
  "access_token": "opaque",
  "token_type": "Bearer",
  "expires_in": 3600,
  "scope": "openid profile demo.read sensors.read",
  "id_token": "signed-jwt"
}
```

The ID token is not needed by the v1 device and should be ignored after validating the bounded JSON shape. No refresh token is requested. When `expires_in` reaches zero, REST and WebSocket operations stop and the UI offers REAUTHORIZE.

## 6. Proposed end-to-end architecture

```text
HOST: demo-device-auth-server

  http.Server (:8787, timeouts)
        |
        +-- /idp/* ---------------- embeddedidp.Provider
        |       |                    Fosite strict engine, DevMode
        |       +------------------> SQLite identity.sqlite
        |
        +-- /api/v1/me ------------ bearer middleware --+
        +-- /api/v1/demo/fortune -- bearer middleware --+--> introspection
        +-- /api/v1/sensors/snapshot                    |    through in-process
        +-- /api/v1/sensors/ws ------ bearer handshake -+    issuer transport
                       |
                       +-- sensorHub (one 500 ms producer)
                               |
                               +-- bounded latest-wins subscriber queues

NETWORK: trusted LAN HTTP/WS for first gate

PAPERS3: PULP OS

  auth native service ---- POST forms / JSON ---- /idp
       | owns bearer in RAM
       +--> http.bearer() ----------------------- /api/v1/*
       +--> socket.bearer() --------------------- /api/v1/sensors/ws
                                                     |
  socket worker -> bounded message ring -> JS drains every 2 s
                                                     |
  pulp.js SENSOR LINK app -> canvas.wipe + <= 63 plot lines -> partial EPD
```

## 7. Go demo service design

### 7.1 Location and module layout

```text
0114-papers3-pulp-os/demo-device-auth-server/
  go.mod
  README.md
  cmd/pulp-auth-demo/main.go
  internal/app/app.go
  internal/app/config.go
  internal/identity/identity.go
  internal/identity/resource_client.go
  internal/authn/introspection.go
  internal/httpapi/routes.go
  internal/httpapi/middleware.go
  internal/sensors/hub.go
  internal/sensors/model.go
  internal/sensors/websocket.go
  internal/testutil/deviceflow.go
```

Keep the service native Go. It does not need goja, xgoja, React, or an application database beyond tiny-idp's SQLite store.

For local development, add this module to the existing workspace without touching tiny-idp source:

```bash
cd /home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink
go work use \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/demo-device-auth-server
```

The committed `go.mod` should require a tagged/pseudo-version of `github.com/go-go-golems/tiny-idp`. Do not commit an absolute `replace` path. The local `go.work` supplies the checkout during development.

Use `github.com/coder/websocket` v1.8.15 for the server endpoint. It integrates with `net/http`, context cancellation, and bounded writes. Authentication happens before `websocket.Accept`.

### 7.2 CLI configuration

Minimum flags/environment:

```text
--listen             0.0.0.0:8787
--public-base-url    http://192.168.0.X:8787   REQUIRED; no auto-guess
--state-dir          ./var/pulp-auth-demo
--demo-login         alice
--demo-password-file ./var/demo-password       preferred over argv
--sensor-interval    500ms
--log-level          info
```

`public-base-url` is authoritative. It determines:

```text
issuer   = PUBLIC_BASE_URL + /idp
audience = PUBLIC_BASE_URL + /api
ws URL   = ws(s)://same-host/api/v1/sensors/ws
```

Do not derive the issuer from `r.Host`: issuer drift breaks OIDC and audience checks.

### 7.3 Durable startup composition

Pseudocode:

```go
func New(ctx context.Context, cfg Config) (*Application, error) {
    ensureMode0700(cfg.StateDir)

    store := sqlitestore.Open(ctx, DefaultConfig(state/identity.sqlite))
    accounts := idpaccounts.NewService(store, idpaccounts.Options{})
    ensureDemoUser(accounts, cfg.Login, readPasswordFile())

    device := embeddedidp.DeviceClient("pulp-papers3", []string{
        "openid", "profile", "demo.read", "sensors.read",
    })
    device.Client.AllowedAudiences = []string{cfg.Audience()}

    embeddedidp.Bootstrap(ctx, store, BootstrapConfig{
        Mode: DevMode,
        Clients: []ClientSpec{device},
        SigningKeyID: "pulp-demo-rs256-1",
    })

    secret := loadOrCreate0600(state/secrets/introspection.key, 32)
    ensureIntrospectionClient(store, secret, cfg.Audience())

    tokenKey := loadOrCreate0600(state/secrets/token.key, 32)
    provider := embeddedidp.New(ctx, Options{
        Issuer: cfg.Issuer(), Mode: DevMode, Store: store,
        Authenticator: accounts,
        Token: TokenConfig{SecretKey: tokenKey},
    })

    transport := embeddedidp.NewInProcessIssuerTransport(
        cfg.Issuer(), provider.Handler(), options)
    authenticator := newIntrospectionAuthenticator(transport, secret, audience)

    hub := sensors.NewHub(cfg.SensorInterval)
    mux := composeRoutes(provider, authenticator, hub)
    server := &http.Server{Handler: mux, ReadHeaderTimeout: 5s, ...}
    return supervisedApplication(...), nil
}
```

Construction order matters: clients and signing key must exist before `embeddedidp.New`, because the provider loads the client view during startup.

### 7.4 Introspection client and middleware

The access token is opaque. The resource API should validate it through `/idp/introspect`, not decode the ID token and not reach into tiny-idp's tables.

Create a confidential `idpstore.Client` with:

```go
idpstore.Client{
    ID:                "pulp-demo-api",
    Public:            false,
    SecretHash:        bcrypt(secret),
    AllowedGrantTypes: []string{idpstore.GrantAuthorizationCode},
    AllowedAudiences:  []string{audience},
    CanIntrospect:     true,
    AccessTokenTTL:    time.Hour,
    IDTokenTTL:        time.Hour,
    RefreshTokenTTL:   24 * time.Hour,
}
```

On restart, load the existing client, compare all security fields, and verify its hash with the owner-only secret. Do not generate a new bcrypt hash and compare bytes; bcrypt salts make equivalent hashes different.

The demo's `internal/authn` package should reproduce the public protocol behavior of tiny-idp xapp's internal reference without importing it:

1. Parse exactly one `Authorization: Bearer TOKEN` value.
2. POST `token=TOKEN` to the discovered `/introspect` endpoint with Basic auth.
3. Require `active=true`, exact issuer, `token_type=Bearer`, nonempty subject, future expiry, and the exact API audience.
4. Require every route scope.
5. Cache positive results for at most 30 seconds and inactive results for at most three seconds.
6. Key the cache by HMAC-SHA256 of the token, never by raw token.
7. Fail closed as 503 when the provider is unavailable; return 401 for invalid credentials and 403 for missing scope.
8. Never log the token or Basic-auth secret.

### 7.5 HTTP route contract

| Method/path | Scope | Response | Purpose |
|---|---|---|---|
| `GET /healthz` | public | `{"ok":true}` | Host liveness |
| `GET /idp/*` / `POST /idp/*` | provider-owned | HTML/JSON | tiny-idp |
| `GET /api/v1/me` | `demo.read` | subject, client, scopes, expiry | Prove identity |
| `GET /api/v1/demo/fortune` | `demo.read` | short deterministic message | Second REST demo |
| `GET /api/v1/sensors/snapshot` | `sensors.read` | latest sample | REST sensor gate |
| `GET /api/v1/sensors/ws` | `sensors.read` | WebSocket upgrade | Live stream |

All API JSON uses `Cache-Control: no-store`. Errors have one bounded shape:

```json
{"error":"unauthorized"}
```

### 7.6 Sensor protocol and hub

Wire schema v1:

```json
{
  "v": 1,
  "type": "sensor.sample",
  "seq": 1042,
  "ts_ms": 1784850000123,
  "temp_c": 22.37,
  "humidity_pct": 47.2,
  "pressure_hpa": 1012.8
}
```

One host goroutine produces a deterministic wave every 500 ms:

```text
temp     = 22 + 2.5*sin(seq/24) + noise(0.08)
humidity = 48 + 7*sin(seq/37 + 1.2) + noise(0.2)
pressure = 1013 + 3*sin(seq/90)
```

A shared hub broadcasts to per-connection channels of capacity one. If a client is slow, replace/drop its queued stale sample and increment a metric. The producer never blocks on a socket.

WebSocket handler pseudocode:

```go
principal := middleware.Authenticate(request, ["sensors.read"])
if unauthorized { return before upgrade }

conn := websocket.Accept(w, r, originPolicy)
ctx := deadlineAt(principal.ExpiresAt) // connection cannot outlive token
subscription := hub.Subscribe(capacity=1)
defer unsubscribe, conn.Close(StatusNormalClosure, "")

for {
    sample := <-subscription
    payload := boundedJSON(sample)
    writeCtx, cancel := context.WithTimeout(ctx, 2*time.Second)
    conn.Write(writeCtx, MessageText, payload)
    cancel()
}
```

The device sends no application messages. Server reads are limited to control/close handling. Maximum outbound JSON is 512 bytes.

## 8. Firmware design

### 8.1 New source files

```text
0114-papers3-pulp-os/main/
  net_auth.h/.cpp       device flow, POST worker, token owner
  net_socket.h/.cpp     WebSocket client and bounded message ring
  js_auth.cpp           safe auth status/config accessors; no token accessor
  js_socket.cpp         socket builder/status/ring accessors
```

Files to modify:

- `main/app_events.h` — add `ModuleId::Auth`, optional auth completion kinds, snapshots, console ops.
- `main/app_owner.cpp` — intercept Auth completions, run `AuthTick`, stop socket/auth at quiesce.
- `main/app_console.cpp` — `auth` and `socket` status/control commands with redacted output.
- `main/app_js_bindings.h`, `tools/js/pulp_stdlib.c`, `mqjs_stdlib_pulp.c`, `pulpjsc.c` — singleton ABI.
- `main/js_http.cpp` and `net_http.cpp` — safe `.bearer()` builder flag.
- `main/CMakeLists.txt` — new sources plus `json` and `esp_websocket_client` requirements.
- `main/idf_component.yml` — pin `espressif/esp_websocket_client: "1.3.0"` (the version already proven in this repository).
- `tools/js/apps/pulp.js` — SENSOR LINK app and launcher row.
- `app_power.cpp` — socket stop before Wi-Fi off.
- `js_probes.cpp` — auth/socket probes.

The component manifest belongs in `main/`, never at the project root (repository `AGENTS.md`).

### 8.2 Native auth state model

```text
Unconfigured
    | configure()
    v
Idle -- start() --> RequestingCode -- success --> WaitingForUser
 ^                    | failure                     |
 |                    v                             | AuthTick at due time
 |                  Error                           v
 |                                           PollingToken
 |                                             | pending -> WaitingForUser
 |                                             | slow_down -> WaitingForUser (+5s)
 |                                             | denied/expired/error -> Error
 |                                             | token -> Authorized
 |                                                          |
 +---------------- clear()/expiry <--------------------------+
```

Recommended enum:

```cpp
enum class AuthState : uint8_t {
    Unconfigured = 0, Idle, RequestingCode, WaitingForUser,
    PollingToken, Authorized, Expired, Error
};
```

`AuthState` changes happen on the owner. The worker writes a typed result mailbox and posts `ModuleDone{Auth}`. `AuthOwnerOnModuleDone` parses the result code, zeroes sensitive temporary fields, and advances the state. `AuthTick(now_us)` starts a token poll only when `now_us >= next_poll_us`.

This service does not use `RegisterModuleCb`. Its lifetime spans `resetTree()`. JavaScript observes it through a page tick, exactly as it observes Wi-Fi state.

### 8.3 Native storage and memory bounds

```cpp
struct AuthStateData {
    char issuer[192];        // public configuration
    char client_id[48];
    char scopes[160];
    char resource[192];

    char device_code[256];   // secret, RAM only
    char user_code[24];      // displayable
    char verification_uri[256];
    char verification_uri_complete[384];

    char access_token[2048]; // secret, RAM/PSRAM only
    int64_t grant_deadline_us;
    int64_t token_deadline_us;
    int64_t next_poll_us;
    uint32_t poll_interval_s;

    AuthState state;
    AuthError error;
};
```

Use an 8 KiB PSRAM HTTP response buffer and a 2 KiB form body cap. Parse with ESP-IDF's `json`/cJSON component in the worker. The typed mailbox copies only validated bounded fields. Before reusing or clearing secret buffers, overwrite them with zeroes through a non-optimized helper.

Never persist `device_code` or `access_token`. The public server configuration can initially be a constant in trusted `pulp.js`; a future ticket can add a string settings file.

### 8.4 JavaScript auth API

```js
auth.configure({
  issuer: 'http://192.168.0.X:8787/idp',
  clientId: 'pulp-papers3',
  scope: 'openid profile demo.read sensors.read',
  resource: 'http://192.168.0.X:8787/api'
});

auth.start();                  // starts native request; returns StatusCode
auth.state();                  // integer AuthState
auth.userCode();               // displayable; empty unless waiting
auth.verificationUri();
auth.verificationUriComplete();
auth.grantSecondsLeft();
auth.tokenSecondsLeft();
auth.pollSecondsLeft();
auth.error();                  // stable non-secret error name
auth.clear();                  // zero codes/token and return Idle
```

There is deliberately no `auth.token()`.

### 8.5 HTTP bearer extension

```js
http.get(API + '/api/v1/me')
    .bearer()                   // token copied directly auth -> HTTP header
    .limit(2048)
    .done(function (k, status, len) { ... })
    .send();
```

Implementation rules:

- `.bearer()` sets a boolean on the request slot; it does not format a JS string.
- `HttpSend` asks `AuthCopyBearerHeader()` to populate a private native header slot immediately before launch.
- Reject when auth is not authorized or expired.
- Reject if the URL's scheme/host/port differs from the configured resource origin or if the path is outside `/api/`.
- Redact `Authorization` in logs and snapshots.
- Zero the private header slot after worker cleanup.

### 8.6 WebSocket client and message ring

Use `esp_websocket_client` as a managed component. Its event callback runs outside the owner. Configuration:

```cpp
esp_websocket_client_config_t cfg{};
cfg.uri = ws_url;
cfg.headers = native_authorization_header; // CRLF terminated
cfg.network_timeout_ms = 5000;
cfg.reconnect_timeout_ms = 2000;
cfg.disable_auto_reconnect = false;
cfg.task_stack = measured_value_starting_at_8192;
cfg.crt_bundle_attach = esp_crt_bundle_attach; // for wss
```

The official component docs warn that one WebSocket message can arrive in multiple `WEBSOCKET_EVENT_DATA` events. Reassemble by `payload_offset` and total `payload_len`; do not assume `data_len` is a complete JSON document.

Recommended bounded ring:

```cpp
constexpr uint32_t kSocketRing = 64;
constexpr uint32_t kSocketMessageMax = 512;
struct SocketMessage { uint64_t seq; uint16_t len; char data[512]; };
```

Flow:

```text
websocket event task
  -> validate text opcode/fragment bounds
  -> reassemble <=512-byte message
  -> xQueueSend(message_queue, 0), increment ingress drop if full

socket ingest task
  -> copy into 64-entry PSRAM ring under short mutex
  -> increment native message sequence
  -> no JS, no present, no per-packet owner event

owner/JS every 2 s
  -> socket.messageCount/messageSeq/message
  -> parse unseen messages, retain last 60 samples
  -> redraw once
```

JavaScript surface:

```js
socket.open('ws://192.168.0.X:8787/api/v1/sensors/ws')
      .bearer()
      .start();
socket.state();               // idle/connecting/open/error
socket.messageCount();        // <=64
socket.messageSeq(i);         // native monotonic ring id
socket.message(i);            // bounded text
socket.dropped();
socket.lastError();           // no token/header text
socket.stop();
```

As with HTTP, `bearer()` enforces the configured resource origin and `/api/` path. `http://` maps only to `ws://`; `https://` maps only to `wss://`.

## 9. SENSOR LINK product app and e-ink plot

### 9.1 Product states

The app is a small view-state machine driven by native auth/socket state:

```text
connect view -> approval view -> authorized dashboard -> streaming dashboard
      ^              |                 |                        |
      +---- retry ---+---- failure ----+---- 401/expiry --------+
```

`enter('sensor')` rebuilds the view, but native auth and socket state survive. The page uses `p.every(1000, ...)` while waiting for approval and `p.every(2000, ...)` while streaming. A tick compares the observed native state/sequence with the last rendered values; unchanged state creates no damage.

### 9.2 REST demo sequence

After authorization:

1. `GET /api/v1/me` and show the `sub`.
2. `GET /api/v1/demo/fortune` and show one short line.
3. `GET /api/v1/sensors/snapshot` to seed the chart before opening the socket.
4. Open the WebSocket.

Do not launch overlapping `http` operations; the existing HTTP module has one slot. Chain them in callbacks. Do not call `enter()` until the chain is complete, or `resetTree()` will cancel the current HTTP callback.

### 9.3 Chart algorithm

Keep 60 parsed samples in a JavaScript circular array. Every two seconds, drain ring messages with `messageSeq > lastSeq`, validate `v`, `type`, finite numeric fields, and increasing server `seq`, then redraw the latest 60 temperatures.

Pseudocode:

```js
function redrawPlot(canvas, samples, width, height) {
  canvas.wipe();
  canvas.box(0, 0, width, height, 32, 2); // gray, thickness

  var min = minimum(temp_c), max = maximum(temp_c);
  if (max - min < 0.5) { min -= 0.25; max += 0.25; }

  for (var i = 1; i < samples.length; i++) {
    var x0 = scale(i - 1, 0, 59, 8, width - 8);
    var x1 = scale(i,     0, 59, 8, width - 8);
    var y0 = scale(samples[i-1].temp_c, min, max, height - 8, 8);
    var y1 = scale(samples[i].temp_c,   min, max, height - 8, 8);
    canvas.line(x0, y0, x1, y1, 0, 2);
  }
}
```

Command budget:

```text
1 border + at most 59 sample segments + up to 4 grid lines = 64 commands
64 < kCanvasCmds (96)
```

Do not draw one disc per sample unless the command count is recalculated. `wipe()` must run before each rebuild or commands accumulate to `CapacityExceeded`.

### 9.4 Refresh policy

Recommended initial policy:

- Network sample cadence: 500 ms.
- UI drain and partial present: 2 s.
- Canvas damage: one fixed chart rectangle.
- `paper.refreshTurns(20)` while streaming, followed by restoring the product default when leaving the app (add a getter/default helper if needed; do not leave a global policy mutation accidentally).
- Full refresh approximately every 40 seconds under continuous visible change; tune from ghosting evidence, not aesthetics alone.
- Pause panel updates when app is not visible, but the socket may remain connected only if product behavior explicitly wants background data. v1 should stop it on leaving SENSOR LINK to save power.

“Realtime” here means a live, continuous network stream with a recent chart, not LCD-rate animation.

## 10. Security and trust model

### 10.1 Development threat model

The first gate uses `http://` and `ws://` on a trusted private LAN. Bearer tokens, login POSTs, and browser cookies are not protected against a network observer. Bind intentionally and never port-forward this server. Log a startup warning:

```text
DEVELOPMENT MODE: HTTP bearer credentials are visible to the local network.
```

### 10.2 Production-shaped path

For any use outside a controlled lab:

- Use an HTTPS issuer and WSS endpoint.
- Run `embeddedidp.ProductionMode` with the required durable audit sink, secure cookies, rate limiter, address resolver, owner-only secrets, and reviewed signing-key lifecycle.
- Terminate TLS with a certificate trusted by the ESP certificate bundle; do not disable hostname or certificate verification.
- Configure trusted-proxy handling only for explicit proxy peers.
- Re-run tiny-idp production release gates; implementation availability is not blanket approval.

### 10.3 Device-side credential rules

- Access token and device code are RAM-only and zeroed on clear/restart.
- Never print request/response bodies from auth endpoints.
- Console snapshots report state, deadlines, HTTP status, and redacted lengths only.
- JavaScript receives no bearer value.
- Same-origin/path restrictions prevent attaching bearer credentials to arbitrary URLs.
- A socket is stopped at access-token expiry; reconnect requires a current token.
- A 401 from any protected API clears authorization and returns the app to reauthorization.
- User code is not a bearer secret but should disappear when the grant completes/expires.

## 11. Design decisions

### Decision: Embed the strict provider; do not modify tiny-idp

- **Context:** tiny-idp already exposes device authorization, browser verification, token issuance, SQLite state, and public embedding APIs.
- **Options considered:** modify tiny-idp; run `serve-dev` as a subprocess; embed the strict provider.
- **Decision:** Import `pkg/embeddedidp`, `pkg/sqlitestore`, and `pkg/idpaccounts` in the demo service.
- **Rationale:** One Go process owns provider and resource routes, startup is deterministic, and current focused tests prove the device path.
- **Consequences:** The demo follows the supported package boundary and requires no tiny-idp source changes. The service still owns resource-token introspection middleware.
- **Status:** accepted.

### Decision: Use DevMode + SQLite for the LAN gate

- **Context:** the PaperS3 must reach the issuer by LAN IP; production mode requires HTTPS and additional operational controls.
- **Options considered:** mock engine; strict ProductionMode immediately; strict DevMode with SQLite.
- **Decision:** strict/Fosite `DevMode` over HTTP for v1, with an explicit insecurity warning.
- **Rationale:** Exercises the real grant/storage/token path without pretending a self-signed LAN deployment is production-ready.
- **Consequences:** Captures are sensitive; Internet exposure is forbidden. Moving to production is a deployment phase, not a flag flip.
- **Status:** accepted.

### Decision: Keep bearer tokens native and memory-only

- **Context:** JS strings can be logged, retained, or exposed; SD credentials are plaintext; device access tokens last one hour.
- **Options considered:** implement the whole flow in JS; persist tokens; native RAM ownership.
- **Decision:** `net_auth` owns all protocol credentials; JS gets status/accessors only.
- **Rationale:** Reduces accidental exposure and avoids persistent bearer replay after device loss.
- **Consequences:** Reauthorization is required after reboot or expiry. HTTP/socket modules need private bearer hooks.
- **Status:** proposed.

### Decision: Auth is an OS state machine, not a module callback chain

- **Context:** browser approval can take minutes, while app switches call `resetTree()` and cancel registered callbacks.
- **Options considered:** nested JS callbacks/timeouts; one long worker; owner-driven native state machine.
- **Decision:** worker transactions + `AuthOwnerOnModuleDone` + `AuthTick`.
- **Rationale:** Survives page rebuilds, enforces poll timing centrally, and preserves one-owner rules.
- **Consequences:** Slightly more native code, but much simpler JS and deterministic recovery.
- **Status:** proposed.

### Decision: Validate opaque access tokens through in-process RFC 7662 introspection

- **Context:** tiny-idp issues opaque access tokens and exposes authenticated introspection.
- **Options considered:** decode ID token; query SQLite directly; introspect through provider handler.
- **Decision:** confidential resource client + in-process issuer transport + bounded cache.
- **Rationale:** Preserves provider/resource separation and validates active state, audience, expiry, and scopes.
- **Consequences:** Demo owns small middleware because tiny-idp's xapp helper is internal. The helper must be security-reviewed and tested.
- **Status:** proposed.

### Decision: Authenticate WebSocket in the HTTP handshake header

- **Context:** the ESP client can set headers; bearer tokens in query strings leak through logs and URLs.
- **Options considered:** token query parameter; first-frame authentication; `Authorization` header.
- **Decision:** `Authorization: Bearer` before upgrade; reject before `Accept`.
- **Rationale:** Reuses REST middleware and avoids token-bearing URLs.
- **Consequences:** Browser WebSocket clients cannot use this exact endpoint without a session/subprotocol design, which is out of scope.
- **Status:** proposed.

### Decision: Buffer network samples and throttle panel redraws

- **Context:** 2 Hz network data is reasonable; 2 Hz e-ink updates are not.
- **Options considered:** redraw each frame; HTTP polling; bounded WS ring + 0.5 Hz plot.
- **Decision:** receive at 2 Hz, ring 64 messages, redraw every two seconds.
- **Rationale:** Preserves streaming semantics while respecting panel latency, ghosting, and power.
- **Consequences:** The chart is recent rather than packet-synchronous; sequence counters expose skipped samples.
- **Status:** proposed.

### Decision: Pin the repository-proven WebSocket component first

- **Context:** this ESP-IDF 5.3.4 firmware has no WS dependency, but firmware 0074 already uses component 1.3.0 with headers, reconnect, and TLS bundle support.
- **Options considered:** latest unqualified component; copy vendored source; pin 1.3.0.
- **Decision:** begin with `espressif/esp_websocket_client: "1.3.0"` and upgrade only through a separate qualification gate.
- **Rationale:** Minimizes version uncertainty during auth integration.
- **Consequences:** Later bug/security fixes may justify an upgrade; record the reason and rerun fragmentation/reconnect tests.
- **Status:** proposed.

## 12. Phased implementation guide

### Phase 0 — Baselines and configuration

1. Build current 0114 with ESP-IDF 5.3.4.
2. Capture `js status`, `heap`, `net status`, and probes 16–18.
3. Confirm the stable USB path:
   `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00`.
4. Determine the workstation LAN IP and reserve/confirm it; set `--public-base-url` explicitly.
5. Create the Go module and add it to the existing development `go.work`.

Gate: unchanged firmware builds and device is reachable on the existing saved Wi-Fi.

### Phase 1 — Go identity host

Implement state directory permissions, SQLite, accounts service, seeded user reconciliation, device client + audience, signing-key bootstrap, persistent token secret, provider construction, readiness, server timeouts, zerolog, and graceful shutdown under `errgroup`.

Gate:

```bash
curl $BASE/idp/.well-known/openid-configuration | jq \
  '.issuer,.device_authorization_endpoint,.introspection_endpoint'
curl $BASE/idp/readyz | jq .
```

### Phase 2 — Resource middleware and REST APIs

Implement the introspection client, owner-only secret, introspection middleware, `/me`, `/fortune`, and `/snapshot`. Add table tests for malformed/multiple Authorization headers, inactive/expired/wrong-audience tokens, missing scopes, provider unavailable, and cache expiry.

Gate: a `curl` device-flow helper gets a token; authorized routes are 200; missing token is 401; wrong scope is 403.

### Phase 3 — WebSocket service

Implement the shared sensor hub and protected endpoint with capacity-one subscriber queues, write deadlines, token-expiry deadline, and metrics. Use a host test client with an Authorization header.

Gate: unauthenticated upgrade fails; authenticated stream produces increasing `seq`; a deliberately slow client does not block another client or the producer.

### Phase 4 — Native auth transport and state machine

Implement form encoding, bounded POST worker, cJSON parsing, state machine, redacted console command, and a probe against scripted HTTP responses before the live server. Do not add UI yet.

Probe matrix:

- start success fields;
- default interval;
- pending;
- slow_down adds five seconds;
- too-early local poll suppression;
- access denied;
- expired token/grant;
- oversized/malformed JSON;
- network timeout/backoff;
- successful token installs bearer and zeroes device code.

Gate: `auth status` reaches Authorized after browser approval; console never contains token bytes.

### Phase 5 — Bearer REST integration

Add `http.bearer()` and origin/path enforcement. Add a native/probe call to `/me`, then implement the JS REST chain.

Gate: `/me`, `/fortune`, and `/snapshot` work from PaperS3; arbitrary-host bearer attachment is rejected; server sees correct subject/audience/scopes.

### Phase 6 — WebSocket firmware

Add the managed component, native socket module, header attachment, fragmentation reassembly, bounded queue/ring, console status, and probes. Start with one small text frame, then fragmented 2 KiB rejection, reconnect, Wi-Fi loss, token expiry, and queue pressure.

Gate: ring sequence increases on hardware; unauthenticated/wrong-path starts fail locally or at 401; reconnect does not leak tasks or heap.

### Phase 7 — SENSOR LINK UI and plot

Add launcher row, approval screen, authorized dashboard, REST results, streaming chart, state errors, and explicit stop/clear actions. Keep strings under the 64-byte widget cap and plot commands under 96.

Gate: complete flow is operable from the device plus a browser; 60-point plot updates every two seconds; touch and console remain responsive.

### Phase 8 — Power, soak, and documentation

Quiesce order:

```text
SocketStop -> AuthCancelAndZero -> ServeStop -> WifiOff -> touch off -> flush -> sleep
```

Run a 30-minute stream soak, repeated reauthorization, denial, Wi-Fi drop/rejoin, server restart, and token-expiry simulation. Update the ESP-53 onboarding API section, README, diary, tasks, changelog, and ticket relations.

Gate: all definition-of-done items pass.

## 13. Testing and validation strategy

### 13.1 Go tests

- Identity bootstrap is idempotent and detects client drift.
- State directories are 0700; secret files are 0600.
- Password/account reconciliation does not silently replace identity.
- Device-flow integration covers pending, slow_down, approve, deny, consume-once, and expiry.
- Introspection validates issuer, audience, expiry, token type, and all scopes.
- WebSocket handshake uses bearer header; query token is rejected.
- Sensor hub is race-tested and slow-subscriber safe.
- `go test -race ./...`, `go vet ./...`, and build pass.

### 13.2 Firmware host tests

Extract pure helpers where possible:

- form URL encoding;
- bounded JSON-to-mailbox validation;
- auth transition table and poll scheduling;
- origin/path matcher including default ports and scheme conversion;
- WebSocket fragment assembler;
- ring wrap/ordering/drop accounting;
- sensor JSON validation and plot coordinate scaling.

Run existing `components/s3paper_core/tests/host && make run` after canvas use changes.

### 13.3 Device probes

Add probes 19–22 or the next free numbers:

- **19 auth parser/state:** synthetic response matrix, no network.
- **20 live device flow:** live server, pauses at user approval.
- **21 bearer REST:** 401/403/200, body cap, hostile URL rejection.
- **22 socket:** header auth, fragmentation, ring wrap, reconnect, drop counters.

Console additions:

```text
auth [status|start|clear]
socket [status|start URL|stop|head]
```

Status must be redacted:

```text
auth state=waiting user_code=ABCD-EFGH grant_left=542s poll_in=3s token_len=0
socket state=open rx=124 ring=64 ingress_drop=0 ring_overwrite=60 http=101
```

Never print `device_code`, `access_token`, or Authorization headers.

### 13.4 Hardware acceptance script

1. Start server with explicit LAN URL.
2. Join saved Wi-Fi and open SENSOR LINK.
3. Record displayed code and browser URL.
4. Confirm early polling remains within server limits.
5. Approve as seeded user.
6. Confirm REST subject and fortune.
7. Start stream; observe sequence and chart.
8. Stop server for 30 seconds; observe socket error/reconnect behavior.
9. Restart server; old token remains valid only if token secret/SQLite are preserved.
10. Force token expiry in a test configuration; verify socket closes and UI reauthorizes.
11. Sleep/wake; verify credentials are cleared and radio policy remains correct.

### 13.5 Soak metrics

Capture before/after:

- `heap` internal free/min/largest and PSRAM free/largest;
- owner event queue high-water and drops;
- JS exceptions/dispatches;
- auth transition/error counters;
- socket connects/disconnects/rx/fragments/oversize/drop/overwrite;
- present full/partial counts;
- server stream clients, broadcast drops, introspection cache hits.

Success is bounded steady state after warm-up, not necessarily zero ring overwrites: overwriting old samples is the intended latest-data policy.

## 14. Operator runbook (planned commands)

### 14.1 Start the service

```bash
cd 0114-papers3-pulp-os/demo-device-auth-server
mkdir -p var
printf '%s\n' 'correct horse battery staple' > var/demo-password
chmod 600 var/demo-password

go run ./cmd/pulp-auth-demo \
  --listen 0.0.0.0:8787 \
  --public-base-url http://192.168.0.X:8787 \
  --state-dir ./var \
  --demo-login alice \
  --demo-password-file ./var/demo-password \
  --log-level debug
```

Use tmux for the long-running server and `lsof-who -p 8787 -k` before restarting, per the Go workspace agent instructions.

### 14.2 Host smoke

```bash
BASE=http://192.168.0.X:8787
curl -fsS "$BASE/healthz" | jq .
curl -fsS "$BASE/idp/.well-known/openid-configuration" | jq .

DEVICE_JSON=$(curl -fsS -X POST "$BASE/idp/device_authorization" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data-urlencode client_id=pulp-papers3 \
  --data-urlencode 'scope=openid profile demo.read sensors.read' \
  --data-urlencode "resource=$BASE/api")
echo "$DEVICE_JSON" | jq .
```

Open `verification_uri_complete`, approve, then poll at the returned interval. Store tokens in shell variables only and do not paste them into ticket docs.

### 14.3 Firmware build/flash

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os
unset IDF_PYTHON_ENV_PATH
source ~/esp/esp-idf-5.3.4/export.sh
./tools/js/gen_pulp_stdlib.sh
./tools/js/build_bytecode_apps.sh
idf.py build
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00 flash
```

Use the existing console client, never `idf.py monitor`.

## 15. Risks, alternatives, and open questions

### 15.1 Main risks

- **HTTP demo leaks bearer credentials on the LAN.** Accepted only for controlled bring-up; loudly documented.
- **WebSocket/TLS internal-memory pressure.** Measure stack high-water and heap minima; do not assume the existing 8 KiB prior-art stack is correct.
- **Fragment handling errors.** Official client docs permit multiple DATA events per message; test offsets, control frames, oversize, and disconnect mid-message.
- **Global refresh policy leakage.** `paper.refreshTurns` mutates planner policy globally; restore it when leaving or add scoped native support.
- **Callback cancellation during REST chain.** Avoid `enter()` until the current HTTP callback completes.
- **Server host address changes.** v1 uses explicit trusted configuration. DHCP reservation or a stable local DNS name is preferable.
- **Token expiry during an open socket.** Server and firmware both enforce expiry; test race windows.
- **Resource middleware duplication.** It copies protocol behavior from tiny-idp xapp's internal helper. Keep it small and consider a future tiny-idp public resource-auth package, but do not block this demo or modify tiny-idp now.

### 15.2 Rejected alternatives

- **Put the token in the WebSocket URL.** Rejected because URLs are logged and copied widely.
- **Implement device polling entirely in `pulp.js`.** Rejected because resetTree cancels callbacks and exposes credentials to JS.
- **Use HTTP polling for sensors.** Valid fallback, but does not exercise authenticated WebSocket streaming.
- **Persist access/refresh tokens.** Rejected for v1 because secure at-rest storage and refresh rotation are outside scope.
- **Render every WebSocket message.** Rejected because network cadence and e-ink cadence must remain independent.
- **Use the device-hosted `serve` module for the identity provider.** Impossible: tiny-idp is a Go server-side provider, while `serve` is the constrained device's local HTTP server.

### 15.3 Questions to settle during implementation

1. What stable hostname/IP should be committed as the demo default, or should build configuration require it every time?
2. Should leaving SENSOR LINK stop the socket immediately (recommended v1) or keep a background latest-value stream?
3. Does the panel look cleaner at a 16, 20, or 30 partial-turn budget for this chart rectangle?
4. Is 512 bytes enough after the final sensor schema and future metadata? Keep the cap explicit even if raised.
5. Should `http.bearer()` be named `http.authorized()` to make the hidden-token behavior clearer?
6. Should the reusable socket singleton expose raw bounded JSON, or should a sensor-specific native decoder expose numeric accessors? Start raw; change only if MicroQuickJS parsing or heap evidence demands it.

## 16. Intern reading order and source map

Read in this order:

1. ESP-53 `design-doc/02` — complete PULP system onboarding.
2. ESP-53 `reference/01-implementation-diary.md` — especially Steps 3–8.
3. `0114/main/app_events.h:26-60,124-149` — event contracts.
4. `0114/main/app_owner.cpp:422-458,503-518` — owner dispatch and ticks.
5. `0114/main/app_js.cpp:269-340,480-496,526-540,609-625` — callbacks/ticks/reset.
6. `0114/main/net_http.cpp:35-147,152-227` — worker/mailbox pattern.
7. `0114/main/js_http.cpp:26-137` — fluent binding pattern.
8. `0114/tools/js/apps/pulp.js:23-45,765-769,964-986` — OS callbacks and network app patterns.
9. `components/s3paper_core/include/s3paper/widget.h:87-108,183-205` — canvas storage.
10. tiny-idp `docs/embedding-foundations.md` — supported embedding boundary.
11. tiny-idp `pkg/embeddedidp/bootstrap.go:32-47,77-223` — clients/bootstrap.
12. tiny-idp `pkg/embeddedidp/options.go:89-265` — provider validation.
13. tiny-idp `internal/fositeadapter/provider.go:550-733,1296-1352,1412-1510` — protocol behavior (read-only reference; do not import).
14. `0074-m5dial-web-remote/firmware/main/remote_client.cpp:182-318` — repository-proven WS lifecycle/header prior art; improve its full-message assumption with fragmentation reassembly.
15. RFC 8628 Sections 3.2–3.5 and 5 — response, user interaction, polling, errors, security.
16. Espressif WebSocket client docs — event data, fragmentation, headers, TLS bundle.

## 17. References

- RFC 8628, *OAuth 2.0 Device Authorization Grant*: <https://datatracker.ietf.org/doc/html/rfc8628>
- RFC 8707, *Resource Indicators for OAuth 2.0*: <https://datatracker.ietf.org/doc/html/rfc8707>
- RFC 7662, *OAuth 2.0 Token Introspection*: <https://datatracker.ietf.org/doc/html/rfc7662>
- Espressif `esp_websocket_client` documentation: <https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html>
- tiny-idp embedding guide: `/home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/tiny-idp/docs/embedding-foundations.md`
- tiny-idp device tutorial: `tiny-idp/cmd/tinyidp/doc/pages/tutorial-device-authorization.md`
- ESP-53 ticket: `ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--pulp-os-connectivity-and-peripherals-wifi-http-fetch-web-serving-filesystem-buzzer/`
- Repository WebSocket prior art: `0074-m5dial-web-remote/firmware/main/remote_client.cpp`
