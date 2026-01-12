---
Title: 'Technical Article Writing Guide: Research and Style Standards'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - documentation
    - writing
    - style-guide
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/10-cardputer-serial-terminal-usb-vs-grove-uart-backend-guide.md
      Note: Example article demonstrating these standards
ExternalSources: []
Summary: 'Complete guide for conducting research and writing technical articles to publication quality.'
LastUpdated: 2026-01-12T11:00:00-05:00
WhatFor: 'Ensure consistency in technical writing across all documentation.'
WhenToUse: 'Reference before and during any technical article writing project.'
---

# Technical Article Writing Guide: Research and Style Standards

This playbook defines how we research, structure, and write technical articles for our documentation. Following these standards ensures consistency, quality, and usefulness across all our developer-facing content.

The goal: every article should read like it was written by the same thoughtful engineer—someone who deeply understands the topic, anticipates reader questions, and explains concepts with clarity and appropriate depth.

---

## Part 1: The Research Phase

Before writing a single line of documentation, you must deeply understand the topic. This isn't about skimming—it's about building genuine expertise.

### 1.1 Locate All Source Materials

Start by identifying everything relevant to your topic:

**Primary sources (highest priority):**
- The actual source code being documented
- Kconfig files (they reveal configuration options and their defaults)
- Existing README files in the project
- Implementation diaries from original development
- Commit messages that explain design decisions

**Secondary sources:**
- ESP-IDF official documentation
- Datasheets for hardware components
- Related tutorials or examples in the codebase
- Stack Overflow answers that shaped decisions

**Create a source inventory:**

```markdown
## Sources Reviewed

### Code Files
- `main/hello_world_main.cpp` — Primary implementation (735 lines)
- `main/Kconfig.projbuild` — Configuration options (232 lines)
- `sdkconfig.defaults` — Default values

### Documentation
- `README.md` — Project overview
- `ttmp/.../reference/01-diary.md` — Implementation diary

### External
- ESP-IDF UART driver docs
- ESP-IDF USB Serial/JTAG docs
```

### 1.2 Read Code Before Writing About It

Never describe code you haven't read. For each source file:

1. **Read the entire file** — Not just the function you think is relevant. Context matters.

2. **Trace the data flow** — Follow a typical operation from start to finish. For a terminal: keypress → scan → parse → buffer → display → serial.

3. **Identify the key abstractions** — What patterns does the code use? What's the architectural decision?

4. **Note the edge cases** — What does the code do when things go wrong? These become troubleshooting content.

5. **Extract the "why"** — Comments, commit messages, and diaries explain *why* the code is structured this way.

### 1.3 Keep a Research Diary

As you research, maintain a step-by-step diary of what you learned. This serves three purposes:

- Forces you to articulate your understanding
- Creates a record for future writers
- Reveals gaps in your knowledge that need more research

**Research diary structure:**

```markdown
## Step 1: [What you investigated]

Short paragraph explaining what you looked at and what you learned at a high level.

### What I did
- Specific actions (read file X, traced function Y)

### What I learned
- Key insights extracted

### What was surprising or non-obvious
- Things a reader might also find surprising

### What I still don't understand
- Gaps to investigate further
```

### 1.4 Build Mental Models

Before writing, you should be able to:

- **Draw the architecture** from memory (even roughly)
- **Explain the key tradeoffs** the code makes
- **Predict what happens** in common scenarios
- **Identify the sharp edges** — where things can go wrong

If you can't do these things, you haven't researched enough.

---

## Part 2: Article Structure

Our technical articles follow a consistent structure that guides readers from understanding to implementation.

### 2.1 The Standard Article Flow

```
1. Introduction (What and Why)
   ├── What we're building/explaining
   ├── Why someone would care
   └── What they'll learn

2. Core Concepts (The Foundation)
   ├── Key abstractions
   ├── Important decisions/tradeoffs
   └── Visual architecture diagram

3. The Critical Constraint (If Any)
   ├── The non-obvious thing that will bite you
   ├── Why it happens
   └── How to work around it

4. Practical Details (The How)
   ├── Configuration
   ├── Wiring/setup
   ├── Building and running
   └── What you'll see

5. Deep Dive (For the Curious)
   ├── Code walkthrough
   ├── Internal architecture
   └── How things work under the hood

6. Troubleshooting (When Things Go Wrong)
   ├── Common problems
   ├── Diagnostic steps
   └── Solutions

7. Reference (Quick Lookup)
   ├── API summary
   ├── Configuration options
   └── Commands cheat sheet

8. Next Steps (Where to Go From Here)
   └── Related topics, advanced usage
```

