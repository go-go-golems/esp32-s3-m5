---
Title: 'Technical Writing Guidelines for ESP32/Embedded Documentation'
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - documentation
    - writing
    - guidelines
    - esp32
    - embedded
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/reference/02-esp-idf-wifi-softap-sta-apsta-guide.md
      Note: Example of these guidelines applied to WiFi documentation
ExternalSources:
    - URL: https://developers.google.com/tech-writing
      Note: Google technical writing courses
Summary: "Writing guidelines for creating comprehensive, readable technical documentation for ESP32 and embedded systems, refined from the process of documenting Tutorial 0017's WiFi implementation."
LastUpdated: 2025-12-26
WhatFor: "Reference guide for creating technical documentation that balances depth, readability, and practical utility for senior embedded developers."
WhenToUse: "When writing reference guides, architecture docs, or comprehensive technical explanations for ESP32/embedded systems."
---

# Technical Writing Guidelines for ESP32/Embedded Documentation

## Purpose and audience

These guidelines describe how to write technical documentation for ESP32 and embedded systems projects that is:
- **Comprehensive enough** for senior engineers to use as a reference
- **Narrative enough** to read comfortably from start to finish
- **Practical enough** to help readers debug and extend systems
- **Accessible enough** for engineers new to the specific platform (ESP32/ESP-IDF) but experienced in embedded development generally

The target reader is a **senior embedded engineer who is new to ESP32 or ESP-IDF specifically**, not someone learning embedded development from scratch. This means you can assume knowledge of concepts like interrupts, DMA, RTOS, and networking, but should explain ESP-IDF-specific APIs, patterns, and quirks in detail.

## Core principles

### 1. Start with orientation, not details

**The problem**: Technical documentation often jumps straight into API calls or code examples without giving readers context about what they're building or why it matters.

**The solution**: Begin every document with an executive summary that orients the reader. This summary should answer:
- What is this document about?
- What will I learn by reading it?
- Why does this topic matter?
- What assumptions or prerequisites do I need?

**Example structure**:
```markdown
## Executive summary

[2-4 paragraphs explaining what the document covers, why it matters,
and how it will help the reader. Use concrete examples and scenarios.]

[Explain what's different or surprising compared to what readers might expect
from other platforms or frameworks they know.]

[Preview the key concepts or decisions they'll need to understand.]
```

**Real example** (from the WiFi guide):
> "If you're coming to ESP32 WiFi from other embedded networking stacks, you'll find that ESP-IDF takes a more structured, layered approach than you might expect. Unlike simpler systems where WiFi is 'just a driver plus socket calls,' ESP-IDF WiFi is a coordinated system involving multiple components that must work together."

This immediately orients experienced embedded engineers by contrasting with what they already know.

### 2. Explain the mental model before the mechanics

**The problem**: Jumping straight into API calls without explaining the underlying architecture leads to "cookbook programming"—readers can copy code but don't understand why it works or how to adapt it.

**The solution**: Include a "mental model" or "architecture" section that explains the conceptual structure before diving into code. This section should:
- Name the key abstractions and their relationships
- Explain what exists at runtime (structures, tasks, state machines)
- Clarify the order of operations and dependencies
- Distinguish between concepts that might be confused

**Structure**:
```markdown
## Understanding the [system/architecture/model]

[Introductory paragraph explaining why understanding the model matters]

### Component A

[Explain what Component A is, what it's responsible for, and how it fits
into the larger system. Use concrete examples.]

### Component B

[Same for Component B, explicitly mentioning how it relates to Component A]

### Key insights and gotchas

[Call out important implications of this architecture that might not be obvious]
```

