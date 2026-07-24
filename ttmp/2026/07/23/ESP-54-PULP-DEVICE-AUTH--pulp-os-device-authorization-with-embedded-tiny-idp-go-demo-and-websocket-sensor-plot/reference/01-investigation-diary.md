---
Title: Investigation Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/tiny-idp/docs/embedding-foundations.md
      Note: |-
        Supported public tiny-idp composition boundary
        Supported tiny-idp embedding boundary
    - Path: abs:///home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/tiny-idp/pkg/embeddedidp/bootstrap.go
      Note: |-
        Device client and signing-key bootstrap behavior
        Device client bootstrap evidence
    - Path: repo://0074-m5dial-web-remote/firmware/main/remote_client.cpp
      Note: |-
        Existing ESP WebSocket client lifecycle prior art
        WebSocket client prior art
    - Path: repo://0114-papers3-pulp-os/demo-device-auth-server/internal/app/app.go
      Note: Embedded tiny-idp host and protected API composition (commit c6f742b)
    - Path: repo://0114-papers3-pulp-os/main/app_owner.cpp
      Note: Bounded owner dispatch through probe 23 (commit e97c589)
    - Path: repo://0114-papers3-pulp-os/main/js_probes.cpp
      Note: Executable live auth REST WSS and QR probes (commit e97c589)
    - Path: repo://0114-papers3-pulp-os/main/js_widgets.cpp
      Note: Retained Canvas QR encoder binding (commit 4c2364c)
    - Path: repo://0114-papers3-pulp-os/main/net_auth.cpp
      Note: Native RFC 8628 state machine and RAM-only token owner (commit 4c2364c)
    - Path: repo://0114-papers3-pulp-os/main/net_http.cpp
      Note: |-
        Existing worker/mailbox implementation that shaped the auth transport design
        Worker and mailbox reference
    - Path: repo://0114-papers3-pulp-os/main/net_socket.cpp
      Note: Authenticated fragmented WebSocket transport and bounded ring (commit 4c2364c)
    - Path: repo://0114-papers3-pulp-os/tools/js/apps/pulp.js
      Note: SENSOR LINK QR approval and e-ink plot UI (commit 4c2364c)
    - Path: repo://components/s3paper_core/include/s3paper/widget.h
      Note: Bounded PSRAM canvas command capacity for QR rendering (commit 4c2364c)
    - Path: repo://ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--pulp-os-connectivity-and-peripherals-wifi-http-fetch-web-serving-filesystem-buzzer/reference/01-implementation-diary.md
      Note: |-
        Most recent relevant ticket and evidence for the completed connectivity architecture
        Most recent PULP implementation evidence
ExternalSources:
    - https://datatracker.ietf.org/doc/html/rfc8628
    - https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html
Summary: Chronological evidence and reasoning behind the ESP-54 device authorization and realtime sensor-stream design.
LastUpdated: 2026-07-23T20:36:53.043094463-04:00
WhatFor: Resuming or reviewing ESP-54 research and implementation.
WhenToUse: Read before changing the design or beginning implementation phases.
---




# Diary

## Goal

Capture the evidence-first design of PULP OS device authorization, the embedded tiny-idp demo service, protected REST/WebSocket APIs, and the e-ink sensor plot so implementation can continue without rediscovering protocol or ownership constraints.

## Step 1: Audit the completed connectivity stack and design the authenticated streaming system

I created a new ESP-54 ticket, then read the most recent relevant ticket, ESP-53, including its system onboarding guide, connectivity guide, full implementation diary, and changelog. That established that Wi-Fi, HTTPS fetch, the one-owner event path, canvas primitives, and product integration are already complete; ESP-54 should extend those patterns rather than introduce a second runtime model.

I then audited tiny-idp's public embedding boundary and strict device-flow source, confirmed the focused device tests pass, reviewed RFC 8628 and Espressif WebSocket fragmentation/header contracts, and wrote a 17-section implementation guide. The central result is that tiny-idp needs no source modification: a new Go service can embed it through `pkg/embeddedidp`, while native PULP services keep bearer credentials out of JavaScript and throttle a 2 Hz stream into a 0.5 Hz e-ink plot.

### Prompt Context

**User prompt (verbatim):**

