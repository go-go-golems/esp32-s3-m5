-- Analyze docmgr command usage patterns
--
-- SCHEMA NOTE: See 01-tool-analysis.sql for JSON[] casting pattern

SELECT
  CASE
    WHEN tc->>'input'->>'command' LIKE '%docmgr ticket%' THEN 'ticket'
    WHEN tc->>'input'->>'command' LIKE '%docmgr doc%' THEN 'doc'
    WHEN tc->>'input'->>'command' LIKE '%docmgr task%' THEN 'task'
    WHEN tc->>'input'->>'command' LIKE '%docmgr changelog%' THEN 'changelog'
    WHEN tc->>'input'->>'command' LIKE '%docmgr doctor%' THEN 'doctor'
    WHEN tc->>'input'->>'command' LIKE '%docmgr vocab%' THEN 'vocab'
    WHEN tc->>'input'->>'command' LIKE '%docmgr status%' THEN 'status'
    WHEN tc->>'input'->>'command' LIKE '%docmgr%' THEN 'other'
    WHEN tc->>'input'->>'command' LIKE '%defuddle%' THEN 'defuddle'
    WHEN tc->>'input'->>'command' LIKE '%remark%' THEN 'remarkable'
    ELSE 'other'
  END AS cmd_type,
  COUNT(*) AS usage_count
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
WHERE tc->>'tool_name' = 'Bash'
  AND (
    tc->>'input'->>'command' LIKE '%docmgr%'
    OR tc->>'input'->>'command' LIKE '%defuddle%'
    OR tc->>'input'->>'command' LIKE '%remark%'
  )
GROUP BY cmd_type
ORDER BY usage_count DESC;