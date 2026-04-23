-- Analyze user prompts to understand research goals and phases
--
-- Extracts user prompts with truncated content for analysis

WITH prompt_rows AS (
  SELECT
    CAST(json_extract(json_each.value, '$.index') AS INTEGER) AS index,
    json_extract_string(json_each.value, '$.content') AS content,
    json_extract_string(json_each.value, '$.role') AS role
  FROM sessions_base,
    LATERAL (SELECT json_each(turns) AS json_each) t
)
SELECT
  index,
  SUBSTR(content, 1, 400) AS prompt_preview
FROM prompt_rows
WHERE role = 'user'
ORDER BY index
LIMIT 80;