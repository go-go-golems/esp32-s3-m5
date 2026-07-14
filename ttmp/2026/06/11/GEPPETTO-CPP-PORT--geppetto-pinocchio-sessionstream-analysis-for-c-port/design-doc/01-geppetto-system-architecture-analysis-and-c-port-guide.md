---
title: "Geppetto System Architecture Analysis and C++ Port Guide"
doc_type: design-doc
status: active
intent: long-term
topics: [geppetto, pinocchio, sessionstream, esp32, cpp, port, architecture]
---

# Geppetto System Architecture Analysis and C++ Port Guide

## Executive Summary

Geppetto is a Go framework for building AI-powered applications with LLM inference, tool calling, and session management. It consists of three interconnected repositories:

- **geppetto** — The core inference framework: engines, turns, tools, events, profiles, middleware
- **pinocchio** — The application layer: CLI, TUI, web chat, chatapp domain, agent mode
- **sessionstream** — The event streaming substrate: command/event bus, projections, hydration, WebSocket transport

This document provides a deep architectural analysis of all three systems, explains how they compose, and provides a concrete C++ port guide for running similar capabilities on ESP32-S3 (specifically the M5StackChan hardware). Every concept is grounded in file references and source code.

---

## Table of Contents

1. [System Overview and Data Flow](#1-system-overview-and-data-flow)
2. [The Turn System (geppetto/pkg/turns)](#2-the-turn-system)
3. [The Engine Interface (geppetto/pkg/inference/engine)](#3-the-engine-interface)
4. [Tool Calling Architecture](#4-tool-calling-architecture)
5. [The Tool Loop (geppetto/pkg/inference/toolloop)](#5-the-tool-loop)
6. [Session Management (geppetto/pkg/inference/session)](#6-session-management)
7. [Middleware Chain (geppetto/pkg/inference/middleware)](#7-middleware-chain)
8. [Event System (geppetto/pkg/events)](#8-event-system)
9. [Provider Engines](#9-provider-engines)
10. [Profile and Configuration System](#10-profile-and-configuration-system)
11. [SessionStream Substrate](#11-sessionstream-substrate)
12. [Pinocchio ChatApp](#12-pinocchio-chatapp)
13. [End-to-End Flow: User Prompt to Streamed Response](#13-end-to-end-flow)
14. [C++ Port Strategy](#14-cpp-port-strategy)
15. [Decision Records](#15-decision-records)
16. [Reference: Key File Index](#16-reference-key-file-index)

---

## 1. System Overview and Data Flow

The three repositories form a layered architecture. SessionStream is the bottom layer — a generic event sourcing substrate. Geppetto sits in the middle — providing AI inference capabilities. Pinocchio is the top layer — a concrete chat application that wires everything together.

```
┌─────────────────────────────────────────────────────────────────┐
│  PINOCCHIO (Application Layer)                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │ CLI / TUI    │  │ Web Chat     │  │ Simple Chat Agent     │ │
│  │ (Cobra cmds) │  │ (HTTP + WS)  │  │ (Bubbletea)          │ │
│  └──────┬───────┘  └──────┬───────┘  └───────────┬───────────┘ │
│         │                 │                       │             │
│  ┌──────┴─────────────────┴───────────────────────┴──────────┐ │
│  │  ChatApp Engine (pkg/chatapp)                            │ │
│  │  - Command handlers: ChatStartInference, ChatStopInference│ │
│  │  - Event mapping: geppetto events → sessionstream events  │ │
│  │  - Plugins: reasoning, toolcall, widgets, agentmode      │ │
│  └──────────────────────────┬───────────────────────────────┘ │
│                             │                                   │
│  ┌─────────────────────────┴───────────────────────────────┐ │
│  │  Runner (pkg/cmds/run)                                  │ │
│  │  - Assembles Engine + Middleware + ToolRegistry + Session│ │
│  │  - Resolves profile → InferenceSettings → Engine        │ │
│  └──────────────────────────┬───────────────────────────────┘ │
└─────────────────────────────┼───────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────┐
│  GEPPETTO (Inference Framework)                               │
│  ┌──────────────────────────┴───────────────────────────────┐ │
│  │  Runner (pkg/inference/runner)                           │ │
│  │  - Prepare() → assemble Engine + Session + Tools         │ │
│  │  - Start() / Run() → execute inference                  │ │
│  └──────────┬────────────────────────────┬─────────────────┘ │
│             │                            │                    │
│  ┌──────────┴──────────┐  ┌──────────────┴────────────────┐ │
│  │  Tool Loop          │  │  Engine Interface              │ │
│  │  (pkg/inference/    │  │  (pkg/inference/engine)        │ │
│  │   toolloop)         │  │  - RunInference(ctx, Turn)     │ │
│  │  - Iterate:         │  │  - Implementations:            │ │
│  │    infer → tools →  │  │    OpenAI, Claude, Gemini,     │ │
│  │    execute → append │  │    OpenAI-Responses, Ollama    │ │
│  └──────────┬──────────┘  └───────────────────────────────┘ │
│             │                                                  │
│  ┌──────────┴──────────────────────────────────────────────┐ │
│  │  Turns (pkg/turns)                                      │ │
│  │  - Turn: ID + Blocks[] + Metadata + Data                │ │
│  │  - Block: ID + Kind + Role + Payload + Metadata         │ │
│  │  - Data: typed key-value store (namespace.value@vN)     │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                               │
│  ┌──────────────────────┐  ┌───────────────────────────────┐ │
│  │  Events (pkg/events) │  │  Middleware (pkg/inference/   │ │
│  │  - EventSink         │  │   middleware)                 │ │
│  │  - Canonical events  │  │  - Chain(before→after)        │ │
│  │  - Correlation IDs   │  │  - SystemPrompt, Logging,    │ │
│  │  - Registry/Codec    │  │    SinkWatermill, Reorder     │ │
│  └──────────────────────┘  └───────────────────────────────┘ │
└───────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────┐
│  SESSIONSTREAM (Event Streaming Substrate)                    │
│  ┌──────────────────────────┴───────────────────────────────┐ │
│  │  Hub (pkg/sessionstream)                                │ │
│  │  - Submit(command) → CommandHandler → EventPublisher    │ │
│  │  - projectAndApply(event) → UIProjection + TimelineProj │ │
│  │  - Fanout: publish UIEvents to WebSocket clients        │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                               │
│  ┌──────────────┐  ┌───────────────┐  ┌──────────────────┐  │
│  │ Schema       │  │ Hydration     │  │ Transport        │  │
│  │ Registry     │  │ Store         │  │ (WebSocket)      │  │
│  │ (protobuf)   │  │ (SQLite)      │  │ (gorilla/ws)     │  │
│  └──────────────┘  └───────────────┘  └──────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

The fundamental data flow is:

1. **User input** arrives at Pinocchio (CLI, TUI, or Web)
2. Pinocchio creates a **Command** (`ChatStartInference`) and submits it to SessionStream Hub
3. The Hub dispatches to a **CommandHandler** (ChatApp Engine)
4. ChatApp Engine builds a **Turn** from the user's prompt, then calls the **Runner**
5. The Runner assembles the **Engine** + **Middleware** + **ToolRegistry** + **Session**
6. The Session starts inference, which enters the **Tool Loop**
7. The Tool Loop calls `Engine.RunInference(ctx, turn)` which calls the LLM API
8. During inference, the Engine emits **Events** (text deltas, tool calls, etc.) to **EventSinks**
9. Events are bridged to SessionStream as proto-typed **Events** via the ChatApp runtime sink
10. SessionStream applies **Projections** (UI + Timeline) and **fans out** to WebSocket clients
11. The Tool Loop checks for pending tool calls, executes them, and loops

---

## 2. The Turn System

**Source:** `geppetto/pkg/turns/types.go`, `keys_gen.go`, `block_kind_gen.go`, `serde/serde.go`

The Turn is the central data structure in Geppetto. It represents a single inference interaction — a snapshot of the conversation state including all messages, tool calls, tool results, metadata, and application data.

### Turn Structure

```
Turn
├── ID: string                    # Unique identifier (UUID)
├── Blocks: []Block               # Ordered list of content blocks
│   └── Block
│       ├── ID: string            # Block UUID
│       ├── Kind: BlockKind       # Enum: user, assistant, tool_call, tool_result, system, reasoning...
│       ├── Role: string          # Role within the kind context
│       ├── Payload: map[string]any  # Arbitrary typed data
│       └── Metadata: BlockMetadata  # Typed key-value store
├── Metadata: Metadata            # Turn-level typed key-value store
└── Data: Data                    # Application data payload (typed key-value store)
```

The `Data` and `Metadata` fields use an opaque typed key system with namespaced keys in the format `namespace.value@vN` (e.g., `geppetto.tool_config@v1`). This provides versioned, collision-resistant key access.

**Key file references:**
- `geppetto/pkg/turns/types.go` — Turn, Block, Data, Metadata types
- `geppetto/pkg/turns/key_types.go` — TurnDataKey, TurnMetadataKey, BlockMetadataKey
- `geppetto/pkg/turns/keys_gen.go` — Generated key constants (KeyToolConfig, KeyToolDefinitions, etc.)
- `geppetto/pkg/turns/block_kind_gen.go` — Generated BlockKind enum
- `geppetto/pkg/turns/serde/serde.go` — YAML serialization/deserialization

### How Turns Flow Through Inference

When a user sends a prompt:

1. A new `Turn` is created (or the latest session turn is cloned)
2. A `user` block is appended with the prompt text
3. The Turn is passed to `Engine.RunInference(ctx, turn)`
4. The engine reads user blocks, builds an API request, calls the LLM
5. The engine appends `assistant` blocks (text, tool calls) and `reasoning` blocks to the Turn
6. If tool calls are present, the Tool Loop extracts them, executes the tools, and appends `tool_result` blocks
7. The updated Turn is passed back to the engine for the next iteration
8. The final Turn contains the complete conversation state

### C++ Port Considerations

- **BlockKind enum** → C++ `enum class BlockKind { User, Assistant, ToolCall, ToolResult, System, Reasoning }`
- **Data/Metadata** → Use `std::map<std::string, nlohmann::json>` or a custom typed key wrapper
- **Payload** → `nlohmann::json` or `ArduinoJson::JsonObject`
- **Clone** → Deep copy with smart pointers or arena allocation
- **YAML serialization** → Can be simplified to JSON on ESP32 (no YAML parser needed)

---

## 3. The Engine Interface

**Source:** `geppetto/pkg/inference/engine/engine.go`, `types.go`, `inference_config.go`

The Engine interface is beautifully simple:

```go
type Engine interface {
    RunInference(ctx context.Context, t *turns.Turn) (*turns.Turn, error)
}
```

That's it. One method. The engine takes a Turn, calls the LLM API, and returns the updated Turn. All provider-specific logic lives inside the engine implementation.

### Engine Configuration

Engines are configured through `InferenceSettings` which is resolved from the profile system:

```yaml
# Example profile (from examples/js/geppetto/profiles/10-provider-openai.yaml)
chat:
  api_type: "openai-chat"
  model: "gpt-4o-mini"
  temperature: 0.7
  max_tokens: 4096
  stream: true
api:
  api_key_env: "OPENAI_API_KEY"
```

The `InferenceSettings` struct (`geppetto/pkg/steps/ai/settings/settings-inference.go`) holds:

- `Chat` — Chat model settings (api_type, model, temperature, max_tokens, stream, system_prompt)
- `API` — API credentials (api_key_env, base_url)
- `Client` — HTTP client settings (timeout, retries)
- `Embeddings` — Embedding model settings

### Engine Factory

**Source:** `geppetto/pkg/inference/engine/factory/factory.go`

The `EngineFactory` interface creates the right engine based on `settings.Chat.ApiType`:

```go
type EngineFactory interface {
    CreateEngine(settings *settings.InferenceSettings) (engine.Engine, error)
    SupportedProviders() []string
    DefaultProvider() string
}
```

The `StandardEngineFactory` supports these API types:
- `"openai-chat"` → OpenAI Chat Completions engine
- `"openai-responses"` → OpenAI Responses API engine
- `"claude"` → Anthropic Claude Messages engine
- `"gemini"` → Google Gemini engine
- `"ollama"` → Ollama local engine (via OpenAI-compatible endpoint)

### C++ Port Considerations

The Engine interface translates directly to C++:

```cpp
class Engine {
public:
    virtual ~Engine() = default;
    virtual std::expected<std::unique_ptr<Turn>, Error> RunInference(
        const Context& ctx, const Turn& turn) = 0;
};
```

On ESP32, you'd implement:
- `OpenAIChatEngine` — HTTP POST to `/v1/chat/completions`, SSE stream parsing
- `ClaudeEngine` — HTTP POST to `/v1/messages`, SSE stream parsing
- `GeminiEngine` — HTTP POST to `/v1beta/models/{model}:generateContent`
- `OllamaEngine` — HTTP POST to local `/api/chat`

---

## 4. Tool Calling Architecture

**Source:** `geppetto/pkg/inference/tools/definition.go`, `executor.go`, `base_executor.go`, `registry.go`, `adapters.go`

Tool calling is the mechanism by which LLMs can invoke external functions. Geppetto implements the full tool calling lifecycle:

### ToolDefinition

A tool is defined by:
- `Name` — Unique identifier
- `Description` — Natural language description for the LLM
- `Parameters` — JSON Schema describing the expected input
- `Function` — The actual Go function to execute (wrapped in `ToolFunc`)
- `Examples`, `Tags`, `Version` — Optional metadata

### ToolFunc (The Clever Part)

**Source:** `geppetto/pkg/inference/tools/definition.go:47-130`

Geppetto uses Go reflection to automatically generate JSON schemas from Go function signatures. You register a tool like:

```go
tool, err := tools.NewToolFromFunc("get_weather", "Get current weather", GetWeather)
```

Where `GetWeather` could be:
- `func() (string, error)` — No input
- `func(input WeatherInput) (WeatherOutput, error)` — Struct input
- `func(ctx context.Context, input WeatherInput) (WeatherOutput, error)` — With context

The `NewToolFromFunc` function:
1. Validates the function signature using reflection
2. Generates a JSON Schema from the input struct type
3. Creates pre-compiled executors (`executor`, `executorCtx`) that deserialize JSON args and call the function
4. Caches the input/output types for validation

### ToolRegistry

**Source:** `geppetto/pkg/inference/tools/registry.go`

The registry maps tool names to `ToolDefinition`s:

```go
type ToolRegistry interface {
    Register(tool *ToolDefinition) error
    Lookup(name string) (*ToolDefinition, bool)
    ListTools() []*ToolDefinition
}
```

### ToolExecutor

**Source:** `geppetto/pkg/inference/tools/base_executor.go`

The executor handles the runtime execution of tool calls:

```go
type ToolExecutor interface {
    ExecuteToolCall(ctx context.Context, toolCall ToolCall, registry ToolRegistry) (*ToolResult, error)
    ExecuteToolCalls(ctx context.Context, toolCalls []ToolCall, registry ToolRegistry) ([]*ToolResult, error)
}
```

The `BaseToolExecutor` implements:
- Tool lookup from registry
- JSON argument deserialization
- Timeout handling
- Retry with exponential backoff
- Parallel execution of multiple tool calls
- Error classification (validation, execution, timeout, not_found)

### OpenAI Tool Adapter

**Source:** `geppetto/pkg/inference/tools/adapters.go`

The `OpenAIToolAdapter` converts between Geppetto's `ToolDefinition` format and the OpenAI API's tool format. Each provider has its own tool format — this adapter handles the translation.

### C++ Port Considerations

Go reflection doesn't exist in C++. For the ESP32 port, tool registration would be explicit:

```cpp
// No reflection — explicit schema + callback
tool_registry.register_tool({
    .name = "get_weather",
    .description = "Get current weather for a location",
    .parameters = R"({
        "type": "object",
        "properties": {
            "location": {"type": "string", "description": "City name"}
        },
        "required": ["location"]
    })"_json,
    .handler = [](const nlohmann::json& args) -> nlohmann::json {
        std::string location = args["location"];
        // Call weather API or read sensor
        return {{"temperature", 22}, {"condition", "sunny"}};
    }
});
```

This is actually simpler and more suitable for embedded — no reflection overhead, no dynamic type checking at runtime.

---

## 5. The Tool Loop

**Source:** `geppetto/pkg/inference/toolloop/loop.go`, `config.go`, `step_controller.go`

The Tool Loop is the orchestration engine that iterates between LLM inference and tool execution until the LLM stops requesting tools.

### Loop Algorithm

```
func (l *Loop) RunLoop(ctx, initialTurn):
    t = initialTurn
    
    # Inject tool config and definitions into turn data
    KeyToolConfig.Set(&t.Data, engineToolConfig)
    KeyToolDefinitions.Set(&t.Data, persistedToolDefinitions)
    
    for i = 0; i < maxIterations; i++:
        # 1. Call the LLM
        updated = l.eng.RunInference(ctx, t)
        
        # 2. Check for pending tool calls
        calls = ExtractPendingToolCalls(updated)
        if len(calls) == 0:
            return updated  # No tools → done
        
        # 3. Execute the tools
        results = l.executeTools(ctx, calls)
        
        # 4. Append tool results to the turn
        AppendToolResultsBlocks(updated, results)
        
        # 5. Loop back for the LLM to process tool results
        t = updated
    
    return error("max iterations reached")
```

### Step Controller (Human-in-the-Loop)

**Source:** `geppetto/pkg/inference/toolloop/step_controller.go`

The `StepController` enables pausing the tool loop at specific phases (after inference, after tool execution) for human review. This is used for agent debugging and safety:

- `StepPhaseAfterInference` — Pause after LLM responds, before tools execute
- `StepPhaseAfterTools` — Pause after tools execute, before next inference

When paused, a `DebuggerPauseEvent` is published. The controller waits until the pause is resolved or times out.

### C++ Port Considerations

The Tool Loop is the core value of Geppetto — it's the agentic loop. On ESP32, this would be:

```cpp
class ToolLoop {
public:
    struct Config {
        int max_iterations = 10;
        ToolConfig tool_config;
    };
    
    std::expected<std::unique_ptr<Turn>, Error> RunLoop(
        const Context& ctx, Turn& turn) {
        
        for (int i = 0; i < config_.max_iterations; i++) {
            auto result = engine_->RunInference(ctx, turn);
            if (!result) return result;
            
            auto calls = ExtractPendingToolCalls(*result);
            if (calls.empty()) return result;
            
            auto tool_results = ExecuteTools(ctx, calls);
            AppendToolResults(*result, tool_results);
            
            turn = *result;
        }
        return std::unexpected(Error{"max iterations"});
    }
};
```

On ESP32, you'd likely run this on one FreeRTOS task while the audio pipeline runs on another.

---

## 6. Session Management

**Source:** `geppetto/pkg/inference/session/session.go`, `execution.go`, `builder.go`, `context.go`

A `Session` represents a long-lived, multi-turn conversation. It owns:
- A stable `SessionID`
- An ordered list of `Turn`s (append-only snapshots)
- The invariant that only one inference is active at a time

### Key Operations

```go
// Append a new turn with user prompt
turn, err := session.AppendNewTurnFromUserPrompt("Hello!")

// Start inference asynchronously
handle, err := session.StartInference(ctx)

// Wait for completion
turn, err := handle.Wait()

// Cancel active inference
session.CancelActive()
```

### ExecutionHandle

**Source:** `geppetto/pkg/inference/session/execution.go`

The `ExecutionHandle` represents a single in-flight inference. It provides:
- `Cancel()` — Cancel the inference (via context)
- `Wait()` — Block until completion, return the output Turn
- `IsRunning()` — Check if still running

The handle uses a `done` channel and mutex for thread-safe completion signaling.

### EngineBuilder

**Source:** `geppetto/pkg/inference/session/builder.go`

The `EngineBuilder` interface decouples session construction from engine creation:

```go
type EngineBuilder interface {
    Build(ctx context.Context, sessionID string) (EngineRunner, error)
}

type EngineRunner interface {
    RunInference(ctx context.Context, input *turns.Turn) (*turns.Turn, error)
}
```

### C++ Port Considerations

On ESP32 with FreeRTOS:

```cpp
class Session {
    std::string session_id_;
    std::vector<Turn> turns_;
    TaskHandle_t inference_task_ = nullptr;
    SemaphoreHandle_t mutex_;
    
public:
    std::expected<Turn*, Error> AppendUserPrompt(const std::string& prompt);
    std::expected<ExecutionHandle*, Error> StartInference();
    Error CancelActive();
};
```

Use FreeRTOS task notifications or semaphores instead of Go channels for the ExecutionHandle.

---

## 7. Middleware Chain

**Source:** `geppetto/pkg/inference/middleware/middleware.go`, `systemprompt_middleware.go`, `logging_middleware.go`, `sink_watermill.go`

Middleware wraps the inference handler with additional functionality. The pattern is:

```go
type HandlerFunc func(ctx context.Context, t *turns.Turn) (*turns.Turn, error)
type Middleware func(HandlerFunc) HandlerFunc

func Chain(handler HandlerFunc, middlewares ...Middleware) HandlerFunc {
    // Apply in reverse order so they execute in correct order
    for i := len(middlewares) - 1; i >= 0; i-- {
        handler = middlewares[i](handler)
    }
    return handler
}
```

### Built-in Middlewares

1. **SystemPromptMiddleware** (`systemprompt_middleware.go`) — Injects a system prompt block into the Turn before inference
2. **LoggingMiddleware** (`logging_middleware.go`) — Logs before/after inference with timing
3. **SinkWatermillMiddleware** (`sink_watermill.go`) — Publishes inference events to a Watermill topic (bridges geppetto events to sessionstream)
4. **ReorderToolResultsMiddleware** (`reorder_tool_results_middleware.go`) — Reorders tool results to match the order of tool calls in the assistant's response

### C++ Port Considerations

Middleware translates directly:

```cpp
using HandlerFunc = std::function<std::expected<Turn, Error>(const Context&, const Turn&)>;
using Middleware = std::function<HandlerFunc(HandlerFunc)>;

HandlerFunc Chain(HandlerFunc handler, std::vector<Middleware> middlewares) {
    for (auto it = middlewares.rbegin(); it != middlewares.rend(); ++it) {
        handler = (*it)(handler);
    }
    return handler;
}
```

For ESP32, the most important middlewares are:
- System prompt injection (essential)
- Event sink (for UI updates)
- Logging (for debugging)

---

## 8. Event System

**Source:** `geppetto/pkg/events/canonical_events.go`, `chat-events.go`, `sink.go`, `registry.go`, `correlation.go`

The event system enables real-time observation of inference progress. Events are emitted by engines and consumed by sinks.

### Event Hierarchy

```
Event (interface)
├── Type() string
├── Metadata() EventMetadata
│   ├── ID: uuid
│   ├── SessionID: string
│   ├── InferenceID: string
│   ├── TurnID: string
│   └── Extra: map[string]any
└── CorrelatedEvent (interface)
    └── Correlation() Correlation
        ├── SessionID: string
        ├── InferenceID: string
        ├── ProviderCallIndex: int
        ├── TextSegmentIndex: int
        └── ToolCallIndex: int
```

### Canonical Events

These are the events emitted during a normal inference run, in order:

1. **RunStarted** — Inference started
2. **ProviderCallStarted** — API call to LLM provider started
3. **TextSegmentStarted** — Text response segment started
4. **TextDelta** — Streaming text delta (many of these)
5. **TextSegmentFinished** — Text response segment finished
6. **ReasoningSegmentStarted** — Reasoning/thinking segment started
7. **ReasoningDelta** — Streaming reasoning text
8. **ReasoningSegmentFinished** — Reasoning segment finished
9. **ProviderCallMetadataUpdated** — Usage/stop reason updated
10. **ProviderCallFinished** — API call finished (includes usage, stop reason, duration)
11. **RunFinished** — Inference completed

### Tool Events

12. **ToolCallStarted** — Tool execution started
13. **ToolArgumentsPatch** — Streaming tool arguments delta
14. **ToolCallRequested** — Tool call parsed from LLM response
15. **ToolExecutionStarted** — Tool function invocation started
16. **ToolResultReady** — Tool execution completed
17. **ToolCallFinished** — Tool call lifecycle complete

### EventSink

**Source:** `geppetto/pkg/events/sink.go`

```go
type EventSink interface {
    PublishEvent(event Event) error
}
```

Sinks are registered with the Runner and receive all events. The `NullEventSink` is the default (discards everything). Real sinks include WebSocket broadcasters, loggers, and UI updaters.

### Correlation IDs

**Source:** `geppetto/pkg/events/correlation.go`

Every event carries a `Correlation` struct that links it back to the originating inference, provider call, text segment, and tool call. This enables:
- Tracing an inference from start to finish
- Grouping streaming deltas by text segment
- Tracking tool call lifecycles across multiple events

### C++ Port Considerations

On ESP32, you'd simplify the event system:

```cpp
struct Event {
    EventType type;
    std::string session_id;
    std::string inference_id;
    nlohmann::json payload;
};

class EventSink {
public:
    virtual ~EventSink() = default;
    virtual void PublishEvent(const Event& event) = 0;
};
```

Use a FreeRTOS queue to buffer events between the inference task and the UI task.

---

## 9. Provider Engines

**Source:** `geppetto/pkg/steps/ai/openai/`, `claude/`, `gemini/`, `openai_responses/`

Each provider implements the `Engine` interface. Here's a summary of their architecture:

### OpenAI Chat Completions Engine

**Source:** `geppetto/pkg/steps/ai/openai/engine_openai.go`

- Builds request from Turn blocks (converts blocks to OpenAI message format)
- Always streams (even if profile says non-stream — forces `Stream: true`)
- Uses a `ChatStreamReducer` to accumulate SSE chunks into complete content
- Handles tool calls in the response, emitting ToolCallRequested events
- Supports function calling with parallel tool calls

### OpenAI Responses API Engine

**Source:** `geppetto/pkg/steps/ai/openai_responses/engine.go`

- Uses the newer Responses API (`/v1/responses`)
- Supports `previous_response_id` for conversation chaining
- Built-in tools (web search, file search, code interpreter)
- Different streaming event format (`response.output_item.done`, etc.)
- `StreamState` tracks the streaming state machine

### Claude Messages Engine

**Source:** `geppetto/pkg/steps/ai/claude/engine_claude.go`

- Uses Anthropic Messages API (`/v1/messages`)
- `max_tokens` is required (unlike OpenAI)
- System prompt is a top-level field
- Content blocks use Anthropic's format (text, tool_use, tool_result)
- `ContentBlockMerger` handles partial content block updates
- Supports thinking/reasoning blocks

### Gemini Engine

**Source:** `geppetto/pkg/steps/ai/gemini/engine_gemini.go`, `modern_engine.go`

- Uses Google's Generative AI API
- `ModernAdapter` converts between Gemini and Geppetto formats
- Supports function calling
- `StreamReducer` accumulates streaming chunks

### Stream Reduction Pattern

All engines follow a common pattern for handling SSE streams:

1. Make HTTP request with `Stream: true`
2. Read SSE events from the response body
3. Feed events into a `StreamReducer` that accumulates the complete response
4. Emit `TextDelta` / `ReasoningDelta` events for each chunk
5. When the stream ends, emit `TextSegmentFinished` / `ProviderCallFinished`

### C++ Port Considerations

For ESP32, the most important engine to port is OpenAI Chat Completions:

```cpp
class OpenAIChatEngine : public Engine {
    HTTPClient http_;
    EventSink* sink_;
    
    std::expected<std::unique_ptr<Turn>, Error> RunInference(
        const Context& ctx, const Turn& turn) override {
        
        // 1. Build JSON request from Turn blocks
        auto request = BuildRequest(turn);
        
        // 2. POST to /v1/chat/completions
        http_.begin("https://api.openai.com/v1/chat/completions");
        http_.addHeader("Authorization", "Bearer " + api_key_);
        http_.addHeader("Content-Type", "application/json");
        
        // 3. Read SSE stream
        auto* stream = http_.getStream();
        while (auto line = ReadSSELine(stream)) {
            if (line.starts_with("data: ")) {
                auto json = nlohmann::json::parse(line.substr(6));
                // 4. Extract delta content, emit events
                EmitDeltaEvents(json);
                // 5. Check for tool calls
                if (HasToolCalls(json)) {
                    // Append tool call blocks to turn
                }
            }
        }
        
        return UpdatedTurn();
    }
};
```

The ESP32 Arduino `HTTPClient` + `WiFiClientSecure` handles HTTPS. SSE parsing is straightforward line-by-line reading.

---

## 10. Profile and Configuration System

**Source:** `geppetto/pkg/engineprofiles/`, `geppetto/pkg/cli/bootstrap/`, `pinocchio/pkg/cmds/profilebootstrap/`

The profile system provides layered configuration resolution. Profiles are YAML files that stack on top of each other:

```
Base Profile (10-provider-openai.yaml)
  └── Team Profile (20-team-agent.yaml)
      └── User Overrides (30-user-overrides.yaml)
          └── Feature Flags (50-hardcut-phase123.yaml)
```

Each layer can override settings from previous layers. The `StackMerge` algorithm handles the merging logic.

### Key Types

- `InferenceSettings` — The fully resolved configuration (model, temperature, tools, etc.)
- `ProfileRegistry` — Discovers and loads profiles from multiple sources
- `ProfileRuntime` — Resolves a profile to a fully configured `Runtime`
- `SourceChain` — Ordered list of profile YAML sources

### C++ Port Considerations

On ESP32, profiles would be much simpler — likely a single JSON config stored in NVS or SPIFFS:

```cpp
struct InferenceConfig {
    std::string provider = "openai-chat";
    std::string model = "gpt-4o-mini";
    float temperature = 0.7f;
    int max_tokens = 1024;
    std::string api_key;
    std::string system_prompt;
    bool stream = true;
};
```

---

## 11. SessionStream Substrate

**Source:** `sessionstream/pkg/sessionstream/`

SessionStream is a generic event sourcing framework. It provides the infrastructure for command/event processing, projections, hydration, and WebSocket transport.

### Core Types

```
SessionId   = string    # Universal routing key
ConnectionId = string   # Transport-level connection
Command     { Name, Payload(proto.Message), SessionId }
Event       { Name, Payload(proto.Message), SessionId, Ordinal }
Session     { Id, Metadata }
```

### Hub (The Central Orchestrator)

**Source:** `sessionstream/pkg/sessionstream/hub.go`

The Hub is the entry point for all operations:

```
Hub
├── SchemaRegistry     # Validates command/event types
├── HydrationStore     # Persistence (SQLite or in-memory)
├── SessionRegistry    # Active sessions with metadata
├── CommandRegistry    # Registered command handlers
├── UIProjection       # Projects events → UI events
├── TimelineProjection # Projects events → timeline entities
├── UIFanout           # Broadcasts UI events to WebSocket clients
├── BusConfig          # Optional Watermill event bus
└── LocalOrdinal       # In-memory ordinal counter per session
```

### Command → Event Flow

```
1. Hub.Submit(sessionId, "ChatStartInference", payload)
2.  → CommandRegistry.Lookup("ChatStartInference")
3.  → CommandHandler(ctx, cmd, session, publisher)
4.    → Handler processes command, publishes Event
5.  → EventPublisher.Publish(event)
6.    → (local path) projectAndApply(event)
7.    → (bus path)   Watermill publish → consumer → projectAndApply
8.  → projectAndApply:
9.    → EventStore.AppendEvent(event)
10.   → UIProjection.Project(event, session, view) → UIEvents
11.   → TimelineProjection.Project(event, session, view) → Entities
12.   → HydrationStore.Apply(sessionId, ordinal, entities)
13.   → UIFanout.PublishUI(sessionId, ordinal, uiEvents)
14.   → WebSocket clients receive UIEvents
```

### Ordinals

Every event gets a monotonically increasing ordinal per session. This enables:
- Ordering guarantees for WebSocket delivery
- Cursor-based hydration (reconnect from last seen ordinal)
- Timeline replay (rebuild timeline from any point)

### Hydration Store

**Source:** `sessionstream/pkg/sessionstream/hydration/sqlite/store.go`

The SQLite hydration store persists:
- Events (for replay)
- Timeline entities (projected state)
- Cursors (per-session, per-projector)
- Errors (observed during processing)

On reconnect, the store provides a snapshot from the last cursor position.

### Schema Registry

**Source:** `sessionstream/pkg/sessionstream/schema.go`

The schema registry validates that commands and events use the correct protobuf message types. It prevents type confusion at the framework level.

### WebSocket Transport

**Source:** `sessionstream/pkg/sessionstream/transport/ws/server.go`

The WebSocket server:
- Accepts connections with a session ID
- Subscribes to the fanout for that session
- Sends UIEvents as protobuf-encoded messages
- On connect, hydrates from the store (sends missed events)
- Handles graceful shutdown and observer notifications

### C++ Port Considerations

For ESP32, you wouldn't port all of SessionStream. The relevant concepts are:

1. **Command → Event pattern** — Map user actions to commands, process them, emit events
2. **Ordinal tracking** — Number your events for ordering
3. **Fanout** — Multiple listeners for the same event stream
4. **Hydration** — Reconnect from last known state

On ESP32, this simplifies to:

```cpp
class EventHub {
    std::map<std::string, CommandHandler> handlers_;
    std::vector<std::function<void(const Event&)>> subscribers_;
    std::map<std::string, uint64_t> ordinals_;
    
public:
    void Submit(const std::string& session_id, 
                const std::string& command,
                const nlohmann::json& payload);
    
    void Subscribe(std::function<void(const Event&)> subscriber);
};
```

The WebSocket server would be the ESP32 acting as a **client** (connecting to a remote server like XiaoZhi), not a server.

---

## 12. Pinocchio ChatApp

**Source:** `pinocchio/pkg/chatapp/`

The ChatApp is the concrete chat domain built on top of SessionStream and Geppetto. It defines:

### Commands
- `ChatStartInference` — Start an inference run
- `ChatStopInference` — Cancel an active run

### Events
- `ChatUserMessageAccepted` — User message received
- `ChatRunStarted/Finished/Stopped/Failed` — Run lifecycle
- `ChatProviderCallStarted/Finished` — LLM API call lifecycle
- `ChatTextSegmentStarted/Patch/Finished` — Text streaming
- `ChatReasoningSegmentStarted/Patch/Finished` — Reasoning streaming
- `ChatToolCallStarted/ArgumentsPatch/Requested/ExecutionStarted/ResultReady/Finished` — Tool calling

### Plugins

ChatApp uses a plugin system for extensible event processing:

- **ReasoningPlugin** — Adds reasoning segment events to the UI event set
- **ToolCallPlugin** — Adds tool call events to the UI event set
- **WidgetsPlugin** — Adds widget events for progressive UI updates
- **AgentModePlugin** — Adds agent mode (structured output, debugger pauses)

### ChatApp Engine

**Source:** `pinocchio/pkg/chatapp/chat.go`

The ChatApp Engine is a SessionStream `CommandHandler` that:
1. Receives `ChatStartInference` commands
2. Creates a Geppetto `Session` and `Runner`
3. Starts inference with the user's prompt
4. Bridges Geppetto events to SessionStream events via `RuntimeSink`
5. Manages active runs (prevents double-start, supports cancellation)

### Runtime Sink

**Source:** `pinocchio/pkg/chatapp/runtime_sink.go`

The `RuntimeSink` implements Geppetto's `EventSink` interface and translates Geppetto canonical events into SessionStream proto-typed events:

```
Geppetto EventRunStarted    → SessionStream Event("ChatRunStarted", proto)
Geppetto EventTextDelta     → SessionStream Event("ChatTextPatch", proto)
Geppetto EventToolCallStarted → SessionStream Event("ChatToolCallStarted", proto)
...
```

This is the bridge between the two systems.

### C++ Port Considerations

The ChatApp layer is where the domain logic lives. On ESP32, this would be:

1. A simple command dispatcher
2. Event emission for the UI (LCD display)
3. The bridge between LLM responses and hardware actions (servos, LEDs, NFC)

---

## 13. End-to-End Flow: User Prompt to Streamed Response

Here's the complete flow when a user types a message in the web chat:

```
1. Browser sends WebSocket message: { command: "ChatStartInference", prompt: "Hello" }
2. Pinocchio WebSocket handler calls Hub.Submit(sessionId, "ChatStartInference", protoPayload)
3. Hub dispatches to ChatApp Engine's command handler
4. ChatApp Engine:
   a. Creates Geppetto Session, appends user prompt as a Turn
   b. Builds Runner with profile-resolved Engine + Middleware + Tools
   c. Registers RuntimeSink as EventSink
   d. Calls session.StartInference(ctx)
5. Session.StartInference:
   a. Gets latest Turn, assigns SessionID + InferenceID
   b. Builds EngineRunner via EngineBuilder
   c. Starts goroutine: runner.RunInference(ctx, turn)
6. Engine (e.g., OpenAI):
   a. Emits EventRunStarted
   b. Builds HTTP request from Turn blocks
   c. POSTs to OpenAI API with stream=true
   d. Reads SSE chunks:
      - Emits EventTextDelta for each chunk
      - Emits EventToolCallRequested for tool calls
   e. Emits EventProviderCallFinished
7. RuntimeSink receives each event:
   a. Translates to SessionStream proto event
   b. Calls Hub.Submit() or directly publishes
8. Hub.projectAndApply:
   a. Stores event in HydrationStore
   b. Projects to UI events and timeline entities
   c. Applies entities to store
   d. Fanouts UI events to WebSocket clients
9. Browser receives UI events via WebSocket:
   - ChatRunStarted → show loading
   - ChatTextPatch → append text
   - ChatToolCallStarted → show tool call
   - ChatRunFinished → hide loading
```

---

## 14. C++ Port Strategy

### What to Port (Priority Order)

| Priority | Component | Reason | Estimated Effort |
|----------|-----------|--------|------------------|
| P0 | Turn + Block types | Core data structure | 2 days |
| P0 | Engine interface + OpenAI Chat Completions | Basic LLM access | 3 days |
| P0 | Event system (simplified) | UI updates | 1 day |
| P0 | SSE stream parser | Required for streaming | 1 day |
| P1 | Tool Loop | Agentic behavior | 2 days |
| P1 | Tool Registry + Executor | Tool calling | 2 days |
| P1 | Session management | Multi-turn conversation | 1 day |
| P1 | Middleware (system prompt) | Essential configuration | 0.5 days |
| P2 | Claude engine | Provider diversity | 2 days |
| P2 | Gemini engine | Provider diversity | 2 days |
| P2 | Profile/config system | Flexible configuration | 1 day |
| P3 | SessionStream concepts | Event sourcing | 3 days |
| P3 | WebSocket transport | Remote control | 2 days |

### Architecture for ESP32

```
┌──────────────────────────────────────────────┐
│  M5StackChan Application                    │
│  ┌────────────────────────────────────────┐ │
│  │  Geppetto-CPP (Inference Framework)    │ │
│  │  ┌──────────┐  ┌────────────────────┐ │ │
│  │  │ Engine   │  │ Tool Loop          │ │ │
│  │  │(OpenAI/  │  │ - iterate infer    │ │ │
│  │  │ Claude/  │  │   → execute tools  │ │ │
│  │  │ Gemini)  │  │   → append results │ │ │
│  │  └──────────┘  └────────────────────┘ │ │
│  │  ┌──────────┐  ┌────────────────────┐ │ │
│  │  │ Turn     │  │ EventSink          │ │ │
│  │  │ Blocks   │  │ - FreeRTOS queue   │ │ │
│  │  │ Data     │  │ - UI task consumer │ │ │
│  │  └──────────┘  └────────────────────┘ │ │
│  │  ┌──────────┐  ┌────────────────────┐ │ │
│  │  │ Tool     │  │ Session            │ │ │
│  │  │ Registry │  │ - Turn history     │ │ │
│  │  │ Executor │  │ - Active inference │ │ │
│  │  └──────────┘  └────────────────────┘ │ │
│  └────────────────────────────────────────┘ │
│                                              │
│  ┌────────────────────────────────────────┐ │
│  │  Hardware Abstraction                  │ │
│  │  - Servo control (servo_tool)          │ │
│  │  - LED control (led_tool)              │ │
│  │  - Display output (lcd_tool)           │ │
│  │  - NFC reader (nfc_tool)               │ │
│  │  - IR transmitter (ir_tool)            │ │
│  │  - Sensor reading (sensor_tool)        │ │
│  └────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

### FreeRTOS Task Layout

```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ Audio Task   │  │ Inference    │  │ UI Task      │
│ (Core 1)     │  │ Task (Core 0)│  │ (Core 0)     │
│              │  │              │  │              │
│ - Mic input  │  │ - HTTP/SSE   │  │ - LVGL       │
│ - OPUS enc   │  │ - Tool loop  │  │ - Event      │
│ - Speaker    │  │ - Turn mgmt  │  │   consumer   │
│ - OPUS dec   │  │              │  │ - Display    │
└──────────────┘  └──────────────┘  └──────────────┘
       ↑                 ↑                  │
       │                 │                  │
       │    FreeRTOS Queue (events)         │
       └────────────────────────────────────┘
```

### Memory Budget

ESP32-S3 with 8MB PSRAM:
- Turn history: 2-4 recent Turns in PSRAM (~10-50KB each with tool results)
- Event queue: 32-64 events × ~1KB = 32-64KB in PSRAM
- JSON buffers: ArduinoJson with PSRAM allocator, 16-32KB per buffer
- Tool registry: Static, ~1KB
- HTTP client: ~8KB stack + SSL buffer (~40KB)

### Key Simplifications for ESP32

1. **No protobuf** — Use JSON directly (protobuf is overkill for ESP32)
2. **No Watermill/event bus** — Use FreeRTOS queues directly
3. **No YAML** — Use JSON config from SPIFFS/NVS
4. **No reflection** — Explicit tool registration with JSON schema strings
5. **No SQLite** — Use SPIFFS files or NVS for persistence
6. **No profile stacking** — Single flat config object
7. **No sessionstream Hub** — Simplified EventHub with FreeRTOS queue

---

## 15. Decision Records

### DR-001: Port Engine interface directly, not via abstraction layers

**Context:** Geppetto has many layers (factory, profile, bootstrap, middleware config). On ESP32, most of these add complexity without value.

**Options:**
1. Port the full layered architecture
2. Port only the Engine + Turn + ToolLoop + Session

**Decision:** Option 2. Port the core data structures and inference loop. Configuration is a flat struct. The factory is a simple switch on provider name.

**Rationale:** ESP32 has limited memory and no need for dynamic profile stacking. The core value is in the Turn/Block model, the Tool Loop, and the Event system.

**Consequences:** Less flexibility, but much simpler code. Can add layers later if needed.

### DR-002: Use nlohmann::json for C++ JSON handling

**Context:** Geppetto uses Go maps and interface{} for dynamic data. C++ needs a JSON library.

**Options:**
1. nlohmann::json — Full-featured, header-only
2. ArduinoJson — Lightweight, ESP32-optimized
3. RapidJSON — Fast, but complex API

**Decision:** ArduinoJson for the ESP32 build (memory-efficient, PSRAM support). nlohmann::json for desktop testing.

**Rationale:** ArduinoJson is the standard for ESP32, has PSRAM allocator support, and handles the limited memory budget well.

**Consequences:** Two JSON APIs to maintain, but ArduinoJson is close enough to nlohmann::json that abstractions are minimal.

### DR-003: FreeRTOS queue for event delivery

**Context:** Geppetto uses Go channels and the EventSink interface. ESP32 needs an equivalent.

**Options:**
1. FreeRTOS queue — Built-in, efficient, no allocation
2. FreeRTOS stream buffer — Byte-oriented, not suitable for structs
3. Custom ring buffer with PSRAM — More flexible, more code

**Decision:** FreeRTOS queue for small events (type + session_id + small payload), with larger data passed via PSRAM-allocated pointers.

**Rationale:** Queues are the natural ESP32 equivalent of Go channels. For larger payloads (text deltas, tool results), allocate in PSRAM and pass the pointer through the queue.

**Consequences:** Need careful memory management (who frees the PSRAM allocation?). Use shared_ptr with PSRAM allocator.

---

## 16. Reference: Key File Index

### Geppetto Core

| File | Purpose |
|------|---------|
| `geppetto/pkg/turns/types.go` | Turn, Block, Data, Metadata types |
| `geppetto/pkg/turns/keys_gen.go` | Generated typed key constants |
| `geppetto/pkg/turns/block_kind_gen.go` | Generated BlockKind enum |
| `geppetto/pkg/turns/serde/serde.go` | YAML serialization |
| `geppetto/pkg/turns/toolblocks/toolblocks.go` | Tool call/result block helpers |
| `geppetto/pkg/inference/engine/engine.go` | Engine interface |
| `geppetto/pkg/inference/engine/types.go` | ToolDefinition, ToolConfig, ToolCall, ToolResult |
| `geppetto/pkg/inference/engine/factory/factory.go` | EngineFactory, StandardEngineFactory |
| `geppetto/pkg/inference/runner/run.go` | Runner.Start(), Runner.Run() |
| `geppetto/pkg/inference/runner/types.go` | Runtime, StartRequest, PreparedRun |
| `geppetto/pkg/inference/runner/prepare.go` | Runner.Prepare() |
| `geppetto/pkg/inference/toolloop/loop.go` | Tool Loop (RunLoop) |
| `geppetto/pkg/inference/toolloop/config.go` | LoopConfig |
| `geppetto/pkg/inference/toolloop/step_controller.go` | Human-in-the-loop pauses |
| `geppetto/pkg/inference/session/session.go` | Session management |
| `geppetto/pkg/inference/session/execution.go` | ExecutionHandle |
| `geppetto/pkg/inference/session/builder.go` | EngineBuilder interface |
| `geppetto/pkg/inference/middleware/middleware.go` | Middleware chain |
| `geppetto/pkg/inference/tools/definition.go` | ToolDefinition, ToolFunc, NewToolFromFunc |
| `geppetto/pkg/inference/tools/executor.go` | ToolExecutor interface |
| `geppetto/pkg/inference/tools/base_executor.go` | BaseToolExecutor |
| `geppetto/pkg/inference/tools/registry.go` | ToolRegistry interface |
| `geppetto/pkg/inference/tools/adapters.go` | OpenAI tool adapter |
| `geppetto/pkg/events/canonical_events.go` | Canonical event types |
| `geppetto/pkg/events/chat-events.go` | Chat-specific events |
| `geppetto/pkg/events/sink.go` | EventSink interface |
| `geppetto/pkg/events/registry.go` | Event codec registry |
| `geppetto/pkg/events/correlation.go` | Correlation IDs |

### Provider Engines

| File | Purpose |
|------|---------|
| `geppetto/pkg/steps/ai/openai/engine_openai.go` | OpenAI Chat Completions engine |
| `geppetto/pkg/steps/ai/openai/chat_stream.go` | SSE stream reader |
| `geppetto/pkg/steps/ai/openai/chat_stream_reducer.go` | Stream accumulation |
| `geppetto/pkg/steps/ai/openai_responses/engine.go` | OpenAI Responses API engine |
| `geppetto/pkg/steps/ai/openai_responses/streaming.go` | Responses API streaming |
| `geppetto/pkg/steps/ai/claude/engine_claude.go` | Claude Messages engine |
| `geppetto/pkg/steps/ai/claude/api/streaming.go` | Claude SSE streaming |
| `geppetto/pkg/steps/ai/claude/content-block-merger.go` | Content block accumulation |
| `geppetto/pkg/steps/ai/gemini/engine_gemini.go` | Gemini engine |
| `geppetto/pkg/steps/ai/gemini/modern_engine.go` | Modern Gemini adapter |
| `geppetto/pkg/steps/ai/gemini/stream_reducer.go` | Gemini stream accumulation |
| `geppetto/pkg/steps/ai/settings/settings-inference.go` | InferenceSettings |
| `geppetto/pkg/steps/ai/settings/settings-chat.go` | Chat-specific settings |
| `geppetto/pkg/steps/ai/settings/settings-client.go` | HTTP client settings |

### Profile System

| File | Purpose |
|------|---------|
| `geppetto/pkg/engineprofiles/types.go` | Profile types |
| `geppetto/pkg/engineprofiles/registry.go` | ProfileRegistry |
| `geppetto/pkg/engineprofiles/stack_merge.go` | Profile stacking/merging |
| `geppetto/pkg/engineprofiles/service.go` | Profile service |
| `geppetto/pkg/cli/bootstrap/profile_registry.go` | CLI profile loading |
| `geppetto/pkg/cli/bootstrap/profile_runtime.go` | Profile → Runtime resolution |

### SessionStream

| File | Purpose |
|------|---------|
| `sessionstream/pkg/sessionstream/types.go` | Core types (SessionId, Command, Event) |
| `sessionstream/pkg/sessionstream/hub.go` | Hub (central orchestrator) |
| `sessionstream/pkg/sessionstream/handler.go` | CommandHandler, EventPublisher |
| `sessionstream/pkg/sessionstream/bus.go` | Watermill bus adapter |
| `sessionstream/pkg/sessionstream/consumer.go` | Event consumer |
| `sessionstream/pkg/sessionstream/schema.go` | SchemaRegistry |
| `sessionstream/pkg/sessionstream/projection.go` | UIProjection, TimelineProjection |
| `sessionstream/pkg/sessionstream/fanout.go` | UIFanout |
| `sessionstream/pkg/sessionstream/ordinals.go` | Ordinal tracking |
| `sessionstream/pkg/sessionstream/hydration/sqlite/store.go` | SQLite hydration store |
| `sessionstream/pkg/sessionstream/transport/ws/server.go` | WebSocket server |
| `sessionstream/proto/sessionstream/v1/transport.proto` | Transport protobuf |

### Pinocchio

| File | Purpose |
|------|---------|
| `pinocchio/pkg/chatapp/chat.go` | ChatApp Engine |
| `pinocchio/pkg/chatapp/runner.go` | Geppetto Runner integration |
| `pinocchio/pkg/chatapp/runtime_sink.go` | Geppetto → SessionStream event bridge |
| `pinocchio/pkg/chatapp/messages.go` | Proto message types |
| `pinocchio/pkg/chatapp/projections.go` | UI/Timeline projections |
| `pinocchio/pkg/chatapp/plugins/reasoning.go` | Reasoning plugin |
| `pinocchio/pkg/chatapp/plugins/toolcall.go` | Tool call plugin |
| `pinocchio/pkg/chatapp/plugins/widget.go` | Widget plugin |
| `pinocchio/pkg/chatapp/frontendtools/` | Frontend tool bridge |
| `pinocchio/pkg/chatapp/rpc/jsonl/` | JSONL RPC output |
| `pinocchio/pkg/chatapp/serverkit/` | HTTP server kit |
| `pinocchio/pkg/persistence/chatstore/` | SQLite turn persistence |
| `pinocchio/cmd/web-chat/` | Web chat application |
| `pinocchio/proto/pinocchio/chatapp/` | ChatApp protobuf schemas |
