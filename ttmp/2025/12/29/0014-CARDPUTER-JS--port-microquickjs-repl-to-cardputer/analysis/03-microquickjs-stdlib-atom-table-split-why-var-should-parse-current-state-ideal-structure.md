---
Title: 'MicroQuickJS stdlib / atom-table split: why var should parse, current state, ideal structure'
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Defines JSSTDLibraryDef/JS_NewContext and eval flags central to stdlib/keyword behavior
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.h
      Note: Stdlib generator description DSL (JSPropDef/JSClassDef) used to build atom tables and builtins
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_priv.h
      Note: Defines JS_ROM_VALUE and ROM table expectations; highlights word-size/layout concerns
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h
      Note: Host-generated stdlib blob containing keyword atoms; generated as 64-bit by default and not usable on ESP32 as-is
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib_simple.c
      Note: Stdlib composition file showing CONFIG_MINIMAL_STDLIB approach; reference-only (not built by ESP-IDF)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/legacy/minimal_stdlib.h
      Note: Legacy firmware-selected empty stdlib; explains missing keyword atoms and parse symptoms in pre-fix builds
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-31T20:17:32.611433518-05:00
WhatFor: ""
WhenToUse: ""
---


# MicroQuickJS stdlib / atom-table split: why `var` should parse, current state, ideal structure

This document explains a confusing-but-critical aspect of MicroQuickJS (“mquickjs”) as embedded in this repo:

- The “standard library” is not just “built-in objects like `Math`”.
- In MicroQuickJS, the stdlib definition can also effectively control whether core language tokens (like `var`) are recognized as *keywords* during parsing.

This mattered immediately because the legacy firmware’s autoload example scripts begin with `var ...`, yet early QEMU logs showed parse errors like `expecting ';' at 1:5`. That error is consistent with `var` being tokenized as a normal identifier rather than a keyword.

The goal here is to make this understandable for a new engineer and to outline what the project should look like “in an ideal world”, including how we should generate and select an appropriate stdlib for ESP32 (32-bit) vs host (64-bit).

## Executive summary

- In MicroQuickJS, the `JSSTDLibraryDef` passed to `JS_NewContext()` provides:
  - a ROM “stdlib table” blob (`stdlib_table`) that includes the atom table and global object layout,
  - a C function table (`c_function_table`) for JS-callable native functions,
  - a finalizer table (`c_finalizer_table`) for user classes.
  See `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h:245`.

- The legacy firmware used an “empty stdlib” (`imports/esp32-mqjs-repl/mqjs-repl/legacy/minimal_stdlib.h:1`) which contains essentially *no atoms*.

- If the keyword atoms (like `"var"`, `"function"`, `"return"`) are missing, the parser can treat those words as ordinary identifiers. That makes code like `var x = 1;` parse like `identifier identifier ...` and fail with a syntax error that *looks* unrelated.

- There is a large generated stdlib header (`imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:1`) that *does* contain keyword atoms, but it was not usable on ESP32 as-is because it was host-generated as 64-bit.

- Crucially: the checked-in `esp_stdlib.h` appears to be generated for a **64-bit host** (it declares `uint64_t js_stdlib_table[]` at `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:5`). ESP32-S3 is a **32-bit** target where MicroQuickJS uses 32-bit `JSWord` (`imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h:49`). Mixing those is incorrect without regenerating a 32-bit table.

## The pieces: what files exist and what they do

### 1) The legacy firmware entry point used a minimal stdlib

The embedded firmware lives in:

- `imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c:1`

Key line:

- `imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c:261` creates the context with:
  - `JS_NewContext(js_mem_buf, JS_MEM_SIZE, &js_stdlib);`
  - and `js_stdlib` comes from `imports/esp32-mqjs-repl/mqjs-repl/legacy/minimal_stdlib.h:14`.

`minimal_stdlib.h` defines:

- `js_stdlib_table` containing a single `0` word (effectively empty),
- `js_c_function_table[]` empty,
- `js_c_finalizer_table[]` empty,
- `class_count = JS_CLASS_USER` (no user classes).

Net: you get “just enough runtime structure to allocate a context and evaluate very small expressions”, but you are not guaranteed a full JavaScript language experience.

### 2) The “real” stdlib is table-driven and usually generated

