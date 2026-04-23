-- Analyze timing patterns to understand research phases
SELECT
  t.index,
  CASE CAST(t.timing->>'hour_of_day' AS INT)
    WHEN 0 THEN '00:00-01:00'
    WHEN 1 THEN '01:00-02:00'
    WHEN 2 THEN '02:00-03:00'
    WHEN 3 THEN '03:00-04:00'
    WHEN 4 THEN '04:00-05:00'
    WHEN 5 THEN '05:00-06:00'
    WHEN 6 THEN '06:00-07:00'
    WHEN 7 THEN '07:00-08:00'
    WHEN 8 THEN '08:00-09:00'
    WHEN 9 THEN '09:00-10:00'
    WHEN 10 THEN '10:00-11:00'
    WHEN 11 THEN '11:00-12:00'
    WHEN 12 THEN '12:00-13:00'
    WHEN 13 THEN '13:00-14:00'
    WHEN 14 THEN '14:00-15:00'
    WHEN 15 THEN '15:00-16:00'
    WHEN 16 THEN '16:00-17:00'
    WHEN 17 THEN '17:00-18:00'
    WHEN 18 THEN '18:00-19:00'
    WHEN 19 THEN '19:00-20:00'
    WHEN 20 THEN '20:00-21:00'
    WHEN 21 THEN '21:00-22:00'
    WHEN 22 THEN '22:00-23:00'
    WHEN 23 THEN '23:00-24:00'
  END AS hour_bucket,
  t.timing->>'started_at' AS started_at,
  t.timing->>'duration_seconds' AS duration_s,
  t.metrics->>'tool_call_count' AS tool_count,
  t.turns_count
FROM sessions_base s,
  LATERAL (
    SELECT 
      timing,
      metrics,
      (SELECT COUNT(*) FROM json_each(s.turns)) AS turns_count
  ) t
LIMIT 10;