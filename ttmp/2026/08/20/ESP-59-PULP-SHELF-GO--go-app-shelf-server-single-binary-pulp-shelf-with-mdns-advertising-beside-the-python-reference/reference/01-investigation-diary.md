---
Title: Investigation diary
Ticket: ESP-59-PULP-SHELF-GO
Status: active
Topics:
    - papers3
    - esp32s3
    - golang
    - mdns
    - networking
    - javascript
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/shelf/main.go
      Note: buildIndex strict/non-strict split; the caught divergence (commit 78417abc)
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-20T14:27:54.929008425-04:00
WhatFor: ""
WhenToUse: ""
---


# Investigation diary

## Goal

<!-- What is the purpose of this reference document? -->

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->

# Investigation diary — ESP-59 pulp-shelf in Go

## Goal

Chronological record of the Go shelf-server build: contract study, the
guide, the binary, and the parity + hardware gates.

## Step 1: Guide, binary, parity, hardware — one pass

The ticket ran start to finish in one session because the hard part was
already specified: the ESP-58 wire contract and a working Python
reference. The method was parity-first — the Python script's five
behavioral decisions (per-request rescan, sidecar degradation, fail-fast
metadata validation, outbound-IP base URL, single-interface mDNS bind)
were written down as the spec in the guide BEFORE the Go code, and a
mechanical parity gate compared the two implementations over the same
directory.

### Prompt Context

**User prompt (verbatim):** "make a go version yes. keep the python for reference. / Create a new docmgr ticket, Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** New ticket (ESP-59) delivering a Go implementation of the ESP-58 shelf contract, with the Python script preserved as reference, plus an intern guide stored in the ticket and uploaded to reMarkable.

**Inferred user intent:** A dependency-free single-binary shelf in the toolbox language, without forking the wire contract.

**Commit (code):** 78417abc — "ESP-59: ticket + intern guide + pulp-shelf Go binary (P1) + parity gates (P2)"

### What I did
- Guide (design-doc/01): the contract as law (each rule tied to its
  ESP-55/57 incident), the Python reference's anatomy as behavioral
  spec, decision record R-GOMDNS (libp2p/zeroconf/v2 over grandcat /
  hashicorp / hand-rolled), flag-for-flag CLI parity, a behavior parity
  table, program-shape pseudocode, and a validation flowchart.
- Binary: `0114-papers3-pulp-os/shelf/` (module `pulp-shelf`, go 1.25.5,
  `libp2p/zeroconf/v2 v2.2.0`). Struct field order deliberately matches
  the Python dict insertion order so `encoding/json` emits the same key
  sequence.
- Parity gates (host): canonical index byte-equal over tools/js/demos
  (after normalizing host:port and reserializing both through one JSON
  canonicalizer); module fetch byte-equal (d-books, 1970 B); traversal
  404 on both; bad-ASCII sidecar → both exit 1 with matching messages.
- mDNS gate: the zeroconf client lists "Go Shelf" at 192.168.0.39:8123
  with TXT path+name, beside the device's own "PULP OS" record.
- Hardware gate: device store lists "Go Shelf"; opening it shows all 12
  demos installed-marked; tap → `installed d-power (1643B)`; the Go
  server's log shows the exact request pair (GET /pulp/index.json, GET
  /apps/d-power.js from 192.168.0.149). Shots in sources/shots/.

### What worked
- Parity-first: writing the reference's decisions down as spec made the
  Go port mostly transcription, and the gate caught the one divergence.

### What didn't work
1. **Raw index bytes differ**: Python's `json.dumps` default separators
   include spaces; Go's `Marshal` is compact. Resolution: the parity
   gate canonicalizes both sides (parse → dump with fixed separators,
   key order preserved) — semantic equality is the contract, whitespace
   is not.
2. **Divergence caught by the validation gate**: the Go port initially
   made bad-id filenames FATAL at startup; the Python reference skips
   them with a warning and is fatal only for non-ASCII metadata. The
   reference is the spec: Go now skips too, and the guide §3.3 (which
   had over-claimed "every id must match... kills the server") was
   corrected. A guide bug found by its own test plan.
3. **Shell trap**: `pkill` patterns matching the shell's own eval line
   (the ESP-55 bracket-escape trick applies to pkill too) and a
   backgrounded server killed by its own process group on command exit —
   `nohup` + absolute-path cwd fixed the run.

### What I learned
- `libp2p/zeroconf/v2.Register` + `Shutdown()` is a faithful mirror of
  python-zeroconf's register/unregister including the goodbye packet;
  the single-interface bind maps to passing one `net.Interface`.
- Go's `flag` accepts both `-dir` and `--dir`, so the CLI is invocation-
  compatible with the Python script's argparse doubles.

### What was tricky to build
- Making "byte-level parity" a meaningful claim: it required pinning
  struct field order to the Python dict order AND defining the
  comparison as canonical-form equality. Without the first, key order
  differs; without the second, whitespace fails the gate spuriously.
- A subtle Go bug in the first draft: `continue` inside a map-range
  validation loop skipped the wrong loop, so non-strict mode would have
  included an app with invalid metadata. Rewritten as an explicit
  `invalidMeta` helper with the `continue` at the app level.

### What warrants a second pair of eyes
- Shutdown path: `mdns.Shutdown()` then `srv.Close()` with a 3 s
  hard-exit timer — verify the goodbye packet actually lands (the
  Python reference's unregister does; parity here is asserted, not yet
  packet-captured).
- `ifaceFor` picks the first interface owning the outbound IP; exotic
  setups (same IP on two interfaces) are unhandled by design.

### What should be done in the future
- `-iface`/`-ip` flag for multi-homed hosts (guide §6).
- Packet-capture check of the goodbye on shutdown.
- Registry features (upload proxying, versioning) belong to a future
  ticket, not this binary.

### Code review instructions
- `git show 78417abc` — start at `shelf/main.go` `buildIndex` (the
  contract core) and the strict/non-strict split; then the parity gate
  transcript in this diary.
- Validate: `cd 0114-papers3-pulp-os/shelf && go build -o pulp-shelf .`,
  run beside the Python reference with `-no-advertise` on two ports,
  canonical-compare the indexes; then run advertising and walk the
  device store.

### Technical details
- Gates: canonical index equal = True; module 1970 B equal; both exit 1
  on `Type & Widgets`; device toast `installed d-power (1643B)`.
