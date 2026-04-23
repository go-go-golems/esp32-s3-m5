/**
 * Debug schema - inspect raw tool_calls structure
 * Run: go-minitrace query duckdb --archive-glob '...' --sql "SELECT 1" --db-path :memory: --persist-loaded
 * Then use this script to inspect
 * 
 * Or better - write a pure JS approach that reads the raw JSON directly.
 */

__verb__("debugToolCalls", {
  name: "debug-tool-calls",
  short: "Debug tool_calls schema in the loaded archive",
  fields: {},
});

function debugToolCalls() {
  const mt = require("minitrace");
  
  // Check what typeof returns for tool_calls
  const typeInfo = mt.query(`
    SELECT 
      typeof(tool_calls) AS typeof_tool_calls,
      array_length(tool_calls) AS arr_len,
      tool_calls[0] AS first_element,
      json_type(tool_calls) AS json_type,
      -- Try casting
      (CAST(tool_calls AS JSON))::JSON[0]->>'tool_name' AS via_cast
    FROM ${mt.tableName}
    LIMIT 1
  `);
  
  console.log("Type info:", JSON.stringify(typeInfo, null, 2));
  
  // Try different approaches
  const approaches = mt.query(`
    SELECT
      -- Approach 1: Direct index + cast
      (CAST(tool_calls AS JSON[]))[1]->>'tool_name' AS approach1,
      -- Approach 2: json_each on raw
      -- Approach 3: unnest casted
      UNNEST(CAST(tool_calls AS JSON[]))[1]->>'tool_name' AS approach3
    FROM ${mt.tableName}
    LIMIT 1
  `);
  
  console.log("Approaches:", JSON.stringify(approaches, null, 2));
  
  // Check raw JSON bytes
  const raw = mt.query(`
    SELECT SUBSTR(CAST(tool_calls AS VARCHAR), 1, 500) AS raw_json
    FROM ${mt.tableName}
    LIMIT 1
  `);
  
  console.log("Raw:", raw[0]?.raw_json);
  
  return { typeInfo, approaches, raw };
}

__package__("hardware-research", {
  short: "Hardware research session analysis",
});