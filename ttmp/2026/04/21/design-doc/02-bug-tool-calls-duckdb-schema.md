---
Title: Schema Struggle Report: tool_calls stored as DuckDB JSON[] causes DuckDB query failures
Ticket: TKT-2026-0422-hardware-research-methodology
Status: active
Topics:
    - schema
    - duckdb
    - tool-calls
    - minitrace
    - analysis
---

# Schema Struggle Report: Why DuckDB Can't Access `tool_calls` Fields

## The Problem

When analyzing a Pi agent transcript with `go-minitrace query duckdb`, attempting to access fields within `tool_calls` array elements fails or returns `nil`, even though the data is clearly present in the JSON files.

Example: trying to count tool usage by `tool_name`:

```sql
-- This returns 0 rows or nil:
SELECT
  tc->>'tool_name' AS tool,
  COUNT(*) AS uses
FROM sessions_base,
  LATERAL (SELECT UNNEST(tool_calls) AS tc) t
GROUP BY tool;
```

## Root Cause

The `tool_calls` column in the DuckDB table is stored as type **`JSON[]`** (DuckDB's native JSON array type), NOT as a standard JSON array or Postgres-style JSONB array.

When DuckDB loads a JSON array from a `.minitrace.json` file, it infers the type as `JSON[]` (list of JSON values), which has different semantics than:
- `JSONB[]` (array of JSONB objects)
- `VARCHAR[]` (array of strings)
- Standard SQL arrays

### Why `UNNEST` alone doesn't work

```sql
-- FAILS - returns nothing:
SELECT tc->>'tool_name'
FROM sessions_base,
  LATERAL (SELECT UNNEST(tool_calls) AS tc) t
```

DuckDB's `UNNEST` on a `JSON[]` type returns JSON *values*, not *objects*. You can't use `->>` path access on a bare JSON value directly.

### Why indexing with `[0]` doesn't work

```sql
-- Returns nil:
SELECT tool_calls[0]->>'tool_name' FROM sessions_base;

-- Casting to VARCHAR also doesn't help:
SELECT SUBSTR(CAST(tool_calls AS VARCHAR), 1, 500)
-- Shows raw JSON text starting with array content, but indexing still fails
```

### The Working Pattern

After extensive experimentation, the correct pattern is:

```sql
-- Cast to JSON[] explicitly, THEN unnest:
SELECT
  tc->>'tool_name' AS tool,
  COUNT(*) AS uses
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
GROUP BY tool;
```

The `CAST(tool_calls AS JSON[])` tells DuckDB to interpret the `JSON[]` values as a DuckDB native array, which then allows proper `UNNEST` and field access.

## What We Tried (The Failing Approaches)

| Approach | SQL Pattern | Result |
|---------|-------------|--------|
| Direct UNNEST | `UNNEST(tool_calls)` | Empty results |
| JSON each | `json_each(tool_calls)` | Error: not a table function |
| JSON extract | `json_extract(tool_calls, '$[0]')` | nil |
| Index access | `tool_calls[0]->>'tool_name'` | nil |
| Type check | `typeof(tool_calls[0])` | JSON (not accessible) |
| JSON parse on each | `json_parse(value)` from UNNEST | Error: json_parse doesn't exist |
| json_each_text | `json_each_text(tool_calls)` | Error: function doesn't exist |
| JSON[] explicit | `CAST(tool_calls AS JSON[])` then UNNEST | **WORKS** |

## Evidence from Our Analysis

### Failed Query 1: Direct UNNEST
```sql
SELECT
  REPLACE(CAST(json_extract(tc, '$.tool_name') AS VARCHAR), '"', '') AS tool,
  COUNT(*) AS uses
FROM sessions_base,
  LATERAL (SELECT UNNEST(tool_calls) AS tc) t(tc)
GROUP BY tool;

-- Result: 0 rows
```

### Failed Query 2: json_each on JSON[] type
```sql
SELECT tc->>'tool_name'
FROM sessions_base,
  LATERAL (SELECT json_each(tool_calls) AS tc) t;

-- Error: json_each is a table function but used as scalar
```

### Failed Query 3: json_extract on JSON[] elements
```sql
SELECT
  json_extract(tool_calls, '$[0].tool_name') AS tool
FROM sessions_base;

-- Result: nil
```

### Working Query
```sql
SELECT
  tc->>'tool_name' AS tool,
  tc->>'operation_type' AS op_type,
  COUNT(*) AS uses
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
GROUP BY tool, op_type
ORDER BY uses DESC;

-- Result: (719 rows, correctly showing bash:273, read:211, etc.)
```

## Why This Matters for Analysis

This schema quirk made our entire analysis session painful:

1. **Lost time**: We spent 30+ minutes trying different SQL patterns before finding the working one.
2. **Documentation gap**: There's no indication in the minitrace schema docs that `tool_calls` needs special handling.
3. **SQL portability**: The working pattern (`CAST AS JSON[]`) is DuckDB-specific syntax that wouldn't work in PostgreSQL or SQLite.
4. **Confusion about schema**: It's unclear whether this is intentional (DuckDB optimization) or a bug in the conversion process.

## Alternative Schema Designs That Would Fix This

### Option A: Store tool_calls as a regular JSONB column

Instead of relying on DuckDB's type inference, explicitly serialize `tool_calls` as a JSONB/JSON value that DuckDB handles more naturally:

```json
{
  "tool_calls": [
    {"tool_name": "bash", "operation_type": "EXECUTE", ...}
  ]
}
```

Then queries become:
```sql
-- Works naturally:
SELECT tc->>'tool_name'
FROM sessions_base,
  LATERAL json_each(tool_calls) AS tc;
```

### Option B: Normalize tool_calls into a separate table

Create a `tool_calls` table with a foreign key to `sessions`:

```sql
CREATE TABLE tool_calls (
  session_id TEXT REFERENCES sessions(id),
  index INTEGER,
  tool_name TEXT,
  operation_type TEXT,
  input JSONB,
  output JSONB
);
```

This allows straightforward queries without any JSON gymnastics.

### Option C: Use a consistent array type across all adapters

Ensure all minitrace adapters (pi, codex, etc.) convert arrays to a type that DuckDB handles predictably:
- Either always `JSONB[]` (but DuckDB doesn't have JSONB)
- Or always a DuckDB-native typed array with explicit casting in the schema

### Option D: Document the current behavior with cookbook examples

If the `JSON[]` type is intentional for performance reasons, at minimum document:
1. The exact DuckDB type of each array column
2. The correct SQL pattern to access them
3. A "DuckDB JSON[] cookbook" page in the go-minitrace docs

## Recommendations for go-minitrace

1. **Add a schema reference page** that explicitly documents the DuckDB type of every array column (`tool_calls`, `turns`, etc.) and the correct access patterns.

2. **Consider a `CAST AS` helper or view** that auto-casts common array columns, or provides a view like `sessions_flat` that explodes common arrays.

3. **Fix or document `query commands` archivable loading** (separate bug): even if we had the correct SQL, we couldn't run it through `query commands` because that subcommand doesn't accept `--archive-glob`.

## Lesson Learned

When working with DuckDB and nested JSON arrays:
- Always check `typeof(column)` first
- If the type is `JSON[]`, you need `CAST(column AS JSON[])` before `UNNEST`
- `json_each()` only works on JSON objects, not JSON arrays
- DuckDB's JSON type system is different from PostgreSQL's JSONB

## Tags

- schema
- duckdb
- tool-calls
- JSON[]
- analysis-struggle
- go-minitrace