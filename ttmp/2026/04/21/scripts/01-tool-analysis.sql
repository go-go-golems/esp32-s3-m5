-- Analyze tool usage patterns in the m5stack research session
-- 
-- IMPORTANT SCHEMA NOTE:
-- tool_calls are stored as DuckDB JSON[] type, not a regular array.
-- To access them, you MUST cast to JSON[] first:
--   UNNEST(CAST(tool_calls AS JSON[])) AS tc
-- Then access fields with tc->>'field_name'

SELECT
  tc->>'tool_name' AS tool,
  tc->>'operation_type' AS op_type,
  COUNT(*) AS uses
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
GROUP BY tool, op_type
ORDER BY uses DESC
LIMIT 40;