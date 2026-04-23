-- Session metadata and metrics
--
-- Shows timing, model, and operational context

SELECT
  id,
  title,
  environment->>'model' AS model,
  environment->>'agent_framework' AS framework,
  environment->>'tools_enabled' AS tools,
  CAST(metrics->>'turn_count' AS INTEGER) AS turns,
  CAST(metrics->>'tool_call_count' AS INTEGER) AS tools,
  CAST(metrics->>'total_input_tokens' AS BIGINT) / 1000000.0 AS input_M,
  CAST(metrics->>'total_output_tokens' AS BIGINT) / 1000000.0 AS output_M,
  timing->>'started_at' AS started,
  ROUND(CAST(timing->>'duration_seconds' AS DOUBLE) / 3600, 2) AS duration_hours
FROM sessions_base
ORDER BY timing->>'started_at' DESC
LIMIT 10;