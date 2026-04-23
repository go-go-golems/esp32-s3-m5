-- Web search queries made during the session
--
-- SCHEMA NOTE: See 01-tool-analysis.sql for JSON[] casting pattern

SELECT
  tc->>'input'->>'query' AS query,
  COUNT(*) AS times_searched
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
WHERE tc->>'tool_name' = 'web_search'
GROUP BY tc->>'input'->>'query'
ORDER BY times_searched DESC;