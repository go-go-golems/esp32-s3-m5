---
Title: Bug Report: go-minitrace query commands subcommand does not accept --archive-glob
Ticket: TKT-2026-0422-hardware-research-methodology
Status: active
Topics:
    - bug
    - go-minitrace
    - duckdb
    - query
    - schema
---

# Bug Report: `query commands` Cannot Load Archives Without `--archive-glob`

## Summary

The `go-minitrace query commands` subcommand (used for running structured JS/SQL query commands from a `--query-repository`) does **not** accept `--archive-glob` as a flag, making it impossible to run custom JS query commands against a specific minitrace archive without modifying the global app config or environment variables.

The only way to load a custom archive into `query commands` is through:
1. The `GO_MINITRACE_QUERY_REPOSITORIES` environment variable (only for loading command definitions, not archives)
2. App config file `queryRepositories`
3. `--query-repository` flag (only for loading command definitions, not archives)

**There is no flag to specify which minitrace archive to query.**

This creates a chicken-and-egg problem: to run a JS query command that helps you understand your data, you first need to have configured the archive path globally.

## Steps to Reproduce

```bash
# Convert a Pi transcript to minitrace format
go-minitrace convert pi \
  --source-session /path/to/session.jsonl \
  --output-dir /tmp/my-archive

# Try to run a custom JS query command against this archive
go-minitrace query commands my-repo my-analysis \
  --archive-glob '/tmp/my-archive/**/*.minitrace.json' \
  --query-repository ./my-commands

# ERROR: unknown flag: --archive-glob
```

## Expected Behavior

The `go-minitrace query commands` subcommand should accept `--archive-glob` as a global runtime flag (similar to how `go-minitrace query duckdb` accepts it), so users can:

1. Run custom structured query commands against a specific archive
2. Debug JS query commands in isolation without polluting global config
3. Run one-off analyses without persistent configuration

## Actual Behavior

```
Error: unknown flag: --archive-glob
```

The `--archive-glob` flag is only accepted by:
- `go-minitrace query duckdb`
- `go-minitrace convert`
- `go-minitrace serve`

But **not** by `go-minitrace query commands`, even though query commands internally need to query a DuckDB table backed by an archive.

## Workaround

The workaround is to either:

1. **Set the archive path globally** via app config (~/.config/go-minitrace/config.yaml):
   ```yaml
   queryRepositories:
     - ./my-commands
   archiveGlobs:
     - /tmp/my-archive/**/*.minitrace.json
   ```
   But this requires a config file and changes global state.

2. **Use `query duckdb` with raw SQL** instead of structured query commands:
   ```bash
   go-minitrace query duckdb \
     --archive-glob '/tmp/my-archive/**/*.minitrace.json' \
     --sql "SELECT * FROM sessions_base LIMIT 5"
   ```
   This works but defeats the purpose of having structured query commands.

3. **Load the archive with `--persist-loaded` then query separately**:
   ```bash
   # Step 1: Load archive
   go-minitrace query duckdb \
     --archive-glob '/tmp/my-archive/**/*.minitrace.json' \
     --db-path /tmp/my-archive/debug.duckdb \
     --persist-loaded --load-only
   
   # Step 2: Query with separate command
   # But query commands don't accept --db-path either!
   ```
   This fails because `query commands` also doesn't accept `--db-path`.

4. **Use the `--query-repository` env var trick**:
   ```bash
   GO_MINITRACE_QUERY_REPOSITORIES=./my-commands \
   go-minitrace query commands my-analysis
   ```
   But this still doesn't help because there's no way to specify the archive.

## Root Cause

The `query commands` subcommand is implemented as a separate Cobra command group that doesn't inherit the global flags from `query duckdb`. The archive loading is handled differently between the two subcommands.

Looking at the source (hypothesized):
- `query duckdb` uses `queryDuckDBCmd.Flags()` which includes `--archive-glob`
- `query commands` uses `queryCommandsCmd.Flags()` which does NOT include `--archive-glob`

The JS command handlers in `query commands` internally call `mt.query()` which queries the DuckDB table, but the table must already be loaded. However, there's no way to tell `query commands` to load an archive.

## Evidence

From our analysis session:

```
# This works:
go-minitrace query duckdb \
  --archive-glob '/tmp/minitrace-m5stack/active/**/*.minitrace.json' \
  --preset framework-summary

# This fails:
go-minitrace query commands hardware-research debug-tool-calls \
  --archive-glob '/tmp/minitrace-m5stack/active/**/*.minitrace.json' \
  --query-repository ./my-commands

# Error: unknown flag: --archive-glob
```

## Impact

This bug prevents users from:
1. Running custom JS query commands for exploratory analysis
2. Debugging JS command handlers against real archives
3. Sharing reproducible analysis workflows that specify both commands and data sources

## Suggested Fix

Add `--archive-glob` (and related flags like `--db-path`, `--persist-loaded`, `--table-name`) as **global flags** on the `go-minitrace query` parent command, so they're available to all subcommands including `query commands`.

Alternatively, add a `--runtime-archive-glob` flag specifically to `query commands` that overrides the default loading behavior.

## Tags

- go-minitrace
- query-commands
- archive-glob
- duckdb
- structured-commands