### 2.2 Section Length Guidelines

| Section | Target Length | Notes |
|---------|--------------|-------|
| Introduction | 2-4 paragraphs | Hook the reader, establish context |
| Core Concepts | 3-6 paragraphs per concept | Each concept gets full treatment |
| Critical Constraint | 2-4 paragraphs + diagram | Make this impossible to miss |
| Practical Details | As needed | Be thorough but not padded |
| Deep Dive | 4-8 paragraphs per topic | Code snippets with explanation |
| Troubleshooting | 1-2 paragraphs per problem | Symptom → Cause → Solution |
| Reference | Tables and lists | Scannable, not prose |
| Next Steps | 1-2 paragraphs | Brief, pointing outward |

### 2.3 The Opening Hook

The first paragraph should answer: "Why should I keep reading?"

**Bad opening:**
> This document describes the serial terminal firmware.

**Good opening:**
> The M5Stack Cardputer—with its integrated keyboard, display, and ESP32-S3 brain—makes an ideal platform for building a portable serial terminal. In this guide, we'll walk through the architecture of the Cardputer serial terminal firmware, explore the critical choice between USB and UART backends, and show you how to configure the terminal for your specific use case.

The good opening:
- Names the specific thing (Cardputer, ESP32-S3)
- States what you'll learn (architecture, choice, configuration)
- Implies a benefit (you'll be able to configure it for your use case)

---

## Part 3: Writing Style

### 3.1 Voice and Tone

**Write in second person ("you"):**

> When you select the USB backend, your Cardputer becomes a USB serial device.

Not: "When the user selects..." or "When one selects..."

**Use active voice:**

> The firmware scans the keyboard matrix every 10ms.

Not: "The keyboard matrix is scanned by the firmware every 10ms."

**Be direct and confident:**

> This is the most important constraint in the entire firmware.

Not: "This might be considered an important constraint."

**Address the reader as an intelligent peer:**

Assume they're a competent developer who simply doesn't know *this particular thing* yet. Don't be condescending, but don't assume they already know the topic.

### 3.2 Explain the "Why"

Every significant piece of information should have context. Don't just state facts—explain why they matter.

**Fact only:**
> The default baud rate is 115200.

**Fact with context:**
> The default baud rate is 115200—the most common rate for embedded development and a safe choice for almost any serial device you'll encounter. You can configure rates from 1200 to 3,000,000, but higher rates require shorter cables and clean signals.

### 3.3 Use Concrete Examples

Abstract explanations become clear with concrete examples.

**Abstract:**
> The TX and RX lines must be crossed.

**Concrete:**
> UART is a point-to-point protocol. Each device has a transmit (TX) pin that sends data and a receive (RX) pin that receives data. For two devices to communicate, you must cross the signals: one device's TX connects to the other's RX, and vice versa. If you wire TX→TX and RX→RX, both devices transmit on the same wire while listening to silence.

### 3.4 Anticipate Questions

As you write each section, ask: "What would a reader wonder here?" Then answer that question.

After explaining the REPL conflict:
> **Ask yourself:** "Do I need to type commands to the firmware itself (REPL), or am I only using the terminal to talk to something else?"

### 3.5 Signpost Important Information

Use formatting to draw attention to critical points:

**For warnings/constraints:**
> ⚠️ **Critical constraint:** You cannot run both an interactive `esp_console` REPL and raw serial terminal data on USB Serial/JTAG simultaneously.

**For key takeaways:**
> **The rule of thumb:** Enable LOCAL_ECHO when talking to silent devices. Disable LOCAL_ECHO when talking to devices that echo.

**For common mistakes:**
> **Common mistake:** Connecting TX→TX and RX→RX won't work.

---

## Part 4: Technical Content Standards

### 4.1 Code Snippets

**Always explain what the code does:**

Don't just drop code. Introduce it, show it, explain the key parts.

```markdown
The `backend_init()` function configures the appropriate driver:

\`\`\`cpp
static void backend_init(serial_backend_t &b) {
#if CONFIG_TUTORIAL_0015_BACKEND_UART
    b.kind = BACKEND_UART;
    // ... UART setup ...
#else
    b.kind = BACKEND_USB_SERIAL_JTAG;
    // ... USB setup ...
#endif
}
\`\`\`

The compile-time `#if` block selects between two completely different 
initialization paths based on the menuconfig setting.
```

**Show realistic code, not toy examples:**

Pull actual code from the codebase when possible. Readers need to recognize it when they see the source.

**Include enough context:**

Show imports, struct definitions, and surrounding code when they help understanding. Don't just show the one function in isolation.

### 4.2 Configuration Documentation

For each configuration option, provide:

| Element | Example |
|---------|---------|
| Option name | `CONFIG_TUTORIAL_0015_LOCAL_ECHO` |
| Default value | `y` (enabled) |
| What it controls | Whether typed characters appear on screen immediately |
| When to change it | Disable when talking to devices that echo back input |

Group related options together. Show the menuconfig structure visually:

```
Serial backend
    ( ) USB (USB-Serial-JTAG / ttyACM*)
    (X) GROVE UART (G1/G2)
```

### 4.3 Diagrams

Use ASCII art diagrams for:
- **Data flow** — Show how information moves through the system
- **Architecture** — Show components and their relationships  
- **Wiring** — Show physical connections
- **State machines** — Show transitions between states
- **Decision trees** — Show branching logic

**Diagram guidelines:**

1. Use box-drawing characters for clean lines: `┌ ┐ └ ┘ ─ │ ├ ┤ ┬ ┴ ┼`
2. Use arrows for direction: `→ ← ↑ ↓ ► ◄ ▲ ▼`
3. Label everything clearly
4. Include a legend if symbols aren't obvious
5. Keep diagrams narrow enough to not wrap (80-100 chars max)

**Example data flow diagram:**

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Keyboard   │───►│  Key Scan   │───►│  Token      │
│  Matrix     │    │  Loop       │    │  Parser     │
└─────────────┘    └─────────────┘    └──────┬──────┘
                                             │
                                             ▼
                   ┌─────────────┐    ┌─────────────┐
                   │  Backend    │◄───│  TX Bytes   │
                   │  Write      │    │  Encoder    │
                   └──────┬──────┘    └─────────────┘
                          │
                          ▼
                      Serial Out
```

### 4.4 Tables

Use tables for:
- **Comparisons** — Side-by-side feature analysis
- **Pin mappings** — Hardware connections
- **Configuration options** — Settings and their effects
- **Error codes** — Problem → Cause → Solution

**Table formatting:**

1. Keep columns narrow and scannable
2. Use consistent formatting (all bold headers, etc.)
3. Align columns appropriately (left for text, right for numbers)
4. Use `—` for "not applicable" rather than leaving blank

**Example comparison table:**

| Aspect | USB Serial/JTAG | Grove UART |
|--------|-----------------|------------|
| Host path | `/dev/ttyACM*` | `/dev/ttyUSB*` |
| Connection | Built-in USB-C | Grove port |
| Baud rate | Fixed | Configurable |
| REPL compatible | ❌ No | ✅ Yes |
| Extra hardware | None | USB↔UART adapter |

### 4.5 Troubleshooting Sections

Structure each problem consistently:

```markdown
### Problem: [Symptom in User's Terms]

**Symptom:** What the user observes (specific and recognizable)

**Cause:** Why this happens (brief explanation)

**Diagnostic steps:**
1. First thing to check
2. Second thing to check
3. How to confirm the diagnosis

**Solution:**
1. Concrete steps to fix
2. Any configuration changes needed
3. How to verify the fix worked
```

**Include the common mistakes:**

Think about what you got wrong when learning this. Those are the problems readers will have.

---

## Part 5: Quality Checklist

Before considering an article complete, verify:

### Content Quality

- [ ] **Accurate** — All technical claims verified against source code
- [ ] **Complete** — No obvious questions left unanswered
- [ ] **Current** — Matches the latest version of the code
- [ ] **Tested** — Commands and configurations actually work

### Structure Quality

- [ ] **Logical flow** — Concepts build on each other
- [ ] **Scannable** — Headers, tables, and formatting aid navigation
- [ ] **Right depth** — Enough detail to be useful, not so much it overwhelms
- [ ] **Self-contained** — Reader doesn't need to leave to understand

### Writing Quality

- [ ] **Clear** — No jargon without explanation
- [ ] **Concise** — No unnecessary words or padding
- [ ] **Consistent** — Same terms used throughout
- [ ] **Engaging** — Not dry or robotic

### Technical Elements

- [ ] **Code tested** — All snippets compile/run
- [ ] **Diagrams clear** — Labels visible, flow obvious
- [ ] **Tables scannable** — Not too wide, headers clear
- [ ] **Links valid** — All references resolve

---

## Part 6: The Writing Process

### 6.1 Research Phase (30-40% of total time)

1. **Gather sources** — Find all relevant code, docs, and diaries
2. **Read deeply** — Understand, don't skim
3. **Take notes** — Keep a research diary
4. **Build understanding** — Draw diagrams, trace flows
5. **Identify gaps** — What do you still not understand?

### 6.2 Outline Phase (10% of total time)

1. **List key concepts** — What must the article cover?
2. **Order logically** — What depends on what?
3. **Identify visuals** — Where do diagrams help?
4. **Note examples** — What concrete examples will you use?

### 6.3 Draft Phase (30-40% of total time)

1. **Write sections in order** — Let ideas flow naturally
2. **Don't edit while drafting** — Get words down first
3. **Include placeholders** — Mark things to come back to
4. **Over-write initially** — You can cut later

### 6.4 Revision Phase (20% of total time)

1. **Read aloud** — Catches awkward phrasing
2. **Cut ruthlessly** — Remove anything that doesn't add value
3. **Check code** — Verify all snippets actually work
4. **Add transitions** — Smooth connections between sections
5. **Format for scanning** — Add headers, bullets, emphasis

### 6.5 Review Phase (10% of total time)

1. **Fresh eyes** — Step away, then re-read
2. **Technical review** — Have someone verify accuracy
3. **Editorial review** — Check against style guide
4. **Final polish** — Fix any remaining issues

---

## Part 7: Document Artifacts

For each article project, create and maintain:

### 7.1 Research Diary

Track your investigation with step-by-step entries. This becomes a reference for future updates.

**Location:** `reference/0X-research-diary-[topic].md`

### 7.2 Editor Notes

Provide a checklist for reviewers that helps them verify the article's accuracy and completeness.

**Location:** `reference/0X-editor-notes-[topic].md`

**Contents:**
- Target audience check
- Structural review checklist
- Claims to verify against source code
- Completeness check (topics that should be covered)
- Style and tone review

### 7.3 The Article Itself

The final published document.

**Location:** `playbook/0X-[descriptive-title].md`

---

## Part 8: Examples of Good Patterns

### 8.1 Opening a Section

**Pattern: Context → Concept → Detail**

> The terminal maintains a text buffer that stores the displayed content. Understanding how this buffer works helps explain the terminal's behavior and limitations.
>
> The buffer is implemented as a `std::deque<std::string>`, where each string represents one line of text...

### 8.2 Introducing Code

**Pattern: Purpose → Code → Explanation**

> The `backend_init()` function configures the appropriate driver based on the compile-time selection:
>
> ```cpp
> static void backend_init(serial_backend_t &b) {
>     ...
> }
> ```
>
> The compile-time `#if` block selects between two completely different initialization paths...

### 8.3 Explaining a Constraint

**Pattern: Constraint → Why → Visual → Solution**

> ⚠️ **Critical constraint:** You cannot run both...
>
> **Why:** USB Serial/JTAG provides a single transport...
>
> [Diagram showing the conflict]
>
> **Solution:** You have two clean options...

### 8.4 Troubleshooting Entry

**Pattern: Symptom → Cause → Diagnostic → Solution**

> ### Problem: Characters Appear Twice
>
> **Symptom:** You type "hello" and see "hheelllloo" on the screen.
>
> **Cause:** Both local echo and remote echo are enabled...
>
> **Solution:** Disable local echo in menuconfig...

---

## Appendix: Quick Reference

### Voice Checklist

- [x] Second person ("you")
- [x] Active voice
- [x] Direct and confident
- [x] Peer-to-peer tone

### Must-Have Elements

- [ ] Architecture diagram
- [ ] At least one code walkthrough
- [ ] Configuration table
- [ ] Troubleshooting section
- [ ] Quick reference section

### Formatting Standards

| Element | Format |
|---------|--------|
| File paths | `backticks` |
| Config symbols | `CONFIG_SYMBOL_NAME` |
| Commands | Code blocks with `bash` |
| Key terms on first use | **bold** |
| Warnings | ⚠️ emoji + bold |
| Menu paths | **Bold** → **Arrows** → **Between** |

### Word Choices

| Avoid | Prefer |
|-------|--------|
| utilize | use |
| in order to | to |
| it is important to note that | Note: |
| the user | you |
| basically | (delete) |
| very | (delete or be specific) |
| simple | (delete—let reader judge) |

---

## Summary

Writing good technical documentation is a skill that improves with practice. The key principles:

1. **Research deeply** — You can't explain what you don't understand
2. **Structure deliberately** — Guide readers from understanding to doing
3. **Write for humans** — Be clear, direct, and helpful
4. **Show, don't just tell** — Use diagrams, code, and examples
5. **Anticipate problems** — Help readers when things go wrong
6. **Maintain artifacts** — Keep diaries and notes for future updates

Every article should leave readers feeling informed, capable, and respected.
