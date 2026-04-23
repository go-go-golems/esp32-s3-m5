-- Analyze file types read to understand research patterns
--
-- SCHEMA NOTE: See 01-tool-analysis.sql for JSON[] casting pattern

SELECT
  CASE
    WHEN tc->>'input'->>'path' LIKE '%.pdf' THEN 'PDF'
    WHEN tc->>'input'->>'path' LIKE '%.md' THEN 'Markdown'
    WHEN tc->>'input'->>'path' LIKE '%.c' OR tc->>'input'->>'path' LIKE '%.h' THEN 'C/C++'
    WHEN tc->>'input'->>'path' LIKE '%.cpp' OR tc->>'input'->>'path' LIKE '%.hpp' THEN 'C++'
    WHEN tc->>'input'->>'path' LIKE '%.py' THEN 'Python'
    WHEN tc->>'input'->>'path' LIKE '%.json' THEN 'JSON'
    WHEN tc->>'input'->>'path' LIKE '%.txt' THEN 'Text'
    WHEN tc->>'input'->>'path' LIKE '%.html' THEN 'HTML'
    WHEN tc->>'input'->>'path' LIKE '%datasheet%' OR tc->>'input'->>'path' LIKE '%SPEC%' THEN 'Datasheet'
    WHEN tc->>'input'->>'path' LIKE '%ttmp%' OR tc->>'input'->>'path' LIKE '%docmgr%' THEN 'Docmgr'
    WHEN tc->>'input'->>'path' LIKE '%obsidian%' THEN 'Obsidian'
    WHEN tc->>'input'->>'path' LIKE '%esp-idf%' OR tc->>'input'->>'path' LIKE '%ESP-IDF%' THEN 'ESP-IDF'
    WHEN tc->>'input'->>'path' LIKE '%firmware%' OR tc->>'input'->>'path' LIKE '%esp32%' THEN 'Firmware'
    WHEN tc->>'input'->>'path' LIKE '%m5stack%' OR tc->>'input'->>'path' LIKE '%M5Tab%' THEN 'M5Stack'
    WHEN tc->>'input'->>'path' LIKE '%workspaces%' THEN 'Workspace'
    ELSE 'Other'
  END AS file_category,
  COUNT(*) AS read_count
FROM sessions_base,
  LATERAL (SELECT UNNEST(CAST(tool_calls AS JSON[])) AS tc) t
WHERE tc->>'tool_name' = 'Read'
GROUP BY file_category
ORDER BY read_count DESC;