**Real example** (from the WiFi guide):
The "Mental model (what exists at runtime)" section distinguishes between:
- Radio role (what's happening on the air)
- Network interface (the IP side)
- Event-driven lifecycle (how you know when things are ready)

This prevents the common mistake of thinking "WiFi connected" means "can use HTTP now."

### 3. Explain "when" and "why," not just "how"

**The problem**: Many technical docs are just API references—they tell you what functions do, but not when to use them or why you'd choose one approach over another.

**The solution**: For each major concept, explicitly address:
- **When to use it**: Scenarios where this approach is appropriate
- **What it gives you**: Benefits and advantages
- **What it costs**: Trade-offs, limitations, and downsides
- **When not to use it**: Scenarios where a different approach is better

**Structure**:
```markdown
## [Feature/Mode/Concept]

[Introductory paragraph explaining what this is]

### When to use [this approach]

[Concrete scenarios with explanations. Use bullet points for clarity,
but wrap each point in enough text to explain the context.]

### What [this approach] gives you

[Advantages explained in detail. Don't just list benefits—explain
why each benefit matters in practice.]

### What [this approach] requires from you

[Trade-offs, limitations, failure modes. Be honest about downsides.
Explain how to mitigate them.]
```

**Real example** (from the WiFi guide):
Each WiFi mode (SoftAP, STA, AP+STA) has dedicated sections for:
- When to use SoftAP (field deployment, recovery, first-time provisioning, standalone operation)
- What SoftAP gives you (predictability, independence, simple workflow)
- What SoftAP doesn't give you (no LAN access, no internet, isolation)

This helps readers make informed decisions rather than cargo-culting example code.

### 4. Show complete, compilable examples

**The problem**: Code snippets in documentation are often incomplete fragments that readers can't actually compile or run. This leads to confusion and frustration.

**The solution**: Provide complete examples that:
- Include all necessary includes and declarations
- Show error handling (not just happy-path)
- Are actually correct and compilable
- Include inline comments explaining non-obvious parts

**Structure**:
```markdown
### [Concept] in practice: a complete example

[Paragraph explaining what this example demonstrates and why it's
structured this way]

```c
// Complete, compilable example with inline comments
#include "necessary_headers.h"

// Explain any global state or setup requirements
static esp_netif_t *s_netif = NULL;

esp_err_t setup_feature(void) {
    // Each significant step gets a comment explaining what it does
    // and why it's necessary
    
    esp_err_t ret = some_api_call();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed because X: %s", esp_err_to_name(ret));
        return ret;  // Show error handling
    }
    
    // Show the happy path
    ESP_LOGI(TAG, "Feature ready");
    return ESP_OK;
}
```

[Follow-up paragraph explaining what happens after this code runs,
any state changes, or what the reader should do next]
```

**Real example** (from the WiFi guide):
The "SoftAP in practice: a complete example" section shows a full, compilable SoftAP setup with all includes, error handling, and inline comments explaining each step.

### 5. Walk through APIs in order of use, not alphabetically

**The problem**: API references are alphabetical or grouped by header file, which doesn't match how you actually use APIs. Readers don't know where to start or what order to call things.

**The solution**: Create an "API walkthrough" section that presents functions in the order you'd typically call them, explaining why each step depends on the previous one.

**Structure**:
```markdown
## API walkthrough: the complete sequence

[Introductory paragraph explaining that order matters and why]

### Step 1: [First thing you do]

**Why this comes first**: [Explain the dependency or rationale]

**What to do**:
```c
// Code example
```

**Common issue**: [Describe what goes wrong if you skip this or do it wrong]

**When this matters**: [Explain when this is critical vs. optional]

### Step 2: [Next thing you do]

**Why this comes after Step 1**: [Explain the ordering dependency]

[Continue for all major steps]
```

**Real example** (from the WiFi guide):
The "ESP-IDF API walkthrough" section presents 9 steps in order:
1. Initialize NVS (explains why WiFi needs it)
2. Initialize network stack (explains why this comes before WiFi)
3. Create netifs (explains why interfaces come before driver)
4. Initialize WiFi driver (explains separation from netifs)
etc.

Each step explains not just what to do, but why it must come in that order.

### 6. Include dedicated troubleshooting sections

**The problem**: Documentation that only covers the happy path leaves readers stranded when things go wrong. In practice, debugging takes more time than initial implementation.

**The solution**: Include a "Debugging and troubleshooting" section with common failure modes. For each failure mode, provide:
- **The symptom**: What you observe (logs, behavior)
- **Why it happens**: Common causes (ranked by likelihood)
- **How to diagnose**: Specific steps to confirm the cause
- **How to fix**: Solutions or workarounds

