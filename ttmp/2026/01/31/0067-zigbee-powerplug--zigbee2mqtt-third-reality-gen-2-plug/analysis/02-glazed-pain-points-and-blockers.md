---
Title: Glazed pain points and blockers
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: zigctl/cmd/bridge/permit_join.go
      Note: Permit-join command layering and output defaults
    - Path: zigctl/cmd/bridge/root.go
      Note: Bridge command tree context
    - Path: zigctl/pkg/zigbee/layer.go
      Note: Zigbee parameter layer definitions
ExternalSources: []
Summary: Detailed analysis of the Glazed + Cobra integration issues, friction points, and open questions encountered while evolving zigctl.
LastUpdated: 2026-02-02T00:13:00-05:00
WhatFor: Document practical pain points and current blockers so we can plan a safe implementation path.
WhenToUse: Use when modifying zigctl command/flag behavior or Glazed output defaults.
---


# Glazed pain points and blockers

## Goal

Capture the concrete pain points encountered while extending zigctl with Glazed/Cobra, explain why they are tricky, and document which parts are still uncertain or brittle.

## Scope

This report is focused on zigctl’s use of Glazed (parameter layers + output layer) and its interaction with Cobra’s flag parsing, especially around:
- command/flag placement and ordering,
- per-command output defaults (stream/yaml),
- the Glazed help system frontmatter,
- API drift in field/parameter definitions.

## Current status (high level)

- zigctl is functional and Glazed parsing works for normal `zigctl bridge permit-join --broker ...` usage.
- The user requested a “dual command” behavior where `permit-join` defaults to streaming YAML output and can be invoked as:
  `zigctl bridge --broker ... --base-topic ... permit-join --seconds 60 --stream --watch --output yaml`.
- I verified that flags placed before the subcommand are accepted by Cobra/Glazed in the current build (see “Observed behavior”).
- The remaining complexity is how to encode reliable defaults + expectations into the Glazed output layer without forcing users to remember additional flags.

## Observed behavior (verified)

