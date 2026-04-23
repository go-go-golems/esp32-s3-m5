/**
 * hardware-research-summary.js
 *
 * Analyze a hardware research session to understand methodology,
 * tool usage patterns, and documentation workflow.
 * 
 * Produces a structured summary of the research process:
 * - Tool usage breakdown
 * - Documentation patterns
 * - Research phases
 */

__section__("filters", {
  title: "Filters",
  fields: {
    session_id: {
      type: "string",
      default: "",
      help: "Filter by specific session ID",
    },
    limit: {
      type: "int",
      default: 100,
      help: "Maximum number of results",
    },
  },
});

function toolUsage(mt, sessionIdFilter) {
  const whereClause = sessionIdFilter ? `WHERE id LIKE '%${sessionIdFilter}%'` : "";
  return mt.query(`
    SELECT
      REPLACE(CAST(json_extract(tc, '$.tool_name') AS VARCHAR), '"', '') AS tool,
      REPLACE(CAST(json_extract(tc, '$.operation_type') AS VARCHAR), '"', '') AS op_type,
      COUNT(*) AS uses
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (SELECT unnest(tool_calls) AS tc) t
    ${whereClause}
    GROUP BY tool, op_type
    ORDER BY uses DESC
    LIMIT 40
  `);
}

function researchPhases(mt) {
  // Analyze turn content to identify research phases
  return mt.query(`
    SELECT
      t.index,
      t.role,
      SUBSTR(t.content, 1, 300) AS content_preview
    FROM ${mt.tableName} s
    CROSS JOIN LATERAL (
      SELECT 
        CAST(json_extract(json_each.value, '$.index') AS INT) AS index,
        json_extract(json_each.value, '$.role') AS role,
        json_extract_string(json_each.value, '$.content') AS content
      FROM json_each(s.turns)
    ) t
    WHERE t.role = 'user' OR t.role = 'assistant'
    ORDER BY t.index
    LIMIT 100
  `);
}

function sessionMetrics(mt) {
  return mt.query(`
    SELECT
      id,
      title,
      environment->>'model' AS model,
      environment->>'agent_framework' AS framework,
      CAST(metrics->>'turn_count' AS INT) AS turns,
      CAST(metrics->>'tool_call_count' AS INT) AS tools,
      ROUND(CAST(metrics->>'total_input_tokens' AS DOUBLE) / 1000000, 2) AS input_M,
      ROUND(CAST(metrics->>'total_output_tokens' AS DOUBLE) / 1000000, 2) AS output_M,
      timing->>'started_at' AS started,
      ROUND(CAST(timing->>'duration_seconds' AS DOUBLE) / 3600, 2) AS duration_hours
    FROM ${mt.tableName}
    ORDER BY timing->>'started_at' DESC
    LIMIT 10
  `);
}

function documentationPatterns(mt) {
  // Analyze docmgr and write operations
  return mt.query(`
    WITH tc_list AS (
      SELECT 
        CAST(value AS JSON) AS tc_parsed,
        CAST(json_extract(value, '$.emitting_turn_index') AS INT) AS turn_index
      FROM ${mt.tableName},
        LATERAL (SELECT unnest(tool_calls) AS value) t
    )
    SELECT
      turn_index,
      tc_parsed->>'tool_name' AS tool,
      tc_parsed->>'input'->>'path' AS path
    FROM tc_list
    WHERE tc_parsed->>'tool_name' IN ('Write', 'docmgr')
    ORDER BY turn_index
    LIMIT 50
  `);
}

// Main verb: full research summary
function researchSummary(filters) {
  const mt = require("minitrace");
  
  const tools = toolUsage(mt, filters.session_id);
  const metrics = sessionMetrics(mt);
  
  return {
    session_info: metrics,
    tool_usage: tools,
    analysis_note: "Use 'research-phases' and 'documentation-patterns' for deeper analysis"
  };
}

__verb__("researchSummary", {
  name: "research-summary",
  short: "Generate hardware research session summary",
  fields: {
    filters: { bind: "filters" },
  },
});

__verb__("toolUsage", {
  name: "tool-usage",
  short: "List tool usage breakdown for research sessions",
  fields: {
    filters: { bind: "filters" },
  },
});

__verb__("sessionMetrics", {
  name: "session-metrics",
  short: "Show session metrics and timing",
  fields: {
    filters: { bind: "filters" },
  },
});

__package__("hardware-research", {
  short: "Hardware research session analysis tools",
  long: "Analyze hardware research sessions to understand methodology, tool usage, and documentation patterns."
});