> we are working in /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5 , more precisely in 0114-papers3-pulp-os (read the relevant docmgr ticket, the most recent).
>
> We want to add device-auth support to the pulp os so that we can start a small go service that embeds tiny-idp (whch we have checked out in this workspace irectory for your reference, hopefully you won't have to modify it to create a device-auth server for the eink). It's connected over USB so you can flash it however you want. Once you have a device auth, hit some demo APIs on the server (a go server you can ecreate in a subdirectory of 0114-papers3-pulp-os/), with some streaming websocket plotting of realtime fake sensor data received from the server or such.
>
> Create a new docmgr ticket in /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5 to implement that, and then Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
> [REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory.

**Assistant interpretation:** Create a new ticket in the ESP32 documentation repository, deeply analyze the finished PULP connectivity implementation and the checked-out tiny-idp, design an embedded Go device-authorization server plus protected REST/WebSocket demo, write an intern-ready file-level implementation guide and diary, validate the ticket, and upload the bundle to reMarkable. Implementation and flashing are future phases; this turn produces the implementation blueprint.

**Inferred user intent:** Make the next implementation session executable by a new engineer: every security boundary, protocol state, native/JS ownership rule, API contract, plot constraint, file change, and hardware gate should already be decided or explicitly identified as an open question.

**Commit (code):** N/A — research and ticket documentation only.

### What I did

- Located the most recently updated PULP ticket with:
  - `find ttmp/2026 -type f -name index.md -printf '%T@ %p\n' | sort -nr`
  - `docmgr ticket list --ticket ESP-53-PULP-CONNECTIVITY`
  - `docmgr doc list --ticket ESP-53-PULP-CONNECTIVITY`
  - `docmgr task list --ticket ESP-53-PULP-CONNECTIVITY`
- Read ESP-53's index, two design documents, implementation diary, changelog, and `0114-papers3-pulp-os/README.md`.
- Created `ESP-54-PULP-DEVICE-AUTH` with design and diary documents plus ten research/delivery tasks.
- Audited line-numbered PULP sources:
  - `main/net_http.{h,cpp}` for bounded HTTPS workers and PSRAM mailboxes;
  - `main/app_events.h`, `app_owner.cpp`, and `app_js.cpp` for POD events, owner dispatch, callback lifecycle, and `resetTree()` cancellation;
  - `main/js_http.cpp` and `tools/js/apps/pulp.js` for builder/product patterns;
  - `components/s3paper_core/include/s3paper/widget.h` and refresh planner sources for graph command/refresh limits.
- Found repository WebSocket client prior art in `0074-m5dial-web-remote/firmware/main/remote_client.cpp` and the repository's WebSocket-over-Wi-Fi playbook.
- Audited tiny-idp:
  - `README.md` and `docs/embedding-foundations.md`;
  - `pkg/embeddedidp/{bootstrap,options,provider}.go`;
  - strict device authorization and token handling in `internal/fositeadapter/provider.go`;
  - the xapp resource server/introspection reference, noting it is under a Go `internal` path and cannot be imported by the new demo module.
- Verified current tiny-idp device behavior:
  - `GOWORK=off go test ./pkg/embeddedidp ./internal/fositeadapter -run 'Device|BootstrapCreatesBrowserDevice' -count=1`
  - both packages passed.
- Extracted RFC 8628 and Espressif WebSocket docs with Defuddle; verified `slow_down` interval behavior, `verification_uri_complete`, bearer display rules, fragmented DATA events, custom headers, `payload_offset`, and certificate-bundle support.
- Authored the primary guide with architecture diagrams, request/response examples, state machines, Go and firmware API sketches, eight decision records, eight implementation phases, host/device/soak tests, commands, risks, and a source reading order.

### Why

- ESP-53 already solved the hard PULP concurrency and networking foundations; reusing its worker/mailbox/owner rules minimizes regression risk.
- Device authorization spans minutes and app rebuilds, so a native OS-level state machine is safer than a JavaScript callback chain that `resetTree()` can cancel.
- tiny-idp already has strict RFC 8628 endpoints, durable hashed grants, public embedding, and introspection. Modifying it would add risk without enabling a missing capability.
- Tokens must remain native and memory-only so logging, JavaScript, SD, and arbitrary HTTP/WS URLs do not become credential exfiltration paths.
- WebSocket sample cadence and e-ink present cadence must be separated; otherwise a valid stream becomes an unusable panel workload.

### What worked

- ESP-53's diary gave exact, hardware-proven constraints and bug history instead of requiring architectural inference from source alone.
- tiny-idp's supported public packages are sufficient for the server: SQLite, accounts, client/key bootstrap, provider handler, and in-process issuer transport.
- Focused embedded/provider device tests passed in 5.5 seconds, confirming the audited checkout's device flow is not design-only.
- Existing firmware 0074 proves `esp_websocket_client` 1.3.0, reconnect configuration, custom headers, and an 8 KiB starting task stack already work with this repository's ESP-IDF generation.
- The current canvas can represent a 60-sample line graph within 64 of 96 commands.

### What didn't work

- Initial broad doc search failed because the query parser treated `tiny-idp` badly:

  ```text
  Error: iterate docs rows: no such column: idp
  ```

  I retried with the simpler query `auth` and then inspected recent ticket paths directly.

- The first recent-ticket `find` used `-maxdepth 3`, which only returned `_guidelines` and `_templates`; ticket indices are deeper. I corrected it to search `ttmp/2026` without that depth limit.
- A probe for `0040-.../firmware/main/idf_component.yml` returned:

  ```text
  nl: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/idf_component.yml: No such file or directory
  ```

  The useful dependency/version evidence was available from firmware 0074's managed component and the 0040 tree's vendored component sources, so this did not block the design.

### What I learned

- tiny-idp's public `DeviceClient` intentionally allows only `urn:ietf:params:oauth:grant-type:device_code`; v1 should not request `offline_access` or design around refresh tokens.
- The resource authentication helper in `cmd/tinyidp-xapp/internal/resourceauth` cannot be imported by a sibling demo service. A small demo-owned RFC 7662 middleware is required, using a confidential introspection client and `NewInProcessIssuerTransport`.
- Because bcrypt hashes are salted, idempotent resource-client reconciliation must compare the stored hash to the stable owner-only secret, not generate and byte-compare a new hash.
- The official ESP WebSocket callback may split one message across DATA events. The 0074 prior art copies each event as if complete; ESP-54 must reassemble by payload offset/total length before JSON parsing.
- The actual canvas line API is `line(x0,y0,x1,y1,gray,t?)`, which is more authoritative than an older guide's parameter ordering.
- The device is attached at the expected stable USB-Serial-JTAG path and enumerates as Espressif `303a:1001`; no flash was needed for this documentation phase.

### What was tricky to build

- The hardest boundary was deciding where bearer validation belongs. tiny-idp issues opaque access tokens, the demo API is in the same process, and a tempting shortcut is to query the SQLite store directly. That would couple the demo to provider internals and bypass RFC 7662 audience/capability checks. The chosen path uses an owner-only confidential introspection client and bounded in-process HTTP transport; the API sees only a constrained principal.
- The second hard boundary was auth lifetime versus `resetTree()`. Existing module callbacks are deliberately canceled on every app switch. Device authorization cannot be made reliable by adding more nested JS callbacks. The guide therefore treats auth as persistent native OS state advanced by worker results and `AuthTick`, while JS polls non-secret accessors.
- E-ink “realtime” needed an explicit two-clock design. The server emits at 500 ms, the native ring absorbs samples, and JavaScript redraws every two seconds. Sequence numbers and drop counters make coalescing observable rather than pretending every packet reached the panel.

### What warrants a second pair of eyes

- The proposed demo-owned introspection middleware is security-sensitive. Review exact Authorization parsing, issuer/audience/token-type/expiry checks, scope enforcement, HMAC cache keys, and unavailable-versus-unauthorized behavior.
- `http.bearer()` and `socket.bearer()` origin/path canonicalization must handle explicit/default ports, HTTP→WS and HTTPS→WSS mapping, path-prefix boundaries, userinfo, fragments, encoded slash/dot ambiguity, and redirects. A redirect must not carry Authorization to another origin.
- The auth worker's secret-buffer zeroing and worker/owner handoff need review for race-free ownership and compiler-resistant clearing.
- The global effect of `paper.refreshTurns()` needs a scoped restore strategy before the streaming app changes it.
- The WebSocket task stack and internal-memory cost are initial estimates from prior art, not measurements on PULP with Wi-Fi + TLS + MicroQuickJS active.

### What should be done in the future

- Implement Phases 0–8 in the guide, maintaining this diary step-by-step with code commit hashes and hardware transcripts.
- If the duplicated resource-introspection logic proves generally useful, propose a separate tiny-idp ticket for a supported public resource-auth package; do not add an internal import or compatibility shim in ESP-54.
- Qualify HTTPS/WSS with a CA-bundle-trusted certificate before any deployment outside a controlled LAN.

### Code review instructions

- Start with the primary guide sections 3–6 to verify current-state evidence and the proposed trust boundaries.
- Review server composition in sections 7.3–7.6 against tiny-idp `docs/embedding-foundations.md`, `pkg/embeddedidp/bootstrap.go`, and xapp's internal resource-auth reference.
- Review firmware sections 8–9 against `net_http.cpp`, `app_events.h`, `app_js.cpp`, `js_widgets.cpp`, and 0074 `remote_client.cpp`.
- Confirm every decision record has a consequence reflected in the phase plan and test matrix.
- Re-run the focused tiny-idp test command above before implementation if the checkout changes.

### Technical details

- Proposed server origin: explicit `http://<workstation-LAN-IP>:8787`; issuer `/idp`; audience `/api`.
- Device client: `pulp-papers3`; scopes `openid profile demo.read sensors.read`; no refresh grant.
- Auth grant TTL/poll defaults observed in tiny-idp: ten minutes/five seconds.
- Auth response cap: 8 KiB; token cap: 2 KiB; WebSocket message cap: 512 bytes; ring: 64 messages.
- Sensor stream: 2 Hz; plot: 0.5 Hz; chart history: 60 values; canvas estimate: 64/96 commands.
- Go WebSocket recommendation: `github.com/coder/websocket` v1.8.15 (current package release found during research).
- ESP WebSocket starting pin: `espressif/esp_websocket_client` 1.3.0, already present as repository prior art.

## Step 2: Implement and prove the embedded identity host and native authenticated clients

I implemented the complete host and device path from the design rather than modifying tiny-idp. The Go service embeds the provider through its public package, persists identity state in SQLite, seeds a device client and demo account, introspects opaque tokens for protected APIs, and emits a bounded synthetic sensor stream. The firmware owns authorization and bearer tokens natively, exposes only redacted state to JavaScript, and adds protected HTTP and WebSocket builders.

The first end-to-end hardware run completed device approval and protected REST successfully. It also exposed the important resource limit that the WebSocket TLS task could not allocate an mbedTLS context after the UI and earlier HTTPS calls had fragmented internal RAM. Moving mbedTLS allocations to trusted PaperS3 PSRAM and enabling dynamic TLS buffers fixed the failure; the repeated hardware run connected WSS and accumulated 49 messages with zero drops and zero JavaScript exceptions.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Implement every phase in the approved guide, validate on the USB-connected PaperS3, preserve token secrecy, and keep evidence and commits current.

**Inferred user intent:** Deliver a real, maintainable authenticated PULP application rather than a design-only artifact or mocked happy path.

**Commit (code):** `c6f742b` — "ESP-54 server: embedded tiny-idp device auth, protected APIs, sensor WebSocket"

**Commit (firmware):** `4c2364c` — "ESP-54 firmware: native device auth, protected clients, and sensor UI"

### What I did

- Added `0114-papers3-pulp-os/demo-device-auth-server/` with strict public-API tiny-idp embedding, SQLite state, seeded account/client data, RFC 7662 middleware, REST endpoints, WebSocket hub, HTTPS development certificates, tests, and operator documentation.
- Kept the server running in tmux session `esp54-auth-server` at `https://192.168.0.39:8790`; port 8787 was restored to its unrelated existing owner after accidental contention was detected.
- Added native `net_auth.{h,cpp}` and `net_socket.{h,cpp}`, owner events/ticks, console status, sleep quiesce, certificate pinning, bounded response/message storage, fragmentation reassembly, reconnect behavior, and secure clearing.
- Added `http.bearer()` and `socket.bearer()` with strict trusted-origin checks; no JavaScript token getter exists.
- Added MicroQuickJS bindings, generated stdlib, generated application bytecode, the SENSOR LINK screen, and a 60-sample bounded e-ink chart.
- Ran `go test ./...`, `go test -race ./...`, `go vet ./...`, and `go build ./...` for the server.
- Regenerated with `tools/js/gen_pulp_stdlib.sh` and `tools/js/build_bytecode_apps.sh`, built with ESP-IDF 5.3.4, and flashed through the stable USB Serial/JTAG path.
- Approved real device grants via the CSRF-protected browser flow and verified `/api/v1/me`, fortune, WSS handshake authentication, sensor delivery, heap telemetry, and redacted status.

### Why

- Native ownership makes bearer tokens unavailable to application scripts, logs, flash, SD, URL query strings, and arbitrary origins.
- The owner-task/event architecture preserves existing PULP concurrency invariants and lets device authorization outlive JavaScript tree resets.
- HTTPS/WSS is required because strict tiny-idp rejects a non-loopback HTTP issuer; the embedded development CA preserves transport authentication on the LAN.
- A bounded ring and decoupled 2 Hz ingress/0.5 Hz display cadence preserve stream continuity without abusing the e-ink panel.

### What worked

- The host device flow, CSRF browser approval, opaque token issuance, introspection, protected REST, and WSS path all worked without tiny-idp changes.
- Firmware status remained redacted: the console reported only `token_len=94`, while JavaScript had timers and state but no token accessor.
- After the TLS allocator fix, hardware evidence showed `auth state=5`, `socket state=2 received=49 dropped=0 ring=49`, `internal_free=49567`, `internal_min_free=37475`, `internal_largest=21504`, and `exceptions=0`.
- The generated firmware image was `0x1c3c80` bytes with 56% of the smallest application partition free.

### What didn't work

- A non-loopback development HTTP issuer failed as designed with `dev http issuer must be loopback`; the implementation changed to HTTPS/WSS with an embedded development CA.
- The first SENSOR LINK entry called `auth.configure()` unconditionally and reset a valid native session. The app now configures only when `auth.state() === 0`.
- WSS initially failed repeatedly with:

  ```text
  mbedtls_ssl_setup returned -0x7F00
  ESP_ERR_MBEDTLS_SSL_SETUP_FAILED
  internal_free=50227 internal_largest=12800
  ```

  The WebSocket task had reduced contiguous internal RAM below the TLS context requirement. `CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y` plus `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y` moved TLS allocations to trusted octal PSRAM. The next run connected and streamed without drops.
- A direct scripted approval POST returned `invalid csrf token`. Replaying the actual GET form cookie, `interaction`, and `csrf_token` correctly exercised the same CSRF-protected path as a browser.

### What I learned

- Total free heap is not enough evidence for TLS viability; the largest contiguous internal block and task-stack allocation order matter.
- The PaperS3's trusted PSRAM is appropriate for this controlled demo's TLS allocator threat model and substantially improves coexistence between UI, HTTPS, and WSS.
- Device authorization is genuinely OS state: entering or rebuilding an app must not implicitly clear it.
- tiny-idp's UI CSRF contract should be used, not bypassed in test automation.

### What was tricky to build

- Callback events can arrive after an application tree reset, while authorization must continue. Native state owns all secrets and progress; JavaScript polls snapshots and can safely disappear.
- WebSocket events fragment messages and run on a component task. The implementation validates total length and offsets before publishing one bounded message to the owner-visible ring.
- TLS failure appeared only with the fully rendered app and prior REST handshakes. Reproducing it under the real screen was essential; a standalone socket smoke test had misleadingly passed.

### What warrants a second pair of eyes

- Review origin canonicalization and redirect behavior in `net_http.cpp`, `net_auth.cpp`, and `net_socket.cpp` for bearer confinement.
- Review secure-clear timing and worker/owner state handoffs for stale callbacks during sleep or explicit clear.
- Review the PSRAM TLS choice against any future hostile physical-memory threat model; this ticket's development device does not claim that model.

### What should be done in the future

- Complete denial, expiry, server-loss, Wi-Fi-loss, reconnect, sleep, malformed/oversized payload, ring-wrap, and 30-minute soak acceptance gates before closing ESP-54.
- Replace the short-lived LAN development CA for any deployment outside the controlled workstation/device demo.

### Code review instructions

- Start at `demo-device-auth-server/internal/app/app.go`, then `main/net_auth.cpp`, `main/net_socket.cpp`, and `tools/js/apps/pulp.js`.
- Confirm no token accessor, persistence path, query-string bearer, or arbitrary-origin bearer route exists.
- Re-run server tests with the ticket's `GOWORK`, regenerate JS artifacts, build under ESP-IDF 5.3.4, flash, and inspect redacted `auth status`, `socket status`, `heap`, and `js status`.

### Technical details

- Server tmux session: `esp54-auth-server`; endpoint: `https://192.168.0.39:8790`; issuer: `/idp`; audience: `/api`; WSS: `/api/v1/sensors/ws`.
- Hardware log artifacts: `/tmp/esp54-auth-start.log`, `/tmp/esp54-authorized.log`, and `/tmp/esp54-psram-ws.log`.
- Firmware uses `espressif/esp_websocket_client` 1.3.0 and ESP-IDF 5.3.4.

## Step 3: Add QR-assisted approval and validate it on the e-ink screen

I added a reusable Canvas `qr(text, size)` primitive and used `verificationUriComplete()` on SENSOR LINK so approval is now scannable instead of requiring manual URL and code entry. The manual eight-character code remains prominent as a fallback, and the QR payload is cleared as soon as authorization succeeds.

The QR encoder runs natively with M5GFX's bounded encoder and emits horizontally coalesced monochrome fill runs into PSRAM-backed retained canvas storage. I regenerated bytecode and stdlib, built and flashed the firmware, started a fresh grant on-device, and observed the pending screen with zero JavaScript exceptions before approving it and proving REST/WSS still worked.

### Prompt Context

**User prompt (verbatim):** "show a qr code"

**Assistant interpretation:** Put a large scannable QR code on the device-authorization screen, encoded with the complete verification URI, while keeping the manual code fallback.

**Inferred user intent:** Make browser approval immediate and ergonomic on the physical e-ink device.

**Commit (code):** `4c2364c` — "ESP-54 firmware: native device auth, protected clients, and sensor UI"

### What I did

- Added `Widget.qr(text, size)` to the MicroQuickJS Canvas API and generated bindings.
- Used M5GFX `lgfx_qrcode` versions 1–10 with a fixed stack buffer, bounded 64–480 pixel target, monochrome runs, and a four-module quiet zone.
- Increased per-canvas command capacity from 96 to 512 in PSRAM so QR runs remain bounded without taking scarce internal RAM.
- Added a centered 180 px QR area and `SCAN QR / OR ENTER CODE MANUALLY` label; retained the visible user code.
- Reduced the chart from 330 to 280 px to keep the approval controls and live plot within the 540×960 viewport.
- Regenerated stdlib/bytecode, built, flashed in tmux, and created hardware grant `3V6J-DKAW`.

### Why

- RFC 8628 explicitly provides `verification_uri_complete` for this low-friction handoff.
- Rendering the QR as a retained canvas keeps it compatible with existing e-ink diff/present behavior rather than drawing out-of-band directly to M5GFX.
- The complete URI carries only the public one-time user code; the native bearer token remains inaccessible.

### What worked

- The pending authorization state rendered with no JavaScript exception: `exceptions=0`, `state=3`, and a 572-second grant lifetime.
- After CSRF-protected approval, the same firmware reached `state=5`, cleared the QR canvas, completed protected REST, connected WSS, and received 49 samples with zero drops.
- Widget arena allocation moved from 26,652 to 66,588 PSRAM bytes; internal heap remained available for tasks and DMA.

### What didn't work

- The first incremental build failed because `main` included the M5GFX QR header without declaring the direct component dependency:

  ```text
  fatal error: lgfx/utility/lgfx_qrcode.h: No such file or directory
  ```

  Adding `m5stack__m5gfx` to `main/CMakeLists.txt` fixed component include propagation.
- `git diff --cached --check` found one generated trailing space in `main/js_pulp.h`; it was stripped before the focused commit.

### What I learned

- QR modules should be coalesced by horizontal black runs; one retained command per module would consume much more memory and draw-op capacity.
- A persistent QR widget needs explicit wiping when the state becomes authorized so an expired approval URI is not left on-screen.
- The UI must budget vertical space for both approval and live plotting even though they are temporally distinct states.

### What was tricky to build

- The retained Canvas originally allowed only 96 commands, suitable for the 60-point chart but not a URL QR. Increasing the fixed PSRAM-backed slab preserves deterministic allocation while avoiding a special direct-display escape hatch.
- QR encoding failure must be explicit. The binding bounds input storage, encoder versions, target size, and command capacity, raising a JavaScript range error instead of rendering a truncated code silently.

### What warrants a second pair of eyes

- Scan the physical QR with more than one phone/camera and verify the complete URI and user code exactly match the native state.
- Review the 512-command per-canvas PSRAM cost against other future Canvas-heavy apps.
- Consider a future generic bitmap Canvas command if several dense machine-readable graphics are added; it is not justified solely for this QR.

### What should be done in the future

- Add a focused QR binding probe for oversize text and undersized target rejection.
- Capture a camera artifact of the pending screen during final acceptance.

### Code review instructions

- Review `js_w_qr` in `main/js_widgets.cpp`, the capacity change in `components/s3paper_core/include/s3paper/widget.h`, and the SENSOR LINK state transition in `tools/js/apps/pulp.js`.
- Validate with `tools/js/gen_pulp_stdlib.sh`, `tools/js/build_bytecode_apps.sh`, `idf.py build`, hardware flash, a fresh pending grant, `js status`, and a phone scan.

### Technical details

- QR source: `auth.verificationUriComplete()`; target: 180×180 px; error correction: M5GFX level 0; versions: 1–10.
- Pending evidence: `/tmp/esp54-qr-pending.log`, `/tmp/esp54-qr-status.log`, `/tmp/esp54-qr-js.log`.
- Post-approval/TLS evidence: `/tmp/esp54-psram-ws.log`.

## Step 4: Add executable probes and harden auth startup before lazy Wi-Fi initialization

I added probes 20–23 for live device authorization, protected REST, authenticated WSS, and retained QR rendering. The first live probe immediately found a real native fault: starting ESP HTTP before the lazily initialized Wi-Fi/TCP-IP stack asserted in lwIP instead of returning an error.

I hardened `AuthStart()` and token polling to require `kWifiUp`, changed the live probe to join saved Wi-Fi before starting, rebuilt and reflashed, and reran the complete chain. The fixed firmware created and approved a real grant, preserved authorization when probe 20 was rerun, returned protected `/me`, connected WSS, received samples without drops, and retained zero JavaScript exceptions.

### Prompt Context

**User prompt (verbatim):** (continuation of the Step 1 implementation request)

**Assistant interpretation:** Add repeatable console probes for the completed auth surfaces, then treat every hardware failure as a defect to triage rather than documenting a nominal pass.

**Inferred user intent:** Leave executable acceptance tools that prove both security boundaries and real network behavior on future builds.

**Commit (code):** `e97c589` — "ESP-54 probes: validate live auth, bearer REST, WSS, and QR"

### What I did

- Added probe 20 to join saved Wi-Fi, start a real device grant, and preserve an existing authorized session.
- Added probe 21 for native bearer REST, probe 22 for authenticated WSS, and probe 23 for QR render/range checks.
- Extended owner probe dispatch through probe 23.
- Added `WifiStatus() == kWifiUp` guards before authorization HTTP startup and token polling.
- Rebuilt and flashed twice: once to expose the crash and once to validate the fix.
- Approved grant `UNHV-JQ2E` through tiny-idp's cookie/interaction/CSRF form and ran probes 20–22.

### Why

- Console probes turn manual product behavior into repeatable, redacted hardware evidence.
- A public native API must reject invalid lifecycle order safely; JavaScript UI sequencing is not an acceptable crash guard.

### What worked

- Probe 23 rendered a QR with `ops=227` and printed `rendered=yes small-denied=yes` with zero exceptions.
- Probe 19 reconfirmed arbitrary HTTP/WSS origins are denied and `auth.token` is undefined.
- Fixed probe 20 reported `network=1 start=0 state=requesting`, then native status reached pending with a real code.
- After approval, probe 20 reported `authorized-preserved=true`.
- Probe 21 returned HTTP 200 and the expected subject/scopes; probe 22 reached `socket state=2 received=4 dropped=0 ring=4`.
- Heap evidence was `internal_free=49615 internal_min_free=44083 internal_largest=21504`; JavaScript exceptions remained zero.

### What didn't work

- The first probe 23 attempt returned `InvalidArgument` because owner dispatch stopped at probe 20. Extending the bounded dispatch range to 23 fixed it.
- The first probe 20 run crashed with:

  ```text
  assert failed: tcpip_send_msg_wait_sem /IDF/components/lwip/lwip/src/api/tcpip.c:449 (Invalid mbox)
  ```

  The device rebooted with `RTC_SW_CPU_RST`. The underlying cause was invoking `esp_http_client` before lazy Wi-Fi initialization. Native guards plus Wi-Fi-aware probe sequencing eliminated the assertion.

### What I learned

- Probe code is valuable precisely because it bypasses product-level helpers and exposes whether the native API itself is safe.
- Lazy radio initialization means all native network entry points need explicit lifecycle contracts; relying on lwIP internals to be ready is unsafe.
- Rerunning an auth probe while authorized must be non-destructive, mirroring the SENSOR LINK fix.

### What was tricky to build

- Probe 20 is asynchronous: `wifi.joinSaved()` completes later on the owner event path, so the probe must print from its callback and operators must inspect subsequent `auth status` rather than treating eval completion as network completion.
- The crash occurred after the probe printed success because worker startup raced into an uninitialized TCP/IP mailbox. Capturing the full monitor transcript, not just command return status, was required to diagnose it.

### What warrants a second pair of eyes

- Audit other native HTTP/socket starts for the same pre-Wi-Fi lifecycle assumption.
- Review whether `InvalidArgument` is sufficiently expressive for “network not up”; the stable status enum currently has no `Unavailable` member.
- Confirm token polling behavior across Wi-Fi loss/rejoin: it now pauses while down, but final reconnect acceptance remains open.

### What should be done in the future

- Run denial, expiry, reconnect, sleep, malformed/oversized response, ring-wrap, and soak gates.
- Consider a shared native `NetworkReady()` precondition helper if more modules gain direct network starts.

### Code review instructions

- Review probe scripts and dispatcher bounds in `main/js_probes.cpp` and `main/app_owner.cpp`.
- Review Wi-Fi guards in `AuthStart()` and `AuthTick()` in `main/net_auth.cpp`.
- Flash, run probes 19, 23, then 20; approve the code; run 20, 21, and 22 again; inspect auth/socket/heap/JS status.

### Technical details

- Failure evidence: `/tmp/esp54-probes-pending.log`.
- Fixed pending evidence: `/tmp/esp54-probe20-safe.log`.
- Protected REST/WSS evidence: `/tmp/esp54-probes21-22.log`.
- Final probe firmware size: `0x1c4220`, with 56% application partition free.

## Step 5: Exercise denial, ring wrap, sleep, reconnect, soak, and token expiry

I ran the long-duration and lifecycle acceptance paths against the probe-enabled firmware. This included a CSRF-protected user denial, an authorized deep-sleep transition, server termination/restart, Wi-Fi off/rejoin, a timed 30-minute WSS soak, deliberate ring wrap with no consumer, and waiting for the one-hour access token to expire naturally.

All security and lifecycle transitions were explicit and redacted. Denial ended in `access_denied`; deep sleep quiesced networking and woke with no token or socket state; both server and Wi-Fi recovery reconnected WSS without reissuing the token; the 30-minute stream stayed connected; and expiry securely cleared the token and stopped the socket.

### Prompt Context

**User prompt (verbatim):** (continuation of the Step 1 implementation request)

**Assistant interpretation:** Continue through the ticket's fault, lifecycle, soak, and expiry gates with real elapsed-time hardware evidence.

**Inferred user intent:** Prove the system survives realistic interruptions and does not retain or use credentials beyond their lifecycle.

**Commit (code):** N/A — hardware acceptance against `e97c589`.

### What I did

- Started grant `RKQR-6AM7`, submitted a real denial through tiny-idp's CSRF interaction, and inspected native state.
- Allowed the unconsumed socket ring to wrap, proving bounded 64-message behavior and drop accounting.
- Authorized grant `8KT8-GH9X`, started WSS, invoked `sleep deep 3`, and reconnected after timer wake.
- Authorized grant `AHR6-G22E`, ran WSS continuously from 22:07:44 to 22:37:44, then collected auth/socket/heap/JS snapshots.
- Terminated the `esp54-auth-server` tmux process, observed socket reconnect state, recreated the tmux server, and observed automatic reconnection.
- Ran `net off`, observed disconnect, ran `net joinsaved`, and observed Wi-Fi and WSS recovery while the same native token remained valid.
- Waited through the remaining token lifetime and collected the expiry snapshot.

### Why

- Happy-path connectivity does not prove credential lifecycle, bounded storage, or reconnect correctness.
- Real elapsed time catches leaks, timer errors, and stale state that accelerated unit tests cannot establish on the complete device stack.

### What worked

- Denial: `auth state=7 ... token_len=0 error="access_denied"`, with zero JS exceptions.
- Ring wrap: `received=213 dropped=149 ring=64`; capacity remained fixed.
- Sleep: logs showed `sleep: mode=deep-timer seconds=3 (quiesce begins)` and `entering deep-timer sleep`; after wake, auth/socket counters and token length were zero.
- 30-minute soak: auth remained state 5 with 1,751 seconds left; WSS remained state 2 with 3,685 messages; internal free/min/largest were 49,611/42,107/21,504 bytes; exceptions remained zero.
- Server recovery: socket moved to reconnecting with an error, then returned to state 2 and resumed receive counts after the tmux server restarted.
- Wi-Fi recovery: socket moved to reconnecting after `net off`, saved network `yolobolo` rejoined, and socket returned to state 2 while auth stayed state 5.
- Expiry: `auth state=6 ... token_len=0 error="token_expired"`; socket stopped at state 0; internal heap recovered to 64,991 bytes; exceptions remained zero.

### What didn't work

- The first denial submission omitted login/password because the button used `formnovalidate`; tiny-idp still required authentication and redisplayed `Enter your username and password.` Reposting the same valid interaction/CSRF token with credentials and action `deny` correctly denied the grant.
- Sending Ctrl-C to the original server pipeline ended the tmux session itself. I recreated `esp54-auth-server` with an explicit trailing shell and confirmed the service and socket recovered.
- `sleep status` was reported as an unrecognized command by one post-wake console transcript despite the source accepting it. Auth/socket/JS snapshots and deep-sleep logs supplied the required lifecycle evidence; this console anomaly should be checked separately.

### What I learned

- A non-consuming probe is a useful deterministic ring-wrap test: overwrite/drop counts should increase while memory and capacity remain fixed.
- The WebSocket component's reconnect loop survives both application-server and Wi-Fi outages without JavaScript intervention.
- Token expiry correctly couples authorization and socket lifecycle: secure token clearing precedes socket shutdown.
- Deep sleep intentionally clears RAM-only authorization; reauthorization after wake is required and security-preserving.

### What was tricky to build

- Server interruption had to preserve the intended tmux workflow while genuinely closing the listener. The original pipeline exited its session on Ctrl-C, so recovery required recreating the named session rather than silently starting a background process elsewhere.
- Expiry evidence required waiting for the actual one-hour opaque token, not modifying clocks or adding a test-only shortcut. The final snapshot therefore proves production timer behavior.

### What warrants a second pair of eyes

- Investigate why one `sleep status` invocation was rejected after wake even though `sleep deep 3` and the parser source are valid.
- Review whether ring overwrite should be called `dropped` in operator UI; it accurately counts messages not retained, but during a deliberate no-consumer probe the number is expected and large.
- The 30-minute soak used the transport probe rather than the SENSOR LINK consumer, so it proves native stream stability/ring bounds but not 30 minutes of continuous panel updates.

### What should be done in the future

- Run the remaining malformed/oversized auth and WebSocket payload probes.
- Run a full 30-minute SENSOR LINK UI/panel soak and capture a camera artifact with final chart, present count, and drop count before checking P8 complete.
- Recheck the post-wake `sleep status` console anomaly.

### Code review instructions

- Compare `/tmp/esp54-denial-result2.log`, `/tmp/esp54-sleep-active.log`, `/tmp/esp54-sleep-wake.log`, `/tmp/esp54-soak-final.log`, `/tmp/esp54-server-loss.log`, `/tmp/esp54-server-restart2.log`, `/tmp/esp54-wifi-rejoin.log`, and `/tmp/esp54-token-expired.log`.
- Verify token length is always redacted and becomes zero on denial, sleep, and expiry.

### Technical details

- Soak window: 2026-07-23 22:07:44–22:37:44 EDT (exactly 30 minutes).
- Expiry wait window: 2026-07-23 22:41:34–23:07:54 EDT.
- Final expired state: socket stopped, 7,043 total messages observed, bounded ring 64, no active token, zero JavaScript exceptions.
