/**
 * hardware-session-analysis.js
 * 
 * Go-minitrace JS command handler for analyzing hardware research sessions.
 * This script demonstrates how to use the minitrace module to query
 * session data and produce structured analysis output.
 * 
 * Run with:
 *   go-minitrace query commands hardware-research session-analysis \
 *     --query-repository <path-to-this-dir> \
 *     --archive-glob '/path/to/*.minitrace.json' \
 *     --output json
 * 
 * Why JS instead of SQL:
 * - The Pi transcript JSONL has tool_calls stored as a JSON array of objects
 * - Each tool call has: tool_name, operation_type, input.path, input.command, etc.
 * - The go-minitrace DuckDB integration uses json_extract() for path access
 * - However, DuckDB's JSON path extraction can be tricky with nested structures
 * - JS gives us programmatic control over the data transformation
 */

__section__("filters", {
  title: "Filters",
  fields: {
    session_id: {
      type: "string",
      default: "",
      help: "Filter by session ID (partial match)",
    },
    limit: {
      type: "int",
      default: 100,
      help: "Maximum number of results per section",
    },
  },
});

function _getSessionFilter(filters) {
  return filters.session_id ? `WHERE id LIKE '%${filters.session_id}%'` : "";
}

/**
 * Tool usage breakdown - counts each tool type
 */