**Structure**:
```markdown
## Debugging and troubleshooting

[Introductory paragraph explaining that failures are normal and this
section will help you diagnose them]

### [Common problem 1]

**The symptom**: [What you see—logs, behavior, error messages]

**Why this happens**: [Explain the root causes. For multiple causes,
use a list and explain each one:]

**Cause 1: [Specific cause]**

[Paragraph explaining this cause, why it happens, and how to recognize it]

**How to diagnose**: [Specific steps to confirm this is the problem]

**How to fix**: [Solutions, workarounds, or configuration changes]

**Practical solutions**:
- [Actionable recommendation 1]
- [Actionable recommendation 2]
- [Actionable recommendation 3]

### [Common problem 2]

[Same structure]
```

**Real example** (from the WiFi guide):
The "Debugging and troubleshooting" section covers:
- STA connects but device is unreachable (client isolation, firewall, VLANs)
- DHCP is slow or fails (wrong credentials, weak signal, DHCP unavailable)
- Connection keeps dropping (signal strength, power management, network policies)
- SoftAP clients can't get IP addresses

Each one includes symptoms, causes, diagnostics, and practical solutions.

### 7. Bridge abstract concepts with concrete implementations

**The problem**: Documentation is either too abstract (theory with no code) or too concrete (code with no explanation). Readers need both.

**The solution**: Use a two-pass approach:
1. **First pass**: Explain the concept abstractly with clear explanations
2. **Second pass**: Show the concrete implementation that demonstrates the concept

**Structure**:
```markdown
## [Concept]

[Abstract explanation of the concept—what it is, why it exists, how it works]

### How [concept] works in ESP-IDF

[Explain how the abstract concept maps to ESP-IDF specifics]

### [Concept] in practice

[Show actual code that implements the concept, with enough context
that readers can see the theory applied]
```

**Real example** (from the WiFi guide):
"Synchronization: waiting for network readiness" section:
1. Explains the abstract problem (need to wait for async operations)
2. Describes the FreeRTOS event group pattern
3. Shows complete code implementing this pattern
4. Explains what to do with the result

### 8. Use progressive detail throughout

**The problem**: Readers need different levels of detail at different times. Starting with too much detail is overwhelming; staying at high-level is not actionable.

**The solution**: Structure each major topic with progressive detail:
1. **High-level overview**: What this is and why it matters (1-2 paragraphs)
2. **Conceptual explanation**: How it works without code (bullet points + prose)
3. **Implementation details**: Actual APIs and code examples
4. **Edge cases and gotchas**: What can go wrong and how to handle it

**Structure**:
```markdown
## [Topic]

[1-2 paragraph high-level overview]

### Understanding [topic]

[Conceptual explanation with diagrams or analogies]

### How to implement [topic]

[Step-by-step with code examples]

### Common issues with [topic]

[Gotchas and edge cases]
```

**Real example** (from the WiFi guide):
Each WiFi mode starts with conceptual explanation, then shows implementation, then covers troubleshooting.

### 9. Make it scannable but also readable linearly

**The problem**: Some docs are only readable linearly (walls of text), while others are only scannable (bullet points with no context). Readers need both.

**The solution**: Use a layered structure:
- **Section headings** are descriptive and form a useful outline
- **First paragraph** of each section is a standalone summary
- **Bullet points and code blocks** provide scannable reference
- **Explanatory paragraphs** around structured content provide readability

**Heading guidelines**:
- Use descriptive headings that tell you what the section covers
- Good: "SoftAP: creating your own WiFi network"
- Bad: "Mode 1" or "Configuration"
- Make headings specific enough to be useful in a table of contents

**First paragraph rule**: The first paragraph of every section should be readable standalone. Someone skimming headings and first paragraphs should get a complete overview.

**Balance prose and structure**:
```markdown
## Section

[First paragraph: standalone summary]

[Additional context paragraphs explaining background, relationships, or rationale]

**Key points**:
- [Scannable bullet point 1]
- [Scannable bullet point 2]

[Explanatory paragraph linking the bullet points to larger context]

```c
// Code example
```

[Paragraph explaining what the code does and what to do next]
```

