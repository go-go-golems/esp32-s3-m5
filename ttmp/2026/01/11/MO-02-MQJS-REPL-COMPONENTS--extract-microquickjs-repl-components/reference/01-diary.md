# Diary

## Goal

Track the analysis and design work for extracting ReplLoop, JsEvaluator, and Spiffs into reusable ESP-IDF components.

## Step 1: Create ticket workspace and scaffolding

I created the new ticket and the core documents (analysis, design doc, diary), then added initial tasks. This establishes a stable workspace for the component-extraction effort without mixing it into the previous MicroQuickJS bring-up ticket.

The initial tasks make it explicit that we need both an inventory of existing components and a design plan for extraction and usage.

### What I did
- Created ticket `MO-02-MQJS-REPL-COMPONENTS` via docmgr
- Added analysis, design-doc, and diary documents
- Added initial tasks and removed the placeholder

### Why
- Provide a clean workspace and task list for the component-extraction effort

### What worked
- Docmgr ticket creation and document scaffolding succeeded

### What didn't work
- N/A

### What I learned
- The docmgr root is still `esp32-s3-m5/ttmp`, so new tickets land there automatically

### What was tricky to build
- Keeping tasks tidy and removing the default placeholder

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Start with `esp32-s3-m5/ttmp/2026/01/11/MO-02-MQJS-REPL-COMPONENTS--extract-microquickjs-repl-components/index.md`
- Confirm the analysis/design/diary docs exist

### Technical details
- Commands run:
  - `docmgr ticket create-ticket --ticket MO-02-MQJS-REPL-COMPONENTS ...`
  - `docmgr doc add --ticket MO-02-MQJS-REPL-COMPONENTS --doc-type analysis/design-doc/reference`
  - `docmgr task add --ticket MO-02-MQJS-REPL-COMPONENTS --text "..."`

## Step 2: Inventory existing components and prior art

I scanned the existing mqjs-repl sources plus prior ticket docs to capture where ReplLoop, JsEvaluator, and Spiffs live and how they are already described. This inventory highlights what should be extracted and the tickets that document constraints or failure modes.

The resulting analysis doc now lists component files and the most relevant diaries/design docs that already cover the REPL stack.

### What I did
- Searched `ttmp/` for references to ReplLoop, JsEvaluator, and Spiffs
- Listed existing component files and prior ticket docs in the analysis document

### Why
- Avoid repeating known information and ensure the extraction plan points to existing constraints

### What worked
- Found relevant prior docs in tickets `0014` and `0023`, plus SPIFFS autoload bug details in `0016`

### What didn't work
- N/A

### What I learned
- The earlier design doc already proposed componentizing ReplLoop/JsEvaluator/Spiffs

### What was tricky to build
- Keeping the prior-art list concise while still covering the key references

### What warrants a second pair of eyes
- Validate that no critical prior art was missed (especially around SPIFFS autoload failures)

### What should be done in the future
- N/A

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-02-MQJS-REPL-COMPONENTS--extract-microquickjs-repl-components/analysis/01-existing-components-and-prior-art.md`
- Cross-check the referenced prior docs and component paths

### Technical details
- Search command used:
  - `rg -n "ReplLoop|JsEvaluator|Spiffs" esp32-s3-m5/ttmp`

## Step 3: Draft component extraction design doc

I wrote a design document outlining how to split the REPL pieces into reusable ESP-IDF components, including dependencies and usage examples. The plan keeps responsibilities separated: REPL core, JS eval, and SPIFFS helpers.

The design doc now serves as the blueprint for implementation, with a clear component layout and example `idf_component_register` usage.

### What I did
- Authored the component extraction plan with layout, dependencies, and usage
- Documented design decisions, alternatives, and an implementation plan

### Why
- Provide a clear, reusable extraction blueprint before touching firmware code

### What worked
- The proposed layout aligns with existing interfaces (`IEvaluator`, `IConsole`) and current file structure

### What didn't work
- N/A

### What I learned
- Explicitly separating storage helpers simplifies reuse in non-JS firmware too

### What was tricky to build
- Balancing minimal dependencies against keeping the REPL core usable out of the box

### What warrants a second pair of eyes
- Confirm that the proposed component boundaries make sense for future firmware integration

### What should be done in the future
- Convert the implementation plan into concrete tasks once we start code changes

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-02-MQJS-REPL-COMPONENTS--extract-microquickjs-repl-components/design-doc/01-component-extraction-plan-replloop-jsevaluator-spiffs.md`
- Validate the component layout and usage example

### Technical details
- Proposed components: `mqjs_repl`, `mqjs_eval`, `mqjs_storage`
