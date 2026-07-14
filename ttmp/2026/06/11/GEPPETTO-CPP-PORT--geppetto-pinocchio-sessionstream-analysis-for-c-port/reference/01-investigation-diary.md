---
title: "Investigation Diary"
doc_type: reference
status: active
intent: long-term
topics: [geppetto, pinocchio, sessionstream, esp32, cpp, port, architecture]
---
# Investigation Diary

## Goal

Analyze the geppetto/pinocchio/sessionstream Go codebase to produce a comprehensive architecture analysis and C++ port guide for ESP32-S3 (M5StackChan). Read-only — no modifications to source repos.

## Step 1: Deep Architecture Analysis and Design Document

Read the complete source of geppetto, pinocchio, and sessionstream to understand the layered architecture. Produced a 500+ line design document covering all subsystems with file references, pseudocode, diagrams, and C++ port guidance.

### Prompt Context

**User prompt (verbatim):** "Let's create a new ticket, and let's analyze (do not modify) the geppetto implementation and framework (and its sessionstream companion) in ~/code/wesen/go-go-golems/{geppetto,pinocchio,sessionstream}. We want to potentially port that to C++ / use it at least as reference. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and then upload to remarkable. Keep a detailed diary as you work."

**Assistant interpretation:** Create a new docmgr ticket, deeply analyze the three Go repos (geppetto, pinocchio, sessionstream), produce an intern-ready technical guide with architecture diagrams, pseudocode, API references, file references, and C++ port guidance, store in ticket, upload to reMarkable.

**Inferred user intent:** Get a comprehensive reference document that can guide the C++ port of geppetto's inference framework to ESP32-S3, suitable for onboarding someone unfamiliar with the Go codebase.

### What I did

- Created docmgr ticket GEPPETTO-CPP-PORT with topics geppetto,pinocchio,sessionstream,esp32,cpp,port,architecture
- Created design doc and diary doc in ticket workspace
- Mapped file trees for all three repos (excluding ttmp, vendor, .git)
- Read go.mod for each repo to understand dependency graph
- Deep-read 15+ source files across all three repos:
  - Engine interface and types
  - Turn system (Turn, Block, Data, Metadata, typed keys)
  - Tool Loop (RunLoop algorithm, step controller)
  - Session management (Session, ExecutionHandle, EngineBuilder)
  - Middleware chain (HandlerFunc → Middleware → Chain)
  - Event system (canonical events, event sink, correlation)
  - Tool calling (ToolDefinition, ToolFunc with reflection, ToolRegistry, ToolExecutor)
  - SessionStream Hub (command dispatch, event publishing, projections, fanout)
  - SessionStream bus (Watermill integration, event envelope)
  - SessionStream handler (CommandHandler, EventPublisher)
  - Pinocchio ChatApp Engine (command → event bridging)
  - OpenAI engine implementation (SSE streaming, tool call handling)
  - Engine factory (provider selection)
  - Profile system (InferenceSettings, profile stacking)
- Wrote 500+ line design document with:
  - ASCII architecture diagram showing all three layers
  - 16 sections covering every subsystem
  - C++ pseudocode for each major component
  - Decision records for port choices
  - Complete file reference index
  - End-to-end flow walkthrough
  - FreeRTOS task layout for ESP32
  - Memory budget analysis
  - Priority-ordered port strategy table

### Why

The user wants to use geppetto as a reference architecture for implementing LLM inference with tool calling on ESP32-S3. A comprehensive analysis document serves as both the onboarding guide for a new intern and the blueprint for the C++ port.

### What worked

- Reading the source code directly was much more informative than reading READMEs or docs
- The Engine interface being a single method (RunInference) made the architecture immediately clear
- The Turn system's typed key approach (namespace.value@vN) is elegant and well-documented in the source
- SessionStream's Hub pattern (Command → Handler → Event → Projection → Fanout) is a clean CQRS/ES pattern that translates well to ESP32
- Starting with the file trees and go.mod gave me the dependency graph quickly

### What didn't work

- The geppetto file tree was enormous (200+ Go files) — had to focus on the core packages and skip edge cases
- The profile/bootstrap system has a lot of indirection (ProfileRegistry → SourceChain → StackMerge → InferenceSettings) that's hard to explain concisely
- Some files are very long (hub.go is 500+ lines, definition.go is 400+ lines) — had to read selectively

### What I learned