### 10. Include concrete scenarios and use cases

**The problem**: Abstract explanations are hard to remember and apply. Readers need concrete scenarios to anchor the concepts.

**The solution**: Include specific scenarios whenever explaining concepts. These scenarios should:
- Be realistic and relatable
- Clearly illustrate the concept
- Help readers recognize when to apply the concept

**Pattern**:
```markdown
### When to use [approach]

[Introductory sentence]

**[Scenario 1: Descriptive name]**: [Detailed explanation of this scenario,
why it's relevant, and how the approach solves the problem. Include enough
detail that readers can recognize similar situations in their own work.]

**[Scenario 2: Descriptive name]**: [Same structure]

**Example**: [If helpful, include a concrete example: "For example, an IoT sensor
might boot into SoftAP mode, you connect your phone to configure it, and then
it switches to STA mode to join your network."]
```

**Real example** (from the WiFi guide):
"When to use SoftAP" includes specific scenarios:
- Field deployment and setup (with example)
- Recovery and debugging (explains the "back door" concept)
- First-time provisioning (describes the workflow)
- Standalone operation (lists concrete applications)

Each scenario includes enough context that readers can map it to their own projects.

### 11. Explain the lifecycle and state machines

**The problem**: Many embedded systems involve state machines and asynchronous operations. Documentation that only shows "how to start" without showing "what happens next" leaves readers confused.

**The solution**: For any asynchronous or stateful system, include a section that explicitly explains:
- What states exist
- How the system transitions between states
- What events trigger transitions
- What the system can and can't do in each state

**Structure**:
```markdown
### How [system] works: the lifecycle

[Introductory paragraph explaining that this is an asynchronous/stateful
system and understanding the lifecycle is crucial]

**Phase 1: [Initial state]**

[Explain what happens in this phase, what state exists, what you can
and can't do. Include code if helpful.]

**Phase 2: [Next state]**

[Explain the transition from Phase 1 to Phase 2—what triggers it,
what happens internally, and what changes]

[Continue for all major phases]

**Key insight**: [Call out important implications of this lifecycle
that might not be obvious]
```

**Real example** (from the WiFi guide):
"How STA mode works: the connection lifecycle" section breaks down the connection process into phases:
1. Interface initialization
2. WiFi driver initialization
3. Register event handlers
4. Configure credentials
5. Start WiFi and initiate connection
6. Wait for IP address

Each phase explains what happens, why it's necessary, and what can go wrong.

### 12. Distinguish between "must," "should," and "can"

**The problem**: Documentation often uses vague language like "you need to" without clarifying whether something is strictly required, a best practice, or optional.

**The solution**: Be explicit about requirements using clear language:
- **"Must" / "Required"**: Strictly necessary—code will fail without it
- **"Should" / "Recommended"**: Best practice—code works without it, but you probably want it
- **"Can" / "Optional"**: Available for advanced use cases, but not needed for basic usage

**Examples**:
- "You **must** initialize NVS before calling `esp_wifi_init()`—WiFi initialization will fail otherwise."
- "You **should** set a hostname for production devices—it makes them easier to identify on the network, but it's not required."
- "You **can** customize the netif configuration for advanced use cases, but the default is usually sufficient."

### 13. Show error handling, not just happy paths

**The problem**: Example code that only shows the success case teaches readers to ignore errors. When errors inevitably happen, readers don't know how to handle them.

**The solution**: Show error handling in all examples:
- Check return values
- Explain what errors mean
- Show how to handle or recover from errors

**Pattern**:
```c
esp_err_t ret = some_api_call();
if (ret != ESP_OK) {
    // Explain what this error means and how to handle it
    ESP_LOGE(TAG, "Failed to X because Y: %s", esp_err_to_name(ret));
    
    // Show recovery or cleanup
    if (ret == ESP_ERR_SPECIFIC_ERROR) {
        // Handle this specific error case
    }
    
    return ret;  // Or handle differently
}

// Continue with success case
```

**Common error patterns in ESP-IDF**:
- `ESP_ERR_INVALID_STATE`: Function called in wrong state/order
- `ESP_ERR_TIMEOUT`: Async operation didn't complete in time
- `ESP_ERR_NO_MEM`: Out of memory
- `ESP_ERR_NOT_FOUND`: Resource doesn't exist