MicroQuickJS is designed such that its built-in objects and many “engine constants” are compiled into a ROM-like blob. In this repo, the generator-facing API lives here:

- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.h:1`

This file defines the macros used to describe objects/classes and C-callable functions:

- `JS_CFUNC_DEF(...)`, `JS_PROP_STRING_DEF(...)`, `JS_CLASS_DEF(...)`, etc.

At a high level, you write “descriptions” (arrays of `JSPropDef` / `JSClassDef`), and the build utility creates:

- an atom table (interned strings)
- a global object graph
- a `c_function_table` mapping function indices → function pointers
- a `c_finalizer_table` mapping user class IDs → finalizers

Those are then wrapped in:

```c
typedef struct {
  const JSWord *stdlib_table;
  const JSCFunctionDef *c_function_table;
  const JSCFinalizer *c_finalizer_table;
  uint32_t stdlib_table_len;
  uint32_t stdlib_table_align;
  uint32_t sorted_atoms_offset;
  uint32_t global_object_offset;
  uint32_t class_count;
} JSSTDLibraryDef;
```

See `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h:245`.

### 3) There are multiple “stdlib-ish” artifacts in this repo, and they’re easy to confuse

In `imports/esp32-mqjs-repl/mqjs-repl/main/`:

- `minimal_stdlib.h`
  - used today by firmware
  - tiny, but not a full language environment

- `esp_stdlib.h`
  - “automatically generated - do not edit”
  - contains a large atom table (including keyword strings like `"var"`)
  - defines a non-empty `js_c_function_table` including symbols like `print`, `gc`, `Date.now`, etc.
  - declares `uint64_t js_stdlib_table[]` at `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:5`
  - defines `const JSSTDLibraryDef js_stdlib = {...}` at `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:2673`

- `esp_stdlib.c` / `esp_stdlib_simple.c`
  - appear to be host-side “stdlib composition” sources that include `mqjs_stdlib.c`
  - `esp_stdlib_simple.c` defines `CONFIG_MINIMAL_STDLIB` then `#include "mqjs_stdlib.c"` (`imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib_simple.c:10`)
  - these are not currently compiled by ESP-IDF because `main/CMakeLists.txt` only lists `main.c` (`imports/esp32-mqjs-repl/mqjs-repl/main/CMakeLists.txt:1`)

Also in `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/`:

- `mqjs.c`
  - a host REPL program (not used in firmware)
  - includes `mqjs_stdlib.h` and uses “full stdlib” patterns (`imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mqjs.c:223`)

- `mqjs_stdlib.c`
  - a large set of standard library “definitions” (`JSPropDef` tables etc.) used to build a complete JS environment on host
  - not currently compiled into the ESP-IDF `mquickjs` component (it’s not listed in `components/mquickjs/CMakeLists.txt:2`)

Bottom line: there are *three different stories* present (embedded minimal, host full, and “ESP-flavored full”), but only the embedded minimal one is wired into the actual firmware build.

## The core concept: atoms, keywords, and why `var` matters

### What is an “atom”?

MicroQuickJS uses an “atom table” to intern strings and represent identifiers efficiently.

Conceptually:

- The source code contains lots of repeated strings: `"length"`, `"constructor"`, `"toString"`, `"var"`, etc.
- Instead of storing/handling these as repeated heap strings, the engine stores a canonical copy (“atom”) and uses an integer ID or pointer to refer to it.

In this implementation, the stdlib table includes an atom table stored in ROM-ish memory. You can see this explicitly at the top of `esp_stdlib.h`:

- `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:6` begins with `/* atom_table */`
- and soon after includes strings like `"var"` at `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:19`.

### Why would keyword recognition depend on atoms?

Many engines treat keywords as “hard-coded” tokens, independent of runtime library.

MicroQuickJS is optimized for embedded footprint and uses a table-driven approach. One plausible (and observed-by-symptom) implementation strategy is:

1) The lexer reads an identifier-like token (`[A-Za-z_][A-Za-z0-9_]*`).
2) It interns the token as an atom (`atom = atomize("var")`).
3) The parser checks whether this atom is one of the “keyword atoms” (`atom == ATOM_var`, etc.).

If the keyword atoms are not present/initialized, step (3) cannot succeed, and `"var"` is treated as a normal identifier.

