-- Analyze web fetch / browser operations for source discovery
--
-- SCHEMA NOTE: See 01-tool-analysis.sql for JSON[] casting pattern

SELECT
  tc->>'tool_name' AS tool,
  tc->>'input'->>'url' AS url,
  tc->>'input'->>'path' AS path,
  SUBSTR(tc->>'input'->>'prompt', 1, 100) AS prompt_preview,
  CAST(tc->>'emitting_turn_index' AS INTEGER) AS turn_index
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
WHERE tc->>'tool_name' IN ('WebFetch', 'playwright_browser_navigate')
   OR tc->>'tool_name' LIKE 'playwright_%'
ORDER BY turn_index
LIMIT 30;