Include explanations of what these mean in your domain.

### 14. Provide synchronization patterns explicitly

**The problem**: Embedded systems have asynchronous operations, but documentation often doesn't show how to wait for completion or synchronize between components.

**The solution**: When describing asynchronous operations, always show:
- How to know when the operation completes
- How to wait for completion (if synchronous behavior is needed)
- How to handle timeouts
- What state is safe to query

**Pattern**:
```markdown
### Synchronization: waiting for [operation]

[Explain why you can't just assume the operation is done immediately]

**The problem**: [Describe what goes wrong if you don't wait properly]

**The solution**: [Show the synchronization mechanism, e.g., event groups,
queues, semaphores, flags]

```c
// Show complete example with:
// 1. Setup (creating sync primitives)
// 2. Initiating the async operation
// 3. Waiting for completion with timeout
// 4. Handling both success and timeout cases
```

**Why this matters**: [Explain practical implications]
```

**Real example** (from the WiFi guide):
"Synchronization: waiting for network readiness" shows:
- The problem (can't use network immediately after start)
- The pattern (FreeRTOS event groups)
- Complete implementation with event handler + waiting code
- Timeout handling and recommendations

### 15. Call out ESP-IDF-specific quirks

**The problem**: Experienced embedded engineers will have assumptions from other platforms. ESP-IDF has specific patterns and quirks that violate these assumptions.

**The solution**: Explicitly call out ESP-IDF-specific behavior, especially where it differs from expectations. Use clear labels:
- **"Important"**: Critical information that prevents common mistakes
- **"Note"**: Clarification or additional context
- **"Warning"**: Things that can go seriously wrong
- **"Tip"**: Helpful shortcuts or optimizations

**Examples**:
- **Important**: "Unlike some platforms, `esp_netif_init()` can be called multiple times—it returns `ESP_ERR_INVALID_STATE` if already initialized, which is safe to treat as success."
- **Note**: "The 'default' in `esp_netif_create_default_wifi_ap()` means ESP-IDF uses sensible defaults for IP range, DHCP settings, etc."
- **Warning**: "Don't assume the network is ready immediately after `esp_wifi_start()`. For STA mode, you must wait for `IP_EVENT_STA_GOT_IP` before the network is usable."

### 16. Link concepts to working code

**The problem**: Abstract explanations without code references are hard to verify. Readers wonder "Is this actually how it works, or is this theoretical?"

**The solution**: After explaining a concept, explicitly link to the working implementation:
- Point to specific files and line ranges
- Explain what to look for in the code
- Show how the theory maps to practice

**Pattern**:
```markdown
### [Concept explanation]

[Detailed explanation of how something works]

**In Tutorial 0017**: This is implemented in `main/wifi_app.cpp`, lines 165-252.
The key parts to notice are:
- [Specific aspect of the implementation]
- [Another specific aspect]
- [Important detail that might not be obvious]

You can see the complete implementation at:
`esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp`
```

### 17. Provide decision trees for choosing between approaches

**The problem**: When there are multiple ways to do something, readers need guidance on which approach to choose.

**The solution**: Provide clear decision guidance:

**Pattern**:
```markdown
### Choosing between [option A], [option B], and [option C]

**Use [option A] when**:
- [Criterion 1 with explanation]
- [Criterion 2 with explanation]

**Use [option B] when**:
- [Different criterion]

**Use [option C] when**:
- [Yet another criterion]

**During development, we recommend**: [Specific recommendation with rationale]

**For production, consider**: [Different recommendation with rationale]
```

**Real example** (from the WiFi guide):
The mode selection sections help readers choose between SoftAP, STA, and AP+STA based on:
- Infrastructure dependencies
- Development vs. production needs
- Required features (internet access, recovery path, etc.)

### 18. Include "what happens next" transitions

**The problem**: Readers finish a section but don't know what to do next or how it relates to what follows.

**The solution**: End sections with transition text that:
- Summarizes what was covered
- Explains how it connects to the next section
- Provides actionable next steps

**Pattern**:
```markdown
[Section content]

**Where to go from here**: [Brief explanation of what the next section covers
and how it builds on this section. Or, if this is a terminal section, explain
what readers can do with this knowledge.]
```

### 19. Provide both "reference" and "tutorial" paths

**The problem**: Some readers want to read cover-to-cover, others want to jump to specific information.

**The solution**: Structure docs to support both:
- **For linear readers**: Logical flow from basics to advanced
- **For reference readers**: Clear headings, table of contents, and self-contained sections

**Guidelines**:
- Make section headings descriptive enough to find via search
- Include a table of contents (auto-generated from headings)
- Make each subsection as self-contained as possible
- Use forward and backward references ("as described in Section X")

### 20. Include realistic configuration examples

**The problem**: Documentation shows defaults but doesn't explain how to choose good values for production.

**The solution**: When showing configuration options, include:
- Default values and why they're defaults
- Typical ranges for different use cases
- How to decide what value to use
- Real-world constraints

**Pattern**:
```markdown
### Configuration parameter: [PARAMETER_NAME]

**Default**: [value]

**Typical range**: [range with explanation]

**How to choose**: [Decision guidance]

**Examples**:
- **For [scenario 1]**: Use [value] because [rationale]
- **For [scenario 2]**: Use [value] because [rationale]

**Trade-offs**: [Explain what changes when you adjust this parameter]
```

**Real example** (from the WiFi guide):
Timeout configuration includes:
- Default (15 seconds)
- Why 15 seconds (scan + associate + DHCP)
- When to increase it (production, slow networks)
- What it affects (boot time vs. reliability)

## Document structure template

For comprehensive guides (like the WiFi guide), use this overall structure:

```markdown
# [Title]

## Executive summary
[2-4 paragraphs orienting the reader]

## Understanding [the system/architecture]
[Mental model section explaining components and relationships]

## [Primary concept 1]
[When to use, what it gives you, how it works, implementation]

## [Primary concept 2]
[Same structure]

## [Additional concepts...]

## Complete API walkthrough
[Step-by-step in order of use]

## Worked example: [Real implementation]
[Walkthrough of actual working code from the repo]

## Debugging and troubleshooting
[Common problems with symptoms, causes, and solutions]

## Resources for further learning
[Categorized links to official docs, examples, RFCs]
```

This structure supports:
- **Newcomers**: Can read linearly from start to finish
- **Experienced users**: Can jump to specific sections via headings
- **Debuggers**: Can skip to troubleshooting section
- **Reference users**: Can use API walkthrough as a checklist

## Writing process

### Phase 1: Research and outline

Before writing, gather:
- Official API documentation for all mentioned functions
- Working example code from the repo
- Known issues or gotchas from experience
- Related resources (RFCs, tutorials, forums)

Create an outline:
- List major concepts/sections
- For each section, note: what it covers, key APIs, and examples to include
- Identify decision points (where readers need guidance)

### Phase 2: Write the mental model

Start by writing the "Understanding [system]" section:
- Name the key abstractions
- Explain relationships
- Clarify the order of operations
- Call out surprising or non-obvious aspects

This section becomes the foundation—other sections will reference it.

### Phase 3: Write concept sections

For each major concept:
1. Start with "when to use" scenarios
2. Explain "what it gives you" (benefits)
3. Explain "what it requires" (costs/trade-offs)
4. Show implementation with complete code examples
5. Add troubleshooting subsection

### Phase 4: Write the API walkthrough

Create the sequential walkthrough:
- List APIs in order of use (not alphabetically)
- For each API, explain: what it does, why this order, what can go wrong
- Include code snippets for each step
- Show error handling

### Phase 5: Add the worked example

Walk through the actual implementation from your repo:
- Point to specific files and functions
- Explain design decisions
- Show how concepts map to real code
- Include quotes or snippets from the actual code

### Phase 6: Write troubleshooting section

Based on experience or anticipation:
- List common failure modes
- For each: symptom → causes → diagnosis → fixes
- Include practical solutions
- Reference relevant code or logs

### Phase 7: Add transitions and polish

- Add transition paragraphs between sections
- Ensure first paragraphs are standalone summaries
- Check that headings are descriptive
- Verify code examples are complete and correct
- Add forward/backward references where helpful

### Phase 8: Review for balance

Check that each section has:
- Narrative introduction
- Structured content (bullets, code, tables)
- Explanatory text around structured content
- Practical implications or examples
- Transitions to next section

## Common patterns to reuse

### The "three-mode comparison" pattern

When explaining multiple approaches to the same problem:

```markdown
## [Approach A]

[Explanation, when to use, pros/cons]

## [Approach B]

[Same structure, explicitly contrasting with Approach A where relevant]

## [Approach C]

[Same structure]

## Choosing between [A], [B], and [C]

[Decision guidance with clear criteria]
```

### The "complete example" pattern

When showing how to use an API:

```markdown
### [Feature] in practice: a complete example

Here's what a minimal but complete [feature] setup looks like in practice:

```c
// Complete, compilable code
// With inline comments explaining each step
```

After this function completes, [explain the resulting state and what
the reader can do next].
```

### The "troubleshooting entry" pattern

For each common problem:

```markdown
### [Problem description]

**The symptom**: [What you observe]

**Why this happens**: [Root causes explained]

**Common causes**:

**[Cause 1]**: [Explanation]

**How to diagnose**: [Specific steps]

**[Cause 2]**: [Explanation]

**How to diagnose**: [Specific steps]

**Practical solutions**:
- [Solution 1]
- [Solution 2]
- [Solution 3]
```

### The "synchronization explanation" pattern

For asynchronous operations:

```markdown
### Synchronization: waiting for [operation]

**Why you need to wait**: [Explain the async nature]

**The problem**: [Show what goes wrong without proper synchronization]

**The solution**: [Explain the mechanism—event groups, queues, etc.]

```c
// Complete example showing:
// - Creating sync primitive
// - Starting async operation
// - Waiting with timeout
// - Handling both success and failure
```

**Timeout considerations**: [Explain how to choose a timeout value]
```

## Specific ESP-IDF guidelines

### Handling ESP-IDF return codes

Always check return values and explain common errors:

```c
esp_err_t ret = esp_function_call();
if (ret != ESP_OK) {
    // Explain what this error means in this context
    ESP_LOGE(TAG, "Failed to X: %s", esp_err_to_name(ret));
    
    // Show error-specific handling if relevant
    if (ret == ESP_ERR_INVALID_STATE) {
        // This usually means you called things in the wrong order
    }
    
    return ret;
}
```

### Menuconfig documentation

When documenting menuconfig options:
- Show the config symbol name (`CONFIG_XXX`)
- Explain what it controls
- Show default value and why it's the default
- Explain when to change it
- Show how it's used in code

### Component dependencies

Explain component/library dependencies clearly:
- What components need to be added to `CMakeLists.txt`
- What headers need to be included
- What initialization order is required

### FreeRTOS integration

When FreeRTOS APIs are involved:
- Explain the RTOS concepts (tasks, queues, semaphores) briefly
- Show how to use them correctly (e.g., `FromISR` variants)
- Explain priority and stack size considerations
- Show proper cleanup/deletion

## Examples of good vs. problematic patterns

### Good: Explaining order dependencies

> "You must initialize NVS before calling `esp_wifi_init()` because the WiFi driver stores calibration data in NVS. If NVS isn't initialized, WiFi initialization will fail with `ESP_ERR_INVALID_STATE`."

### Problematic: Just listing steps

> "1. Initialize NVS
> 2. Initialize WiFi"

**Why it's better**: The first version explains WHY the order matters, making it memorable and helping readers understand similar dependencies.

### Good: Showing failure handling

```c
esp_err_t ret = wait_for_sta_ip();
if (ret == ESP_OK) {
    // Network is ready
    start_http_server();
} else {
    ESP_LOGW(TAG, "Failed to get IP within timeout");
    // Fall back to SoftAP for recovery
    start_softap_fallback();
}
```

### Problematic: Only showing success

```c
wait_for_sta_ip();
start_http_server();
```

**Why it's better**: The first version shows what to do when things fail, which is crucial for robust firmware.

### Good: Concrete scenario

> "**Field deployment and setup**: If you're deploying devices in locations without existing WiFi infrastructure, SoftAP lets you connect directly to configure them. For example, an IoT sensor might boot into SoftAP mode, you connect your phone to configure it, and then it switches to STA mode to join your network."

### Problematic: Abstract description

> "SoftAP is useful for setup."

**Why it's better**: The concrete scenario helps readers recognize when they're in a similar situation.

### Good: Explaining state

> "Only after receiving the `IP_EVENT_STA_GOT_IP` event can you reliably use the network. Before this point, the radio link might be established, but you don't have an IP address yet, so TCP/IP communication won't work."

### Problematic: Just stating the fact

> "Wait for `IP_EVENT_STA_GOT_IP` before using the network."

**Why it's better**: The first version explains the state model (radio link vs. IP address) and the practical implication (TCP/IP won't work), helping readers understand why this matters.