- **Geppetto's core is small**: Engine interface (1 method), Turn (3 fields + blocks), ToolLoop (1 loop), Session (1 mutex). The complexity is in the providers and configuration.
- **The Turn is the central abstraction**: Everything flows through Turns. Understanding the Block/Kind/Payload model is the key to understanding the whole system.
- **Tool calling via reflection is Go-specific**: The `NewToolFromFunc` pattern (auto-generating JSON schemas from Go function signatures) doesn't translate to C++. ESP32 tools will need explicit registration.
- **SessionStream is generic CQRS/ES**: It's not specific to chat or LLMs. It's a reusable event sourcing substrate. For ESP32, you'd extract just the Command → Event → Fanout pattern.
- **Pinocchio's ChatApp is the glue**: It bridges Geppetto's event system to SessionStream's proto-typed events. This is the pattern to replicate for ESP32's UI pipeline.
- **The profile system is overengineered for ESP32**: Profile stacking with YAML merging is powerful for a CLI app, but ESP32 needs a flat JSON config.
- **SSE streaming is the critical path**: All provider engines follow the same pattern: HTTP POST → SSE read → StreamReducer → Events. This is the first thing to implement in C++.

### What was tricky to build

- The architecture diagram needed to show all three repos in one view while making the data flow clear. Used layered ASCII boxes with the flow going top-to-bottom.
- Balancing depth vs. breadth — the document could easily be 2x longer if I covered every edge case. Focused on the "happy path" for the C++ port.
- The profile/bootstrap system has too many layers to explain concisely. Documented it at a high level and marked it as P2 for the port.

### What warrants a second pair of eyes

- The C++ pseudocode hasn't been compiled or tested — it's conceptual
- The FreeRTOS task layout assumes audio on Core 1 and inference on Core 0, which matches XiaoZhi's design but needs verification for the specific M5StackChan hardware
- The memory budget estimates are rough — need actual profiling on hardware
- The decision to skip protobuf in favor of JSON for ESP32 might have implications for the WebSocket transport layer

### What should be done in the future

- Actually implement the C++ Turn/Block types and test with ArduinoJson
- Implement the SSE parser for OpenAI Chat Completions
- Build a minimal Tool Loop with 2-3 hardware tools (servo, LED, display)
- Test the EventSink → FreeRTOS queue → LVGL pipeline
- Evaluate whether nlohmann::json or ArduinoJson is better for the Turn's Payload field
- Benchmark memory usage with real LLM responses on ESP32-S3

### Code review instructions

- Start with the design doc: `design-doc/01-geppetto-system-architecture-analysis-and-c-port-guide.md`
- Cross-reference with source files using the file index table in Section 16
- Focus on the Tool Loop (Section 5) and Event System (Section 8) — these are the core value
- Verify the C++ pseudocode in Sections 4, 5, 6, 7 for correctness
- Check the FreeRTOS task layout in Section 14 for Core 0/1 assignment correctness

### Technical details

- Ticket: GEPPETTO-CPP-PORT
- Design doc: 500+ lines, 50KB
- Source repos analyzed: geppetto (200+ Go files), pinocchio (150+ Go files), sessionstream (30+ Go files)
- Key insight: The entire geppetto inference framework boils down to: Turn → Engine.RunInference → ToolLoop → Events

## Step 2: Upload to reMarkable

Uploaded the design document bundle (design doc + diary) to reMarkable at `/ai/2026/06/11/GEPPETTO-CPP-PORT`.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Complete the reMarkable upload as specified in the original request.

**Inferred user intent:** Have the analysis document available on reMarkable for reading.

### What I did

- Ran `remarquee upload bundle --dry-run` to verify the upload plan
- Ran `remarquee upload bundle` with design doc + diary as a bundled PDF with ToC
- Verified the upload with `remarquee cloud ls`

### Why

The user explicitly asked for the document to be uploaded to reMarkable.

### What worked

- The bundle upload went smoothly — one PDF with table of contents
- remarquee status was healthy

### What didn't work

- N/A

### What I learned

- The design doc is 500+ lines — should render well as a PDF on reMarkable

### What was tricky to build

- Had to fix docmgr doctor issues first (missing frontmatter on diary, broken relative file paths for external Go source files, missing vocabulary entries)

### What warrants a second pair of eyes

- Verify the PDF renders correctly on reMarkable (diagrams, tables, code blocks)

### What should be done in the future

- Consider generating a more visual version with proper diagrams (Mermaid/PlantUML) for reMarkable

### Code review instructions

- Check reMarkable for the PDF at `/ai/2026/06/11/GEPPETTO-CPP-PORT`
- Verify the ToC is navigable

### Technical details

- Upload path: `/ai/2026/06/11/GEPPETTO-CPP-PORT`
- Bundle: design doc (50KB) + diary (6KB) → single PDF
