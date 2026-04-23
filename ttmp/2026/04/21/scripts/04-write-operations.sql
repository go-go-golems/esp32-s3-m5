-- Analyze write operations to understand documentation output
--
-- SCHEMA NOTE: See 01-tool-analysis.sql for JSON[] casting pattern

SELECT
  CAST(tc->>'emitting_turn_index' AS INTEGER) AS turn_index,
  tc->>'tool_name' AS tool,
  tc->>'input'->>'path' AS path,
  SUBSTR(tc->>'input'->>'content', 1, 150) AS content_preview
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
WHERE tc->>'tool_name' IN ('Write', 'WriteNoOverwrite', 'Edit')
ORDER BY turn_index;