## Quality checklist

Before considering a document complete, verify:

- [ ] Executive summary orients readers and previews key concepts
- [ ] Mental model/architecture section explains structure before details
- [ ] Each major concept has "when to use" and "trade-offs" sections
- [ ] Code examples are complete and compilable
- [ ] Error handling is shown, not just happy paths
- [ ] Asynchronous operations include synchronization patterns
- [ ] Troubleshooting section covers common failure modes
- [ ] API walkthrough presents functions in order of use
- [ ] Worked example points to actual code in the repo
- [ ] Transitions between sections explain relationships
- [ ] Headings are descriptive and form a useful outline
- [ ] First paragraph of each section is a standalone summary
- [ ] Technical terms are explained when first introduced
- [ ] Resources section provides paths to deeper learning

## Anti-patterns to avoid

### Anti-pattern 1: Starting with API reference

Don't start with "here are the functions you call." Start with "here's what you're building and why."

### Anti-pattern 2: No explanation of ordering

Don't just show steps without explaining why they must happen in that order. Every "you must do X before Y" should have a "because..." explanation.

### Anti-pattern 3: Ignoring failure cases

Don't show only successful paths. Real firmware must handle errors, and documentation should show how.

### Anti-pattern 4: Assuming reader knowledge

Don't assume readers know ESP-IDF-specific concepts. You can assume embedded development knowledge, but ESP-IDF patterns need explanation.