### Why does that produce `expecting ';' at 1:5`?

Consider the first line of the shipped example library:

```js
var MathUtils = {};
```

If `var` is incorrectly tokenized as an identifier, the parser sees:

```text
IDENT(var) IDENT(MathUtils) '=' '{' '}' ';'
```

In JavaScript grammar, a statement starting with an identifier is usually an expression statement:

```text
ExpressionStatement:
  Expression ';'
```

But `IDENT(var) IDENT(MathUtils)` without an operator in between is invalid, so the parser often complains at the point it expected the expression to end (a `;`) but found another identifier instead.

That makes the error message look like “missing semicolon”, but the real cause is “keyword not recognized”.

This explains why the QEMU autoload parse error is completely consistent with “stdlib/atom table is too minimal”.

## Current state summary: what is broken and why

### Observed behavior

- Autoload library load attempts fail with:
  - `expecting ';' at 1:5` in ticket `0016-SPIFFS-AUTOLOAD`.

### Likely cause (most consistent with evidence)

- Firmware uses `minimal_stdlib.h` which does not include keyword atoms.
- Therefore `var` is not recognized as keyword.
- Therefore any script using keywords (`var`, `function`, `return`, `if`, ...) may not parse.

### Secondary complication: 32-bit vs 64-bit generated stdlib

Even if we decide “just use `esp_stdlib.h`”, we need to be careful:

- The ESP32-S3 target is 32-bit, so MicroQuickJS uses 32-bit `JSWord` (`imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h:49`).
- The checked-in `esp_stdlib.h` declares `uint64_t js_stdlib_table[]` (`imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:5`).

This strongly suggests that `esp_stdlib.h` was generated on a 64-bit host with `JS_PTR64` enabled and is not necessarily the correct ROM table layout for a 32-bit embedded target.

In other words:

- `minimal_stdlib.h` is tiny and “fits” the target representation, but lacks necessary atoms.
- `esp_stdlib.h` likely contains the right atoms, but may be the wrong word-size layout unless regenerated for 32-bit.

## Where the 64-bit `esp_stdlib.h` comes from (repo-local evidence)

This repo already includes the host-side stdlib generator binary:

- `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib_gen`

It is an x86-64 ELF, i.e. a host tool, not an ESP32 binary. On this machine it reports as:

- `ELF 64-bit ... x86-64` (confirmed via `file`).

The generator’s “main program” in source form is:

- `imports/esp32-mqjs-repl/mqjs-repl/main/mqjs_stdlib.c:398`

It ends with:

```c
int main(int argc, char **argv)
{
    return build_atoms("js_stdlib", js_global_object, js_c_function_decl, argc, argv);
}
```

And `build_atoms(...)` is implemented in:

- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c:833`

That function prints the generated header to stdout, including this line:

- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c:901`

```c
printf("static const uint%u_t __attribute((aligned(%d))) js_stdlib_table[] = {\n",
       JSW * 8, ATOM_ALIGN);
```

So the word size of the output table is not “mysterious”: it is controlled by `JSW`.

On a 64-bit host, `build_atoms` defaults `JSW` to 8 (because `INTPTR_MAX >= INT64_MAX`), so running the generator without flags produces:

- `static const uint64_t ... js_stdlib_table[]`

That is exactly what we see in the checked-in:

- `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h:5`

## How to generate a 32-bit stdlib header (and why this is the right lever)

Good news: MicroQuickJS already supports generating 32-bit ROM tables from a 64-bit host build. The generator explicitly documents and parses `-m32` / `-m64`:

- Usage text: `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c:822`
- Argument parsing: `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c:846`

So we do not need to cross-compile the generator to 32-bit; we can ask the generator to emit a 32-bit table layout.

### Suggested workflow to create an ESP32-safe header