I ran this command to verify the “flags before subcommand” syntax:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge --broker mqtt://localhost:1884 permit-join --seconds 1 --output yaml
```

Result:
- The command executed successfully and printed a YAML response payload from the Zigbee2MQTT bridge.
- This confirms that Cobra is willing to accept subcommand flags that appear before the subcommand token (at least for this command tree).

## Pain points and why they are tricky

### 1) Flag ordering expectations (Cobra + Glazed interaction)

**Symptom**
- There was uncertainty whether `zigctl bridge --broker ... permit-join` would be treated as “unknown flags on bridge” and rejected.

**Why it is tricky**
- Glazed registers flags only on the specific command layer (`cmd.Flags()`), not persistent flags on parent commands.
- Cobra’s flag parsing behavior with child commands is not obvious without reading its internals; it can accept flags for a later subcommand even if they are placed earlier in the argv list.
- This behavior appears to work in practice, but it is not visibly documented within Glazed, so it looks like a potential blocker until tested.

**Current conclusion**
- The requested syntax works in practice today. The main risk is relying on undocumented behavior. If we ever enable `TraverseChildren` or other Cobra parsing settings, this could change.

### 2) Making `permit-join` “stream YAML by default”

**Symptom**
- The user wants `permit-join` to behave as a streaming command by default, without requiring `--stream` or `--output yaml`.

**Why it is tricky**
- Glazed’s output defaults live in the Output parameter layer (created by `settings.NewGlazedParameterLayers()`), which is shared across commands.
- Per-command output defaults are possible, but are not obvious in the public API. The mechanism is:

```go
schema.NewGlazedSchema(
  settings.WithOutputParameterLayerOptions(
    layers.WithDefaults(map[string]interface{}{
      "output": "yaml",
      "stream": true,
    }),
  ),
)
```

- The above is not immediately discoverable without reading `pkg/settings/glazed_layer.go` and `pkg/settings/settings_output.go`. The lack of a concise “how to set per-command output defaults” example is a real friction point.

**Current conclusion**
- The defaults can be applied at command construction time, but this requires non-obvious configuration. It needs to be carefully scoped so it doesn’t globally change defaults for other commands.

### 3) Output layer semantics are uneven across formats

**Symptom**
- `--stream` is meaningful for `table`/`csv` outputs but not strictly required for `yaml` or `json` row output.

**Why it is tricky**
- Users expect `--stream` to matter everywhere, but in Glazed it only determines whether `table` output is rendered incrementally or as a full table at the end.
- For YAML output, rows are already emitted individually. Setting `stream=true` does not change YAML behavior in any obvious way.

**Current conclusion**
- We can still set `stream=true` for clarity, but it is largely a UX consistency flag for YAML.

### 4) API drift: `NewParameterDefinition` vs `fields.New` / `NewField`

**Symptom**
- Some historical Glazed docs/examples refer to `parameters.NewParameterDefinition` or similar APIs, while current zigctl code uses `fields.New` and `schema.WithFields`.

**Why it is tricky**
- The library has evolved, and the newer “field” helpers are in `pkg/cmds/fields` while older examples use `pkg/cmds/parameters` directly.
- Without checking module source, it’s easy to reach for the wrong constructor (`NewParameterDefinition` vs `fields.New`) and end up with style drift or incompatible examples.

**Current conclusion**
- zigctl is consistent on `fields.New`. This is the correct modern convention, but Glazed’s scattered examples still create confusion.

### 5) Help-system frontmatter is fragile

**Symptom**
- YAML frontmatter fails to parse if string values contain a colon but are not quoted.

**Why it is tricky**
- The Glazed help loader fails hard with a YAML parse error (“mapping values are not allowed in this context”) and does not give a targeted “quote this field” hint.
- Small formatting issues in the help docs can break the entire help-system load.

**Current conclusion**
- This is not a code blocker but a documentation sharp edge. It should be handled via doc authoring discipline and/or a lint step.

### 6) Parent/child command layering design is non-obvious

**Symptom**
- If we wanted to declare Zigbee settings at the `bridge` group level (to make them look persistent), we would likely need to attach those flags to the parent command.

**Why it is tricky**
- Glazed layers add flags only via `cmd.Flags()`, not `cmd.PersistentFlags()`. There is no built-in “persistent layer” concept.
- Adding the same layer to parent and subcommands causes duplicate flag registration errors.

**Current conclusion**
- The simplest safe approach is to keep Zigbee settings in each subcommand and accept that flags can be positioned before or after the subcommand (which, as observed, does work). Implementing a truly persistent layer would require a custom layer wrapper or a forked AddLayerToCobraCommand variant.

## What is still unclear or brittle

- **Reliance on Cobra flag ordering behavior**: although observed to work, we should not assume it is immutable without explicit tests.
- **Per-command output defaults**: correct API exists but is not documented in a way that makes it obvious. This can lead to inconsistent behaviors across commands if not carefully applied.
- **Potential need for persistent flags**: if we later introduce parent-level flags for UX consistency, we will need custom Glazed plumbing.

## Recommendations

1) **Add a small regression test or manual validation snippet** for the “flags before subcommand” invocation so that future refactors don’t silently break it.
2) **Document the per-command output default pattern** (with code snippet) in the zigctl design doc for future contributors.
3) **Avoid mixing persistent flags unless required**; current behavior is working without adding special Glazed layers.
4) **Consider a help-doc lint step** (or simple script) to ensure YAML frontmatter stays valid.

## Proposed next steps (if you want changes)

- Implement per-command output defaults in `permit-join` using `schema.NewGlazedSchema(settings.WithOutputParameterLayerOptions(...))`.
- Update the playbook and help tutorial to show the “flags before subcommand” syntax explicitly (the user-requested command line).
- Add a quick “CLI invocation smoke test” script that runs `zigctl bridge --broker ... permit-join` with short timeout to catch regressions.

## Reference files (code/context)

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl/cmd/bridge/permit_join.go`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl/cmd/bridge/root.go`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl/pkg/zigbee/layer.go`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl/doc/tutorials/01-getting-started-power-plug-join.md`