function toolUsage(filters) {
  const mt = require("minitrace");
  const limit = Math.min(filters.limit, 200);
  
  // Query tool calls and count by tool_name
  const query = `
    SELECT
      tc->>'tool_name' AS tool_name,
      tc->>'operation_type' AS op_type,
      COUNT(*) AS count
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (
      SELECT json_each_text FROM json_each_text(tool_calls)
    ) tc_text,
    LATERAL (SELECT json_parse(tc_text.value) AS tc) tc_parsed
    ${_getSessionFilter(filters)}
    GROUP BY tc->>'tool_name', tc->>'operation_type'
    ORDER BY count DESC
    LIMIT ${limit}
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message, hint: "Check JSON path syntax" }];
  }
}

/**
 * Analyze bash commands - extract common patterns
 */
function bashCommands(filters) {
  const mt = require("minitrace");
  const limit = Math.min(filters.limit, 100);
  
  // Use raw JSON extraction for bash commands
  const query = `
    SELECT
      tc->>'input'->>'command' AS command,
      COUNT(*) AS count
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (
      SELECT json_each_text FROM json_each_text(tool_calls)
    ) tc_text,
    LATERAL (SELECT json_parse(tc_text.value) AS tc) tc_parsed
    WHERE tc->>'tool_name' = 'bash'
    ${_getSessionFilter(filters)}
    GROUP BY tc->>'input'->>'command'
    ORDER BY count DESC
    LIMIT ${limit}
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

/**
 * Analyze write operations - what files were created
 */
function writeOperations(filters) {
  const mt = require("minitrace");
  const limit = Math.min(filters.limit, 50);
  
  const query = `
    SELECT
      tc->>'input'->>'path' AS path,
      COUNT(*) AS count
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (
      SELECT json_each_text FROM json_each_text(tool_calls)
    ) tc_text,
    LATERAL (SELECT json_parse(tc_text.value) AS tc) tc_parsed
    WHERE tc->>'tool_name' = 'Write'
    ${_getSessionFilter(filters)}
    GROUP BY tc->>'input'->>'path'
    ORDER BY count DESC, path ASC
    LIMIT ${limit}
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

/**
 * Analyze docmgr commands - documentation workflow
 */
function docmgrCommands(filters) {
  const mt = require("minitrace");
  
  // Look for docmgr, defuddle, remarkable in bash commands
  const query = `
    SELECT
      CASE
        WHEN tc->>'input'->>'command' LIKE '%docmgr ticket%' THEN 'ticket'
        WHEN tc->>'input'->>'command' LIKE '%docmgr doc%' THEN 'doc'
        WHEN tc->>'input'->>'command' LIKE '%docmgr task%' THEN 'task'
        WHEN tc->>'input'->>'command' LIKE '%docmgr changelog%' THEN 'changelog'
        WHEN tc->>'input'->>'command' LIKE '%docmgr doctor%' THEN 'doctor'
        WHEN tc->>'input'->>'command' LIKE '%docmgr vocab%' THEN 'vocab'
        WHEN tc->>'input'->>'command' LIKE '%defuddle%' THEN 'defuddle'
        WHEN tc->>'input'->>'command' LIKE '%remarkable%' THEN 'remarkable'
        ELSE 'other'
      END AS cmd_type,
      COUNT(*) AS count
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (
      SELECT json_each_text FROM json_each_text(tool_calls)
    ) tc_text,
    LATERAL (SELECT json_parse(tc_text.value) AS tc) tc_parsed
    WHERE tc->>'tool_name' = 'Bash'
      AND (tc->>'input'->>'command' LIKE '%docmgr%'
           OR tc->>'input'->>'command' LIKE '%defuddle%'
           OR tc->>'input'->>'command' LIKE '%remarkable%')
    ${_getSessionFilter(filters)}
    GROUP BY cmd_type
    ORDER BY count DESC
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

/**
 * Session metadata - timing, model, etc.
 */
function sessionInfo(filters) {
  const mt = require("minitrace");
  
  const query = `
    SELECT
      id,
      title,
      environment->>'model' AS model,
      environment->>'agent_framework' AS framework,
      CAST(metrics->>'turn_count' AS INT) AS turns,
      CAST(metrics->>'tool_call_count' AS INT) AS tools,
      ROUND(CAST(timing->>'duration_seconds' AS DOUBLE) / 3600, 2) AS hours,
      timing->>'started_at' AS started
    FROM ${mt.tableName}
    ${_getSessionFilter(filters)}
    ORDER BY timing->>'started_at' DESC
    LIMIT 10
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

/**
 * User prompts - research goals and progression
 */
function userPrompts(filters) {
  const mt = require("minitrace");
  const limit = Math.min(filters.limit, 50);
  
  // Extract user prompts from turns
  const query = `
    SELECT
      t.index,
      SUBSTR(t.content, 1, 300) AS content_preview
    FROM ${mt.tableName} s,
    LATERAL (
      SELECT 
        CAST(json_extract(json_each.value, '$.index') AS INT) AS index,
        json_extract_string(json_each.value, '$.content') AS content,
        json_extract(json_each.value, '$.role') AS role
      FROM json_each(s.turns)
    ) t
    WHERE json_extract_string(t.role, '$') = 'user'
    ORDER BY t.index
    LIMIT ${limit}
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

/**
 * File type analysis - what kinds of files were read
 */
function fileTypeAnalysis(filters) {
  const mt = require("minitrace");
  
  const query = `
    SELECT
      CASE
        WHEN tc->>'input'->>'path' LIKE '%.pdf' THEN 'PDF'
        WHEN tc->>'input'->>'path' LIKE '%.md' THEN 'Markdown'
        WHEN tc->>'input'->>'path' LIKE '%.c' OR tc->>'input'->>'path' LIKE '%.h' THEN 'C/C++'
        WHEN tc->>'input'->>'path' LIKE '%.py' THEN 'Python'
        WHEN tc->>'input'->>'path' LIKE '%.json' THEN 'JSON'
        WHEN tc->>'input'->>'path' LIKE '%datasheet%' THEN 'Datasheet'
        WHEN tc->>'input'->>'path' LIKE '%ttmp%' OR tc->>'input'->>'path' LIKE '%docmgr%' THEN 'Docmgr'
        WHEN tc->>'input'->>'path' LIKE '%obsidian%' THEN 'Obsidian'
        WHEN tc->>'input'->>'path' LIKE '%esp-idf%' THEN 'ESP-IDF'
        ELSE 'Other'
      END AS file_type,
      COUNT(*) AS count
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (
      SELECT json_each_text FROM json_each_text(tool_calls)
    ) tc_text,
    LATERAL (SELECT json_parse(tc_text.value) AS tc) tc_parsed
    WHERE tc->>'tool_name' = 'Read'
    ${_getSessionFilter(filters)}
    GROUP BY file_type
    ORDER BY count DESC
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

/**
 * Web search queries - what was searched for
 */
function webSearchQueries(filters) {
  const mt = require("minitrace");
  
  const query = `
    SELECT
      tc->>'input'->>'query' AS query,
      COUNT(*) AS count
    FROM ${mt.tableName}
    CROSS JOIN LATERAL (
      SELECT json_each_text FROM json_each_text(tool_calls)
    ) tc_text,
    LATERAL (SELECT json_parse(tc_text.value) AS tc) tc_parsed
    WHERE tc->>'tool_name' = 'web_search'
    ${_getSessionFilter(filters)}
    GROUP BY tc->>'input'->>'query'
    ORDER BY count DESC
  `;
  
  try {
    return mt.query(query);
  } catch (err) {
    return [{ error: err.message }];
  }
}

// === Command Verbs ===

__verb__("toolUsage", {
  name: "tool-usage",
  short: "Show tool usage breakdown",
  fields: { filters: { bind: "filters" } },
});

__verb__("bashCommands", {
  name: "bash-commands",
  short: "Show most common bash commands",
  fields: { filters: { bind: "filters" } },
});

__verb__("writeOperations", {
  name: "write-operations",
  short: "Show files that were written",
  fields: { filters: { bind: "filters" } },
});

__verb__("docmgrCommands", {
  name: "docmgr-commands",
  short: "Show docmgr and documentation tool usage",
  fields: { filters: { bind: "filters" } },
});

__verb__("sessionInfo", {
  name: "session-info",
  short: "Show session metadata and metrics",
  fields: { filters: { bind: "filters" } },
});

__verb__("userPrompts", {
  name: "user-prompts",
  short: "Show user prompt sequence",
  fields: { filters: { bind: "filters" } },
});

__verb__("fileTypeAnalysis", {
  name: "file-types",
  short: "Analyze file types read",
  fields: { filters: { bind: "filters" } },
});

__verb__("webSearchQueries", {
  name: "web-searches",
  short: "Show web search queries",
  fields: { filters: { bind: "filters" } },
});

/**
 * Full session analysis - runs all sub-analyses and combines
 */
function fullAnalysis(filters) {
  const tools = toolUsage(filters);
  const bash = bashCommands(filters);
  const writes = writeOperations(filters);
  const docmgr = docmgrCommands(filters);
  const info = sessionInfo(filters);
  const prompts = userPrompts(filters);
  const fileTypes = fileTypeAnalysis(filters);
  const searches = webSearchQueries(filters);
  
  return {
    session_info: info,
    tool_usage: tools,
    top_bash_commands: bash.slice(0, 20),
    write_operations: writes,
    docmgr_usage: docmgr,
    user_prompts: prompts.slice(0, 30),
    file_types: fileTypes,
    web_searches: searches,
  };
}

__verb__("fullAnalysis", {
  name: "full-analysis",
  short: "Run complete session analysis",
  fields: { filters: { bind: "filters" } },
});

__package__("hardware-research", {
  short: "Hardware research session analysis",
  long: "Analyze hardware research sessions to understand methodology, tool usage, and documentation patterns. Supports analysis of Pi and Codex transcript archives.",
});