### Anti-pattern 5: Code without context

Don't drop code examples without explaining what they're demonstrating, what to look for, or what happens next.

### Anti-pattern 6: No practical guidance

Don't just describe what APIs do—explain when to use them, how to debug them, and what can go wrong.

## Adaptation for different document types

These guidelines are written for comprehensive technical guides. Adapt them for different document types:

### For playbooks (step-by-step procedures)

- Be more prescriptive and less exploratory
- Use numbered steps with specific commands
- Include expected outputs
- Add decision points ("If X, do Y; otherwise do Z")
- Keep explanations brief but include "why" for non-obvious steps

### For API references

- Keep it concise—focus on what each function does
- Include function signatures
- Document all parameters and return values
- Link to examples or guides for context
- List common errors and their meanings

### For architecture docs

- Focus on relationships between components
- Use diagrams extensively
- Explain design decisions and trade-offs
- Show data flow and control flow
- Include performance characteristics

### For diary entries

- Capture decisions and their rationale
- Record what worked and what didn't
- Note surprises or unexpected behavior
- Link to commits and specific code changes
- Keep it chronological

## Summary

Great embedded systems documentation:
- **Orients** readers before diving into details
- **Explains** the mental model before showing code
- **Addresses** when/why/what trade-offs, not just how
- **Shows** complete, correct examples with error handling
- **Walks through** APIs in order of use
- **Troubleshoots** common failures with clear diagnostics
- **Links** concepts to working code
- **Balances** narrative readability with structured scannability

The goal is documentation that senior engineers can:
- Read linearly to understand the system
- Scan quickly to find specific information
- Use as a reference when implementing
- Turn to when debugging problems
- Extend when building on the system

This guide itself is an example of the approach: it starts with orientation, explains principles with examples, provides actionable patterns, and includes practical considerations throughout.

