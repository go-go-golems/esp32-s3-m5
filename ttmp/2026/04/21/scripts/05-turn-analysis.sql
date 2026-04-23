-- Analyze turn content to extract research phase markers
-- Looks for patterns that indicate phase transitions in research
SELECT
  t.index,
  t.role,
  SUBSTR(t.content, 1, 300) AS content_preview
FROM sessions_base s,
  LATERAL (
    SELECT 
      json_each.value AS turn_json,
      CAST(json_extract(json_each.value, '$.index') AS INT) AS index,
      json_extract(json_each.value, '$.role') AS role,
      json_extract(json_each.value, '$.content') AS content
    FROM json_each(s.turns)
  ) t
WHERE t.role = 'user'
ORDER BY t.index
LIMIT 80;