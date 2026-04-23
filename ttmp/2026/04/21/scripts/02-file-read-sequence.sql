-- Analyze what files were read, in what order
-- Shows the sequence of file operations to understand research flow
--
-- SCHEMA NOTE: See 01-tool-analysis.sql for JSON[] casting pattern

SELECT
  tc->>'tool_name' AS tool,
  tc->>'input'->>'path' AS path,
  tc->>'input'->>'limit' AS limit_val,
  tc->>'input'->>'offset' AS offset_val,
  CAST(tc->>'emitting_turn_index' AS INTEGER) AS turn_index
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
WHERE tc->>'tool_name' = 'Read'
ORDER BY turn_index
LIMIT 80;