From the generator directory:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl/main
./esp_stdlib_gen -m32 > esp32_stdlib.h
```

What you should see at the top of the generated file:

- `static const uint32_t ... js_stdlib_table[] = {`

This matters because:

- The `JS_ROM_VALUE(offset)` macro in `mquickjs_priv.h` takes offsets in units of `JSWord` (`imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_priv.h:57`).
- The offsets (`sorted_atoms_offset`, `global_object_offset`, and all `JS_ROM_VALUE(N)` references) are computed by `build_atoms` and must be computed with the same `JSW` the target engine uses.

### Integration options (project policy decision)

Once generated, we can:

1) Replace the current checked-in `esp_stdlib.h` with a 32-bit version (and regenerate it deterministically), or
2) Add a second header (`esp_stdlib_esp32.h` / `esp32_stdlib.h`) and make the firmware select which one to use.

Given that `minimal_stdlib.h` currently exists for footprint reasons, option (2) tends to be safer initially:

- keep `minimal_stdlib.h` for “expression-only bring-up”
- add `esp32_stdlib.h` for “real JS parsing” runs
- add a Kconfig/sdkconfig toggle to choose between them per build profile

## How this situation likely came to be (project archaeology)

The codebase appears to have grown from multiple upstream contexts:

1) **Upstream MicroQuickJS host REPL**
   - Files like `components/mquickjs/mqjs.c` and `components/mquickjs/mqjs_stdlib.c` look like a classic host REPL implementation:
     - build a context with a relatively complete stdlib
     - provide `print`, timers, etc.

2) **An “ESP flavored stdlib” effort**
   - `tools/esp_stdlib_gen/esp_stdlib.c` implements time and `print/gc` and then includes `mqjs_stdlib.c` to assemble a stdlib that makes sense on ESP.
   - `tools/esp_stdlib_gen/esp_stdlib_simple.c` defines `CONFIG_MINIMAL_STDLIB` to disable some features (Date/timers/load) and include a smaller set.

3) **An embedded “minimal stdlib” shortcut**
   - `legacy/minimal_stdlib.h` is explicitly described as “just enough to initialize the engine” and is table-empty.
   - This is consistent with early-stage “get something running” work where only expressions like `1+2` were needed.

4) **Generated headers committed from a host environment**
   - `tools/esp_stdlib_gen/esp_stdlib.h` is a generated artifact that looks like it came from a host build (64-bit table).
   - A prebuilt `tools/esp_stdlib_gen/esp_stdlib_gen` binary exists in the repo, which is unusual for source control and further hints at a host-generated pipeline being “checked in” rather than integrated properly into ESP-IDF build steps.

Result: the project contains artifacts from “host REPL land” and “embedded minimal land” without a single, clean story for “how stdlib is built and chosen” on the actual target.

## What it should look like in an ideal world

The “ideal” is not about one specific file, but about having clean separations and a reproducible way to build a stdlib that is:

- correct for the target word size (32-bit for ESP32-S3),
- small enough for embedded constraints,
- complete enough to parse real JavaScript (keywords at minimum),
- extensible enough to add native bindings (ESP.* APIs).

### Principle 1: Keyword recognition should not be optional

In normal JavaScript, `var` is a reserved keyword. The engine must recognize it regardless of whether the runtime includes `Math`, `Date`, or any other library object.

Therefore, for our project:

- Any “minimal stdlib” we ship must still include:
  - keyword atoms (`var`, `function`, `return`, `if`, `else`, ...)
  - any atom infrastructure the parser needs to classify tokens

We should treat “keyword atoms” as a **core engine dependency**, not a “nice to have stdlib feature”.

### Principle 2: Separate “language core” from “builtins”

We can conceptualize three layers:

1) **Language core atoms** (needed to parse JS)
   - keywords
   - punctuators (not atoms)
   - minimal intrinsic strings the parser/compiler needs

2) **Runtime builtins** (needed to execute useful JS)
   - `Object`, `Array`, `Function`, `Number`, `String`, etc.
   - maybe `Math`, `JSON`

3) **Platform APIs** (project-specific)
   - `ESP.*` functions for GPIO, display, keyboard, filesystem, etc.

MicroQuickJS packages these into one `JSSTDLibraryDef`, but we should build and reason about them as “sub-layers” so we can trim responsibly.

### Principle 3: Stdlib generation must be target-aware and reproducible

Given the 32-bit vs 64-bit mismatch risk, we should make it explicit:

- A stdlib generated on a 64-bit host is not automatically valid for a 32-bit embedded target.

Ideal options:

**Option A (recommended): generate a 32-bit stdlib header for ESP32 and commit it**

- Provide a script to generate `esp32_stdlib.h` deterministically.
- Ensure the generator emits 32-bit `JSWord` layout.
- Keep host-generated artifacts separate (`host_stdlib.h`) if needed for tooling.

**Option B: change the generator to emit a word-size-independent format**

- e.g. emit `uint8_t blob[]` with explicit endianness/packing, then have the engine parse it.
- This is a larger engine change and likely not worth it right now.

### A concrete “ideal structure” for this repo

One reasonable end state:

```text
main/js/
  stdlib/
    mqjs_stdlib_defs.c          # "definitions" (JSPropDef tables) shared conceptually
    esp32_stdlib_min.h          # generated 32-bit: keywords + tiny builtins
    esp32_stdlib_full.h         # generated 32-bit: more builtins + platform hooks
  engine/
    MicroQuickJsEngine.{h,cpp}  # wraps JSContext and memory arena
```

And in code:

- Development/QEMU: start with `esp32_stdlib_min.h` + `RepeatEvaluator` → prove REPL I/O → then `JsEvaluator`.
- Later: move to `esp32_stdlib_full.h` when we start adding platform APIs and want real JS scripts.

### What “minimal but correct” likely means for us

At minimum, we want the REPL to accept:

- `var x = 1;`
- `function f(){...}`
- `if (...) { ... }`

So “minimal but correct” must include the keyword atoms for those tokens.

It likely should also include enough runtime to not immediately crash on common expressions:

- objects and arrays
- basic number/string operations

If we truly want “expression-only mode”, we should be honest and call it that (and avoid printing banners suggesting `MathUtils` libraries exist).

## Practical recommendations (what we should do next)

These are ordered from “improves clarity immediately” to “bigger refactors”.

1) **Stop conflating “minimal stdlib” with “valid JavaScript”**
   - Document that `minimal_stdlib.h` is expression-only unless we add keyword atoms.
   - Avoid autoloading scripts that require keywords when running with minimal stdlib.

2) **Introduce a “keywords-only” stdlib as a stepping stone**
   - Either:
     - extend `minimal_stdlib.h` to include keyword atoms (if feasible), or
     - generate a tiny 32-bit stdlib header that includes only the keyword atoms and minimal global object wiring.

3) **Regenerate stdlib for ESP32 word size**
   - Ensure the generator produces a 32-bit `js_stdlib_table` matching `JSWord` on ESP32.
   - Treat `tools/esp_stdlib_gen/esp_stdlib.h` as a host artifact unless proven correct for ESP32.

4) **Wire stdlib choice explicitly in firmware**
   - Make stdlib selection a build-time choice (e.g. `CONFIG_MQJS_STDLIB_MIN` vs `CONFIG_MQJS_STDLIB_FULL`).
   - Ensure any “full” stdlib is not accidentally compiled in if we need footprint.

5) **Align REPL UX with actual capabilities**
   - If autoload fails, do not claim “Loaded libraries: MathUtils …” in the banner (`imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c:334`).
   - Prefer: print what loaded successfully, and keep REPL usable even if autoload fails.

## Appendix: diagrams and mental models

### The stdlib definition at a glance

```text
                 +--------------------+
                 |   JSSTDLibraryDef  |
                 |--------------------|
                 | stdlib_table  ----+|----> ROM-ish blob:
                 | c_function_table  |       - atom table
                 | c_finalizer_table |       - global object graph
                 | offsets/lengths   |       - class definitions
                 +--------------------+
```

### Why keyword atoms affect parsing (conceptual pseudo-code)

```text
lexIdentifier():
  s = readIdentifierChars()
  atom = intern(s)                 // needs atom table infrastructure
  if atom in keyword_atoms:        // e.g., ATOM_var, ATOM_function
    return TOKEN_KEYWORD(atom)
  else:
    return TOKEN_IDENT(atom)
```

If `ATOM_var` doesn’t exist (or the keyword_atoms set is empty), `var` becomes a normal identifier token.

### “Intern FAQ”: is `var` a keyword?

Yes. In JavaScript (and in any sane JS engine), `var` is a reserved keyword introducing a variable declaration statement. If an engine can’t parse `var`, it’s not “missing the standard library” — it’s missing a core language feature.

In MicroQuickJS, the “core language feature” is partially implemented via the stdlib/atom table mechanism. That’s the important conceptual mismatch that this doc is meant to highlight.
