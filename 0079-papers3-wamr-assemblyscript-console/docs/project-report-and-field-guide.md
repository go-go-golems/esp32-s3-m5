# PaperS3 WAMR AssemblyScript Console

## Project Report and Field Guide

This document is the long-form technical report for the `0079-papers3-wamr-assemblyscript-console` experiment.

It is written for a future reader who wants more than a changelog. The goal is to preserve the theory, the architecture, the implementation choices, the debugging path, the wrong turns, and the parts that were never fully resolved. If you read this two years from now, you should be able to reconstruct both the technical system and the engineering judgment around it.

This report is intentionally detailed. It mixes:

- architectural explanation
- project chronology
- debugging postmortem
- lessons learned
- API references
- file references
- pseudocode
- diagrams

If you want the short version, it is this:

- We built a PaperS3 firmware prototype that embeds precompiled AssemblyScript programs as WebAssembly modules.
- The device exposes those modules through `esp_console` over USB Serial/JTAG.
- The firmware can initialize WAMR, enumerate embedded modules, and execute at least some minimal Wasm modules.
- A host-only replay path can render the `hello-frame` demo successfully on the real PaperS3.
- A WAMR-backed path can run simple Wasm modules, but after WAMR execution, later replay of PaperS3 display commands can panic with cache-related failures.
- We isolated the failure far enough to say that plain WAMR execution on the console task is sufficient to destabilize later display replay.
- We did not fully root-cause the underlying runtime/platform interaction.

## Why This Project Exists

The original product idea was straightforward and attractive:

- write small visual AssemblyScript programs
- compile them into `.wasm`
- embed them in PaperS3 firmware
- expose a simple console like `wasm run hello-frame`
- let the guest draw through a tiny host API

That idea is appealing for several reasons:

- it separates content from firmware
- it creates a constrained sandbox for demo logic
- it makes the console a live experimentation tool
- it creates a reusable path for future scripting experiments

It is also exactly the sort of idea that looks simple at the product level while hiding a lot of embedded-runtime complexity underneath.

## The Three Ticket Story

This work really happened across three tickets, even though the code converged in `0079`.

### Ticket 1: `ESP-36`

Planning ticket:

- create the initial analysis and design guide
- define the PaperS3 + WAMR + AssemblyScript direction
- choose `ESP-IDF 5.3.4`
- choose USB Serial/JTAG for the console
- define the first milestone as interpreter-first and precompiled assets

### Ticket 2: `ESP-37`

Implementation and first execution-primitives ticket:

- scaffold `0079`
- add console bring-up
- add WAMR runtime service
- add the AssemblyScript build pipeline
- embed a demo registry
- add first host execution primitives
- discover several runtime and hardware crashes

### Ticket 3: `ESP-38`

Replay-isolation debugging ticket:

- add a WAMR-free control path
- prove that host-only replay can work
- compare pre-cleanup vs post-cleanup replay timing
- add minimal non-display Wasm probes
- instrument execution state
- conclude that plain WAMR execution is enough to poison later display replay

The report you are reading now is the narrative that ties those tickets together.

## System Overview

The project root is:

- [0079 project](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console)

The most important source files are:

- [README.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/README.md)
- [main/wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- [main/wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)
- [main/wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- [main/wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
- [main/wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- [main/papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
- [main/wasm_module_registry.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp)
- [tools/build-wasm-demos.mjs](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/tools/build-wasm-demos.mjs)

### High-Level Stack

```text
+---------------------------------------------------+
| User                                              |
| USB Serial/JTAG console                           |
| command: wasm run hello-frame                     |
+------------------------------+--------------------+
                               |
                               v
+---------------------------------------------------+
| esp_console command layer                         |
| main/wasm_command.cpp                             |
+------------------------------+--------------------+
                               |
                               v
+---------------------------------------------------+
| Wasm module lookup and execution                  |
| main/wasm_module_registry.cpp                     |
| main/wasm_module_runner.cpp                       |
+------------------------------+--------------------+
                               |
                               v
+---------------------------------------------------+
| WAMR runtime                                      |
| load / instantiate / create exec env / call       |
+------------------------------+--------------------+
                               |
                               v
+---------------------------------------------------+
| Host imports                                      |
| main/wasm_host_api.cpp                            |
| queue commands instead of touching display live   |
+------------------------------+--------------------+
                               |
                               v
+---------------------------------------------------+
| Replay and canvas                                |
| main/wasm_host_api.cpp                            |
| main/papers3_canvas.cpp                           |
+------------------------------+--------------------+
                               |
                               v
+---------------------------------------------------+
| M5Unified / M5GFX / PaperS3 EPD                   |
+---------------------------------------------------+
```

### Build-Time Asset Flow

```text
+---------------------------+
| wasm-src/<demo>/          |
| AssemblyScript sources    |
+-------------+-------------+
              |
              v
+---------------------------+
| tools/build-wasm-demos.mjs|
| asc compile               |
+-------------+-------------+
              |
              v
+---------------------------+
| wasm-build/release/*.wasm |
| wasm-build/release/*.wat  |
+-------------+-------------+
              |
              v
+---------------------------+
| main/wasm-assets/*.wasm   |
| embedded in firmware      |
+---------------------------+
```

## Theory: What Each Part Is Supposed to Do

### 1. AssemblyScript

AssemblyScript is a TypeScript-like language that compiles to WebAssembly. In this project, it is used as a narrow authoring layer for tiny demos.

Why it was attractive:

- syntax is friendly for quick experiments
- toolchain is simple enough for host-side compilation
- generated Wasm modules can export a plain `run()` function

Example demos:

- `hello-frame`
- `nested-boxes`
- `bars`
- `checkerboard`
- `radar-sweep`
- later debug probes:
  - `return-42`
  - `log-only`

Relevant guest sources:

- [wasm-src/hello-frame/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/hello-frame/assembly/index.ts)
- [wasm-src/return-42/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/return-42/assembly/index.ts)
- [wasm-src/log-only/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/log-only/assembly/index.ts)

### 2. WebAssembly Micro Runtime (WAMR)

WAMR is the embedded runtime that loads, instantiates, and executes the `.wasm` modules.

The important API layer is the standard WAMR C API:

- `wasm_runtime_full_init(...)`
- `wasm_runtime_load(...)`
- `wasm_runtime_instantiate(...)`
- `wasm_runtime_lookup_function(...)`
- `wasm_runtime_create_exec_env(...)`
- `wasm_runtime_call_wasm(...)`
- `wasm_runtime_call_wasm_a(...)`
- `wasm_runtime_destroy_exec_env(...)`
- `wasm_runtime_deinstantiate(...)`
- `wasm_runtime_unload(...)`
- `wasm_runtime_get_exception(...)`

Theory of operation:

- the runtime needs a working allocator
- the module must be valid Wasm
- the host needs to register native symbols for imports
- execution happens inside an execution environment
- results and exceptions are returned through WAMR APIs

### 3. `esp_console`

The user-facing control surface is `esp_console`.

Why it matters:

- it creates a quick operator loop
- it avoids hardcoding one demo path
- it lets us compare behaviors with single commands

Important console choices:

- use USB Serial/JTAG, not normal UART
- keep commands text-based and inspectable
- always have status and info commands, not only run commands

That decision turned out to be correct. A lot of the debugging only became tractable because commands like these existed:

- `wasm status`
- `wasm list`
- `wasm info hello-frame`
- `wasm replay hello-frame`
- `wasm run-preflush hello-frame`
- `wasm run hello-frame`

### 4. Host Imports and the Queue

The guest needs a host ABI. Early on, the natural design temptation was to let Wasm imports directly touch the display.

That turned out to be a mistake.

The more robust design was:

- host import callbacks only record intent
- drawing commands are queued in a host-side frame buffer
- replay happens later in a normal host context

That queue boundary is conceptually important because it separates:

- guest execution
- host display mutation

In principle this should reduce risk. In practice it also made the later isolation experiment possible.

Host import categories:

- logging
- delays
- clear screen
- draw rectangle
- fill rectangle
- present frame

### 5. PaperS3 Display Stack

The physical display path is not a toy. It goes through the M5 stack and ultimately reaches the e-paper panel implementation.

The important part is that this is a cache-sensitive hardware path, not just an abstract frame buffer API.

Observed crash stacks repeatedly passed through functions like:

- `lgfx::v1::Panel_EPD::writeFillRectPreclipped(...)`
- `lgfx::v1::LGFXBase::fillRect(...)`
- `papers3_wasm::PaperCanvasScreenClear(...)`

That matters because it means the failure expresses itself in the display replay, even when the underlying cause may have happened earlier.

## The Intended Happy Path

If the project had worked cleanly, the main run path would look like this:

```text
user types "wasm run hello-frame"
-> console resolves module descriptor
-> runtime status is checked
-> host API status is checked
-> host frame is reset
-> WAMR loads module bytes
-> WAMR instantiates module
-> runner looks up export "run"
-> runner creates exec env
-> Wasm executes
-> host imports queue drawing commands
-> runner flushes queued commands to PaperS3
-> display updates successfully
-> operator sees a frame and a clean success report
```

Pseudocode:

```text
function run_command(name):
    module = registry.find(name)
    require runtime.ready
    require host_api.ready

    reset_host_frame()
    reset_canvas_frame()

    wasm_module = wasm_runtime_load(module.bytes)
    module_inst = wasm_runtime_instantiate(wasm_module)
    function = wasm_runtime_lookup_function(module_inst, "run")
    exec_env = wasm_runtime_create_exec_env(module_inst)

    wasm_runtime_call_wasm(exec_env, function)

    flush_host_frame_to_display()

    destroy exec_env
    deinstantiate module_inst
    unload wasm_module
```

That is what we wanted. The actual story was messier.

## Chronology of What We Built

### Phase 1: Scaffold the Firmware

We created `0079` as a dedicated project instead of burying the experiment inside a busier firmware.

That was good engineering hygiene because it kept the problem small:

- one board family
- one console model
- one runtime experiment

Early wins:

- bring up PaperS3
- standardize the USB Serial/JTAG console
- add a `wasm` command namespace
- add a runtime status command

### Phase 2: Add the AssemblyScript Pipeline

We added:

- `wasm-src/`
- `package.json`
- `npm install`
- `npm run build`
- a release asset sync into firmware

This was one of the cleaner parts of the project. The host-side build pipeline did what it was supposed to do.

What this gave us:

- repeatable demo builds
- embedded `.wasm` files
- human-readable `.wat` output for inspection

### Phase 3: Embed the Module Registry

We added a descriptor registry so the firmware can know:

- module name
- binary location
- binary size
- entrypoint

This enabled:

- `wasm list`
- `wasm info <name>`

Those commands were not glamorous, but they were exactly the kind of observability that paid off later.

### Phase 4: Add Execution Primitives

We added the first host API functions and the first runnable Wasm path.

At this point the system could:

- initialize WAMR
- register host symbols
- find embedded modules
- invoke exports

But the system was still fragile. Several classes of failure started to appear as soon as real drawing was involved.

## Where We Went Wrong

This section matters more than the success list.

### Mistake 1: We Underestimated the Integration Boundary

At the idea level, the project sounds like:

- compile Wasm
- run Wasm
- draw a few rectangles

On embedded hardware, the real boundary was:

- custom allocator behavior
- execution environment semantics
- host import ABI correctness
- cache-sensitive display operations
- ESP32-S3 task and interrupt state

We initially treated the problem like application integration. It was really runtime-platform integration.

### Mistake 2: We Let the Early Milestones Drift Toward “End-to-End”

A common failure mode in experiments is chasing the demo too early.

Instead of insisting on tiny verified layers like:

- run a pure arithmetic Wasm function
- run a pure logging Wasm function
- run a host-only replay path
- only then attempt display rendering

we were initially pulled toward the more satisfying end-to-end picture:

- `wasm run hello-frame`

That made progress feel faster than it really was.

### Mistake 3: Direct Host Side Effects Were Too Optimistic

Direct display mutation inside host callbacks is the simplest design to imagine, but it is the worst place to debug from.

Why it was bad:

- guest execution and hardware mutation happen in one mixed context
- failures are harder to localize
- the runtime boundary becomes harder to reason about

Moving to a queue-and-replay model was the right correction, but it should probably have been the original design.

### Mistake 4: We Did Not Add Minimal Probe Modules Early Enough

The eventual probe modules:

- `return-42`
- `log-only`

were extremely valuable.

They should have existed earlier because they answer questions like:

- can WAMR execute anything at all?
- can host imports work without display traffic?
- is failure specific to rendering or more general?

These tiny modules produced more clarity than some of the more ambitious demo paths.

### Mistake 5: We Assumed “Crash in Display Code” Meant “Display Bug”

That is a subtle but important reasoning trap.

The replay crash stack showed display functions near the top. That did not mean the display path was the origin of the bug. It only meant display operations were where the bad state became observable.

This is a classic post-boundary failure pattern:

- corruption or unstable state occurs earlier
- a later hardware-sensitive operation trips over it
- the stack trace points at the victim, not the cause

### Mistake 6: We Stayed in the Rabbit Hole Slightly Too Long

The user called this out explicitly, and they were right.

There is a point where debugging effort stops compounding. Once the investigation had proven:

- control replay works
- WAMR-backed replay fails
- preflush still fails
- non-display Wasm can still poison later replay
- obvious interrupt-state counters look normal

the next steps became low-level runtime archaeology. That is exactly where rabbit holes begin.

The right response at that point is a decision boundary, not unlimited digging.

## The Debugging Story

### Step 1: Host-Only Replay Baseline

We added:

- [main/wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)

This path mirrored the `hello-frame` guest behavior without entering WAMR at all.

The purpose was very precise:

- prove whether the queue and replay path can succeed on hardware by itself

Result:

- `wasm replay hello-frame` succeeded

Meaning:

- the queue/replay/display path is not independently broken

This was one of the best experiments in the whole project because it sharply reduced ambiguity.

### Step 2: Compare Wasm Path Against Control Path

We then ran:

- `wasm replay hello-frame`
- `wasm run hello-frame`

Result:

- control path succeeded
- Wasm path panicked

Representative panic:

```text
Guru Meditation Error: panic'ed (Cache disabled but cached memory region accessed)
Write back error occurred while dcache tries to write back to flash
...
lgfx::v1::Panel_EPD::writeFillRectPreclipped(...)
papers3_wasm::PaperCanvasScreenClear(...)
papers3_wasm::FlushWasmHostFrame(...)
papers3_wasm::RunEmbeddedWasmModuleOnCurrentThread(...)
```

Interpretation:

- replay itself can work
- entering and returning from WAMR changes something important

### Step 3: Test Pre-Cleanup vs Post-Cleanup Replay

We added `wasm run-preflush <name>`.

The logic was:

- maybe WAMR execution is fine
- maybe cleanup and teardown are the real source of corruption

So we changed timing:

- old path: replay after cleanup
- new path: replay before cleanup

Pseudocode:

```text
call wasm
flush queued frame
destroy exec env
deinstantiate module
unload module
```

Result:

- `run-preflush hello-frame` still crashed

Interpretation:

- teardown alone is not required for the crash
- the bad state can already exist immediately after the Wasm call returns

This was another decisive experiment.

### Step 4: Add Tiny Probe Modules

We introduced:

- `return-42`
- `log-only`

Why:

- `hello-frame` still included display semantics
- smaller probes let us divide the space cleanly

Results:

- `wasm run-preflush return-42` succeeded
- `wasm run-preflush log-only` succeeded
- after a successful `return-42`, a later host-only `wasm replay hello-frame` could still crash

This is one of the most important findings of the whole project.

It means:

- plain WAMR execution can be enough to poison later replay
- display imports are not required for the destabilization

That is a much stronger statement than “display imports are buggy.”

### Step 5: Instrument Execution State

We added execution-state probes around the WAMR call in:

- [main/wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)

The probes printed:

- core ID
- task handle
- cycle count
- processor status register
- interrupt level
- ISR context flag
- scheduler running state
- interrupt nesting
- critical nesting
- old interrupt state
- stack high-water mark

Result on successful `return-42`:

- no obvious leaked ISR state
- no obvious critical nesting leak
- no obvious interrupt level leak

Interpretation:

- the corruption is subtler than “we forgot to re-enable interrupts”

This did not solve the bug, but it prevented us from wasting more time on a weak hypothesis.

## The Two Most Important Theories

### Theory A: The Replay Path Is Broken

This theory became weaker after the control-path experiment.

Evidence against:

- host-only `wasm replay hello-frame` works

What remains possible:

- replay code may still be sensitive
- replay may still be a victim of prior state corruption

But replay alone is not sufficient to explain the failure.

### Theory B: WAMR Execution Leaves the Task or Platform in a Bad State

This is the strongest current model.

Evidence for:

- the control path works
- the Wasm path fails
- preflush still fails
- minimal Wasm probes can poison later replay

What remains unknown:

- exact mechanism
- whether the issue is in WAMR core, Xtensa native-call bridging, allocator behavior, cache state, or an ESP32-S3 integration detail

## Best Current Mental Model

If you need one sentence, use this one:

> The system behaves as though WAMR execution perturbs platform state in a way that later PaperS3 display replay can detect, even when the Wasm guest did not itself perform display imports.

That is not the same thing as a root cause. It is the best current operational model.

### Why the Model Matters

A good debugging model helps you decide what not to do.

Given the current model, poor next steps would be:

- endlessly rewriting PaperS3 drawing helpers
- adding more complex demo scenes
- assuming the queue design itself is the root problem

Better next steps are:

- bounded A/B with the official Espressif WAMR component
- tighter platform-level instrumentation
- or pausing the runtime experiment and using a safer execution model

## Why the Rabbit Hole Happened

There are structural reasons this kind of work spirals.

### 1. The Demo Path Is Emotionally Compelling

`wasm run hello-frame` is a satisfying goal. Once it almost works, it is easy to keep digging.

### 2. Embedded Runtime Bugs Cross Layers

This project crosses:

- TypeScript-like source code
- Wasm code generation
- runtime loading
- native symbol registration
- ESP-IDF task context
- M5 display calls
- e-paper hardware behavior

That is enough layers that each “small” bug can masquerade as another.

### 3. Backtraces Feel More Decisive Than They Really Are

When you see:

- `Panel_EPD::writeFillRectPreclipped`

you naturally think:

- “the display code is broken”

But when the crash only occurs after runtime execution, the display call may just be the first sensitive operation after corruption.

## The Official Espressif WAMR Component Question

An important late discovery was that Espressif now publishes an official WAMR component through the ESP Component Registry.

Primary sources:

- https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0~1
- https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/dependencies?language=en
- https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/examples/esp-idf

Important nuance:

- this is official support
- it is not the same thing as WAMR being a built-in ESP-IDF core component

Does this change the debugging theory?

- not immediately

Does it change the strategy?

- yes

Why:

- if we keep going, an A/B trial against the official Espressif packaging becomes a high-value comparison
- but it should be a bounded experiment, not an uncontrolled in-place migration

## Key Implementation Paths

### Command Surface

- [main/wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)

Responsibilities:

- parse `wasm` subcommands
- validate module names
- choose replay vs WAMR execution
- print runtime and execution results

### Runtime Bring-Up

- [main/wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)

Responsibilities:

- initialize WAMR
- track ready/not-ready status
- expose runtime capability reporting

### Module Registry

- [main/wasm_module_registry.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp)

Responsibilities:

- define embedded module descriptors
- support `find`, `list`, and `info`

### Runner

- [main/wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)

Responsibilities:

- load module bytes
- instantiate and create exec env
- call export
- record result
- choose preflush or post-cleanup replay timing
- print execution probes

### Host ABI and Replay

- [main/wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
- [main/papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)

Responsibilities:

- register native symbols
- queue guest intent
- replay the command frame later
- surface host API status

### Control-Path Baseline

- [main/wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)

Responsibilities:

- provide a Wasm-free baseline
- mirror guest behavior as literally as possible
- test replay without runtime involvement

## Commands Worth Remembering

Build firmware:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py build
```

Flash and monitor:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

Build Wasm demos:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src
npm install
npm run build
```

Useful console probes:

```text
wasm status
wasm list
wasm info hello-frame
wasm replay hello-frame
wasm run-preflush return-42
wasm run-preflush log-only
wasm run-preflush hello-frame
wasm run hello-frame
```

## Review Map for a Future Engineer

If you need to re-enter this project efficiently, read in this order:

1. [README.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/README.md)
2. [main/wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
3. [main/wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
4. [main/wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
5. [main/wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
6. [main/papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
7. [tools/build-wasm-demos.mjs](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/tools/build-wasm-demos.mjs)
8. The ticket docs under `ESP-38`

## Decision Tree: What Should Happen Next

### Option A: One Bounded A/B Experiment

Try the official Espressif WAMR component in a controlled branch or ticket.

Good because:

- high information value
- relatively clean hypothesis test

Rules:

- do not mix with unrelated cleanups
- keep the test matrix small
- stop after one or two sessions if the signal is weak

### Option B: Pause Runtime Work

Keep the console and embedded-module architecture, but stop trying to run them on-device for now.

Good because:

- avoids more runtime archaeology
- preserves most of the useful project structure

### Option C: Keep Digging Into the Current Integration

This is the least attractive option unless there is a very specific new lead.

Why:

- the next layer down is low-level runtime/platform behavior
- effort can grow quickly without guaranteed signal

## What We Actually Achieved

Even though the Wasm display path is unresolved, the project still produced valuable assets.

We now have:

- a clean PaperS3 firmware scaffold
- USB Serial/JTAG console integration
- WAMR runtime bring-up
- AssemblyScript build tooling
- embedded demo registry
- operator-facing status and info commands
- host ABI queue design
- a replay control baseline
- minimal probe modules
- a strong isolation result for the failure
- a documented stopping point instead of vague frustration

That is real progress. It is just not the same as “the demo shipped.”

## Final Lessons

### Lesson 1

On embedded systems, “scripting support” is never just about the script language. The real work is at the runtime and hardware boundary.

### Lesson 2

A control path that mirrors the real path without one suspect subsystem is often more valuable than another round of direct patching.

### Lesson 3

Tiny probe programs are first-class debugging tools. Add them early.

### Lesson 4

A backtrace tells you where the system failed, not necessarily where the bad state began.

### Lesson 5

The right time to stop is part of engineering quality. Rabbit holes are not conquered by morale. They are contained by decision boundaries.

## Appendix: Milestone Commits

Representative commits in the project story:

- `35bffdb` `feat(papers3): add wamr runtime service`
- `68efe8c` `feat(papers3): add assemblyscript demo pipeline`
- `41a04d8` `feat(papers3): embed wasm demo registry`
- `1d6ebf2` `feat(papers3): add wamr execution primitives`
- `39edb29` `fix(papers3): isolate wasm host side effects`
- `86bcf3c` `feat(papers3): add replay isolation control path`
- `f86bfd5` `debug(papers3): compare preflush wasm replay timing`
- `b8d3dcb` `debug(papers3): add minimal wasm replay probes`
- `473a64e` `debug(papers3): instrument wasm execution state`

## Appendix: Related Ticket Documents

The ticket copy of this report and the surrounding investigation docs live under:

- [ESP-38 index](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION--papers3-post-wamr-display-replay-isolation-control-path/index.md)
- [ESP-38 implementation plan](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION--papers3-post-wamr-display-replay-isolation-control-path/design/01-replay-isolation-implementation-plan.md)
- [ESP-38 postmortem](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION--papers3-post-wamr-display-replay-isolation-control-path/design/02-wamr-replay-isolation-postmortem-report.md)
- [ESP-38 diary](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION--papers3-post-wamr-display-replay-isolation-control-path/reference/01-diary.md)

If you have to choose one thing to preserve from this work, preserve the clarity of the debugging boundaries. That clarity is what turns a failed demo into reusable engineering knowledge.
