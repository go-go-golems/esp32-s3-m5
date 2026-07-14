# Investigation Diary

## Goal

Capture the full research journey for M5StackChan documentation — every resource found, downloaded, and catalogued, including what worked, what failed, and what still needs attention.

## Step 1: Initial Resource Discovery and Bulk Download

This step covered the initial discovery phase: reading the M5Stack official documentation pages (K151 product page and shop page), identifying all linked resources (schematics, datasheets, firmware repos, tutorials), and bulk-downloading everything in parallel.

### Prompt Context

**User prompt (verbatim):** "Research and download all the documentation, datasheets, schematics, firmwares, github for the M5 StackChan. Work within this directory. Create a docmgr ticket and keep a detailed diary. Download every interesting resource in a sources/ subfolder, use defuddle when downloading websites. https://shop.m5stack.com/products/stackchan-kawaii-co-created-open-source-ai-desktop-robot https://docs.m5stack.com/en/products/sku/K151?ref=kmjp2x8a"

**Assistant interpretation:** Comprehensively research the M5StackChan desktop robot, create a docmgr ticket to track the work, download all available documentation (schematics, datasheets, firmware source, web pages), and maintain a detailed diary of the investigation.

**Inferred user intent:** Build a complete offline archive of every available resource for the M5StackChan, organized so that future contributors can pick up the research and extend it.

### What I did

- Created docmgr ticket M5STACKCHAN with topics: m5stack, esp32, robot, hardware, firmware
- Downloaded both user-provided URLs via defuddle (shop page + K151 product docs page)
- Identified and downloaded 5 schematic PDFs (CoreS3, Adapter, Power, Ring, Touch boards)
- Identified and downloaded 10 CoreS3 host module datasheets (ESP32-S3, GC0308, LTR-553ALS, BMI270, BMM150, AW88298, ES7210, AXP2101, BM8563, AW9523B)
- Identified and downloaded 6 body module datasheets (SCS0009, ST25R3916, Si12T, IRM56384, INA226, PY32L020)
- Downloaded 2 model/mechanical size PDFs
- Cloned 4 Git repos: `m5stack/StackChan`, `m5stack/StackChan-BSP`, `meganetaaan/stackchan`, `m5stack/M5_Hardware` (sparse)
- Captured additional web docs via defuddle: CoreS3 full page, UiFlow2 guide, Arduino guide, XiaoZhi ESP32 README, Mooncake framework README, stack-chan org page, M5Stack Japanese docs, M5Burner docs
- Attempted to clone `meganetaaan/stackchan-firmware` — repo does not exist (404)
- Wrote comprehensive contributing guide at `docs/guides/contributing-to-m5stackchan-research.md`

### Why

The first step in any hardware documentation project is to grab every official resource while it's available. M5Stack hosts PDFs on Aliyun OSS — these URLs can change or go offline. Capturing web pages with defuddle preserves the content in a searchable, offline-friendly format.

### What worked

- All 23 PDFs downloaded successfully (verified sizes — total ~40MB)
- All 4 Git repos cloned successfully
- defuddle produced excellent markdown extractions for most M5Stack documentation pages
- The K151 product page was the single most comprehensive resource — it contained links to all schematics, datasheets, pin maps, and the factory firmware guide
- The `repos.json` in the firmware repo revealed the full dependency graph: mooncake, smooth_ui_toolkit, xiaozhi-esp32, ArduinoJson, esp-now

### What didn't work

- `meganetaaan/stackchan-firmware` repo returned 404 — does not exist (the community firmware is inside the `meganetaaan/stackchan` repo instead)
- `stackchan.world` returned nearly empty content via defuddle — JS-rendered SPA, needs browser capture
- `xiaozhi.me` returned empty content via defuddle — JS-rendered SPA, needs browser capture
- `makerworld.com/en/@M5Stack/upload` returned empty — JS-rendered, needs browser capture
- Community docs site `stack-chan.github.io/stackchan/` returned empty — likely JS-rendered
- Kagi web search via `surf` failed with socket error (surf CLI not running) — had to rely on direct URL knowledge and the official doc pages instead

### What I learned

- The M5StackChan firmware is built on ESP-IDF v5.5.4 and uses the "mooncake" app framework (by @Forairaaaaa) with XiaoZhi AI agent integration
- The firmware has a dual-boot architecture: mooncake app launcher (with 8 apps) runs first, then optionally hands off to XiaoZhi which never returns
- The partition table uses dual OTA slots (4.9MB each) + a 4MB SPIFFS assets partition
- The BSP library includes drivers for FTServo protocol (a serial servo communication protocol), Si12T touch, and PY32L020 I/O expander
- The server is written in Go using the GoFrame framework with MySQL storage
- The mobile app is Flutter-based with Three.js 3D face rendering and RSA-encrypted BLE communication

### What was tricky to build

- The docmgr root is at a different path (`esp32-s3-m5/ttmp`) than the CWD (`M5StackChan/`), causing some initial confusion about where ticket files live vs. where to put sources
- defuddle's single-line output issue requires the `fold -w 100 -s` pipe — without it, markdown files appear to be 1 line with all content concatenated
- URL-escaped paths in the M5Stack OSS links (e.g., `%20` in datasheet URLs) needed to be handled carefully with curl

### What warrants a second pair of eyes

- Verify all 23 PDFs are valid (not 404 pages disguised as PDFs) — check file signatures
- The hardware architecture diagram in the contributing guide should be verified against the actual schematics
- The I2C address map should be cross-referenced with the schematic PDFs

### What should be done in the future

- Capture JS-rendered pages (xiaozhi.me, stackchan.world, makerworld, stack-chan.github.io) using Playwright browser tools
- Research the original community StackChan project history (pre-M5Stack commercial version)
- Walk through the firmware build process and document it
- Self-host the StackChan server and document the setup
- Find and archive community mods, custom firmware, 3D-printable accessories

### Code review instructions

- Start with `docs/guides/contributing-to-m5stackchan-research.md` for the full picture
- Verify PDF downloads: `file sources/*.pdf` should show "PDF document"
- Check git repo freshness: `cd sources/StackChan && git log --oneline -3`
- Verify the contributing guide's hardware architecture diagram against `sources/schematic-adapter-board.pdf`

### Technical details

- Total sources downloaded: 23 PDFs, 20 markdown web captures, 4 git repos
- Key firmware entry point: `sources/StackChan/firmware/main/main.cpp`
- Key BSP header: `sources/StackChan-BSP/src/M5StackChan.h`
- Partition scheme: dual OTA (0x4f0000 each) + 4MB SPIFFS assets + coredump

## Step 2: Deep Research — XiaoZhi MCP Protocol, Community Projects, Original StackChan

This step targeted three gaps: the XiaoZhi AI platform's MCP protocol (the core mechanism for AI agent control), the original community StackChan project (pre-M5Stack commercial version), and JS-rendered sites that defuddle couldn't capture.

### Prompt Context

**User prompt (verbatim):** "continue research"
**Assistant interpretation:** Continue the M5StackChan research by tackling the remaining items from the contributing guide's checklist.
**Inferred user intent:** Fill in the most impactful gaps — XiaoZhi MCP protocol docs, community project history, and JS-rendered sites.

### What I did

- Verified all PDFs: found `ds-aw9523b-io-expander.pdf` was an XML 404 error page (M5Stack OSS link broken), removed it. All other 24 PDFs valid.
- Used Playwright browser to capture JS-rendered sites:
  - **xiaozhi.me**: SPA with "Let's explore the fascinating world of AI and robotics together!", links to DIY Tutorial, GitHub, Console. Developed by Tenclass (chenlan@tenclass.com)
  - **stackchan.world**: Just displays "Coming Soon" — not fully launched
  - **MakerWorld M5Stack**: 4 StackChan 3D-printable accessories (Pharaoh, American football, Christmas hat, Saint Seiya decorations)
  - **Cosense wiki** (scrapbox.io/stack-chan): Rich community knowledge base with AI Stack-chan variant table, troubleshooting guides, event history, 3D printing recommendations, and academic survey reference
- Discovered and cloned the **stack-chan GitHub org** (5 repos):
  - `stack-chan/stack-chan`: The original JavaScript/Moddable SDK firmware (Apache 2.0, 1.5k stars)
  - `stack-chan/stackchan-arduino`: Arduino framework library with full API docs
  - `stack-chan/awesome-stack-chan`: Curated list of projects/resources
  - `stack-chan/m5stack-avatar`: Avatar face rendering library for M5Stack
- Cloned community firmware variants:
  - `robo8080/AI_StackChan2`: Arduino-based AI version using VOICEVOX + Google Cloud STT/OpenAI Whisper
  - `mongonta0716/M5Unified_StackChan`: MP3-playing StackChan with YAML config, SCS0009 servo support
  - `mongonta0716/3DPrinter_Models`: Community 3D-printable accessories
- Captured **XiaoZhi MCP protocol** full specification:
  - JSON-RPC 2.0 over WebSocket or MQTT transport
  - Device is MCP server, backend is MCP client
  - Flow: hello → initialize → tools/list → tools/call
  - User-only tools (hidden from AI, for companion apps)
  - Device-initiated notifications
- Captured XiaoZhi docs: MCP usage (IoT control tool registration), custom board guide, WebSocket protocol
- Read the original stack-chan's **Roadmap** (detailed 2026-2027 plan with 6 phases from architecture alignment to advanced AI capabilities)
- Read the original stack-chan's **CLAUDE.md** (Moddable SDK TypeScript architecture, host/MOD system, npm-based workflow)
- Downloaded Hat MiniJoyC I2C protocol PDF (remote controller), found Hat MiniJoyC schematic PDF is 404

### Why

The XiaoZhi MCP protocol is the single most important architectural document for understanding how the StackChan AI Agent works — it's the mechanism by which the LLM controls servos, LEDs, NFC, and other hardware. The original community project gives context for the pre-commercial history and the Moddable/TypeScript codebase. JS-rendered sites were previously inaccessible via defuddle.

### What worked

- Playwright browser captured all JS-rendered sites perfectly
- The stack-chan GitHub org repos are a treasure trove — especially the original firmware with its roadmap, CLAUDE.md, and the community's awesome list
- The Cosense wiki revealed the full AI Stack-chan variant ecosystem (AI Stack-chan 2, AI Stack-chan Ex, M5Unified StackChan)
- MCP protocol doc gives complete JSON-RPC 2.0 message format and interaction flow
- The community stackchan-arduino API docs are clean and comprehensive

### What didn't work

- `ds-aw9523b-io-expander.pdf` is 404 on M5Stack's OSS — the AW9523B datasheet link in the official docs is broken
- Hat MiniJoyC schematic PDF is also 404
- Playwright evaluate saves JS-escaped strings (\\n, \\t) rather than clean markdown — raw captures need post-processing
- The XiaoZhi Python server README (427 lines via defuddle) is in Chinese — the Go and Java server repos were not captured yet

### What I learned

- **Two StackChan lineages exist**: the original community project (JavaScript/Moddable SDK by @meganetaaan, born 2021-07-02) and the M5Stack commercial product (ESP-IDF C++ by M5Stack, 2025+). They share the name and community but have completely different codebases.
- **Mooncake** is the M5Stack firmware's app framework — it manages app lifecycle (install, open, close, uninstall) with a singleton pattern. The factory firmware runs 8 apps through it.
- **XiaoZhi** is developed by Tenclass and uses MCP (Model Context Protocol) to let an LLM discover and call device tools over JSON-RPC 2.0. The device is the MCP *server*, the backend is the MCP *client*.
- **The original stack-chan has a v1.0 case redesign** using Dynamixel XL330 servos, with full KiCad schematics and Gerber files for the community PCB.
- **The community Cosense wiki** documents the "M5Stackchan troubleshooting" flow: custom firmware users must flash factory firmware via M5Burner first to pair/unbind, then re-flash custom firmware.
- **Stack-chan Ex** by the community supports CoreS3 and SCS0009 servos — it's the closest community analogue to the M5Stack commercial product.

### What was tricky to build

- Playwright's `evaluate` returns JS string representations with escaped newlines, not clean markdown — the raw captures need a Python sed-like post-processing step to be readable
- The stack-chan GitHub org is separate from meganetaaan's personal repo — there are actually two community repos: `meganetaaan/stackchan` (M5Stack co-creation version) and `stack-chan/stack-chan` (the original JavaScript/Moddable version). This was confusing at first.

### What warrants a second pair of eyes

- The MCP protocol doc was captured from GitHub rendered markdown — verify the JSON examples are complete and not truncated
- The raw captures (xiaozhi-mcp-usage-raw.txt, xiaozhi-custom-board-raw.txt, xiaozhi-websocket-protocol-raw.txt) contain JS-escaped strings that need post-processing before they're useful
- The community variant table in the contributing guide should be updated to include the AI Stack-chan Ex variant

### What should be done in the future

- Post-process the raw XiaoZhi doc captures into clean markdown
- Capture the MQTT+UDP protocol document (the alternative to WebSocket)
- Read and summarize the XiaoZhi Python server README (self-hosting instructions)
- Document the M5StackChan factory firmware's HAL layer (how it integrates XiaoZhi with the mooncake app framework)
- Add a "StackChan Lineage" section to the contributing guide explaining the two codebases

### Code review instructions

- Start with `sources/xiaozhi-mcp-protocol.md` for the MCP protocol
- Compare `sources/stack-chan-og/README.md` (original JS) with `sources/StackChan/README.md` (M5Stack commercial) to understand the lineage
- Read `sources/community-cosense-wiki-summary.md` for community knowledge
- Check `sources/stack-chan-og/docs/ROADMAP.md` for the original project's forward plan

### Technical details

- Total sources: 23 valid PDFs, 35 markdown/text captures, 11 git repos, 4 screenshots
- Key new repos: stack-chan-og (original Moddable JS), AI_StackChan2, M5Unified_StackChan, awesome-stack-chan, m5stack-avatar, stackchan-arduino, 3DPrinter_Models
- MCP protocol version: 2024-11-05 (based on MCP spec)
- XiaoZhi transport options: WebSocket or MQTT+UDP (both carry MCP messages)

## Step 3: OpenAI Realtime API & Responses API on ESP32 Research

Researched OpenAI Realtime API and Responses API implementations specifically for ESP32, looking for alternatives to the XiaoZhi cloud for M5StackChan voice AI.

### Prompt Context

**User prompt (verbatim):** "now search for openai realtime api for esp32, and openai responses api for esp32"
**Assistant interpretation:** Search for existing ESP32 projects that integrate with OpenAI's Realtime API (speech-to-speech) and Responses API (multimodal request-response), as potential alternatives or supplements to the XiaoZhi cloud for M5StackChan.
**Inferred user intent:** Understand the landscape of OpenAI API options for ESP32 voice AI, find reusable implementations, and evaluate feasibility for M5StackChan.

### What I did

- Searched GitHub for repos implementing OpenAI Realtime API on ESP32 — found 7 results, cloned top 4
- **Key discovery: `rudyll/stackchan_ha_addons`** — a Home Assistant add-on that replaces XiaoZhi cloud with OpenAI Realtime/Gemini Live, specifically for StackChan! No firmware modifications needed.
- Cloned ElatoAI (1.8k stars) — the most comprehensive ESP32 voice AI platform with 100+ models
- Cloned Voice-Assistant-5 — clean Arduino implementation with exceptional architecture docs (dual-core FreeRTOS, PSRAM ring buffers, tool calling flow)
- Cloned Bob — hexapod using Realtime API + Structured Output for vision
- Captured OpenAI Realtime API guide (179 lines) and Voice Agents guide (134 lines) via defuddle
- Searched for Responses API on ESP32 — found only 2 repos, neither truly uses the Responses API
- Captured Responses vs Chat Completions comparison (527 lines)
- Wrote comprehensive research summary with 4 implementation options for M5StackChan

### Why

The XiaoZhi cloud dependency is a significant limitation — it requires a XiaoZhi account, sends data to Chinese servers, and has limited model selection. OpenAI Realtime API offers lower latency, more voices, and better English language support. The Responses API is less suitable for real-time voice on ESP32 but could be useful for vision processing.

### What worked

- The GitHub search was productive — found the StackChan-specific HA add-on immediately
- ElatoAI's edge server architecture is a clever solution to ESP32's limited compute
- Voice-Assistant-5's architecture docs are exceptionally detailed — the best reference for implementing Realtime API on ESP32
- The defuddle captures of OpenAI docs worked for the guides but not for the API reference pages (JS-rendered)

### What didn't work

- OpenAI's API reference pages (platform.openai.com/docs/api-reference/*) are JS-rendered SPA — defuddle returns empty content, Playwright timed out
- No genuine Responses API implementations on ESP32 were found — the API is poorly suited for real-time voice on embedded devices
- The Siddheshgupta7905/esp32-voice-interaction-system repo uses standard Chat Completions, not the Responses API

### What I learned

- **Realtime API protocol on ESP32 is well-understood:** WebSocket to `api.openai.com/v1/realtime`, PCM 24kHz, `session.update` → `input_audio_buffer.append/commit` → `response.output_audio.delta` → tool calls → `function_call_output` + `response.create`
- **Tool calling is the key pattern:** The model calls tools (like `set_display_emotion`, `set_volume`) as complete responses. The client must submit results and explicitly request continuation with `response.create`. This is the same pattern as XiaoZhi's MCP.
- **StackChan HA Add-ons is a drop-in replacement:** It intercepts the XiaoZhi WebSocket protocol and proxies to OpenAI/Gemini. The device firmware doesn't need any changes.
- **ElatoAI's Opus compression** is significant — 12kbps vs raw PCM at ~384kbps. This reduces bandwidth by 32x.
- **Responses API is not viable for real-time voice on ESP32** — it's request-response, requires complete audio payloads, and has 2-5s latency vs <1s for Realtime API streaming.

### What was tricky to build

- The GitHub search results parsing was inconsistent — Playwright evaluate returned different element selectors on different pages. The `document.body.innerText` approach was most reliable.
- OpenAI's documentation platform uses heavy client-side rendering, making it difficult to capture API reference content programmatically.

### What warrants a second pair of eyes

- The Voice-Assistant-5 architecture doc describes the exact tool calling flow — verify this matches the current OpenAI Realtime API spec (it references `gpt-realtime-2` and GA API changes)
- The StackChan HA Add-ons NVS injection method — verify the OTA URL override mechanism works with the current M5StackChan firmware version
- The audio pipeline in StackChan HA Add-ons uses OPUS at 16kHz, but OpenAI Realtime API outputs at 24kHz — verify the resampling is handled correctly

### What should be done in the future

- Test StackChan HA Add-ons with actual M5StackChan hardware
- Evaluate ElatoAI's edge server as a more scalable alternative to the HA add-on
- Consider implementing Realtime API directly in M5StackChan firmware (replacing XiaoZhi entirely)
- Research Gemini Live API as an alternative to OpenAI (mentioned in StackChan HA Add-ons but not deeply investigated)

### Code review instructions

- Start with `sources/research-openai-realtime-responses-esp32.md` for the full analysis
- Read `sources/stackchan_ha_addons/README.md` for the StackChan-specific solution
- Study `sources/Voice-Assistant-5/voice_agent_5/docs/architecture.md` for the most detailed ESP32 Realtime API implementation reference
- Check `sources/ElatoAI/README.md` for the edge server architecture

### Technical details

- New repos cloned: ElatoAI, stackchan_ha_addons, Voice-Assistant-5, bob-esp32-hexapod
- Total repos now: 15
- Key OpenAI Realtime API models: gpt-realtime-2, gpt-realtime-mini, gpt-realtime-1.5
- Key Gemini Live model: gemini-2.5-flash-native-audio-latest

## Step 4: Text Chatbot LLM APIs on ESP32 Research

Researched how to call text-based LLM APIs (OpenAI, Claude, Gemini, etc.) from ESP32 for normal chatbot duties — conversation, tool calling, reasoning.

### Prompt Context

**User prompt (verbatim):** "i want to do responses API not for audio, but for normal chatbot duties. What do people use for that? are there claude APIs? openai completions APIs? gemini APIs?"
**Assistant interpretation:** Find out what ESP32 libraries and approaches exist for calling text-based LLM APIs (Chat Completions, Responses, Claude Messages, Gemini) for non-audio chatbot use cases.
**Inferred user intent:** Understand the full landscape of text LLM API options for ESP32, find the best library/approach, and evaluate which providers are practical for M5StackChan.

### What I did

- Searched GitHub for ESP32 + OpenAI/Claude/Gemini/API client libraries
- Found **ESP32_AI_Connect** (33 stars) — the most complete unified Arduino library supporting OpenAI, Claude, Gemini, DeepSeek, Grok, and OpenAI-compatible endpoints
- Found **espai** (13 stars) — alternative unified library
- Found **ESP32_ChatGPT** (43 stars) — basic OpenAI-only library
- Found **openai-c** (4 stars) — lightweight C client
- Found **ESP32-AI-Chatbot** (16 stars) — Ollama client
- Cloned ESP32_AI_Connect, openai-c, ESP32-AI-Chatbot
- Read ESP32_AI_Connect source in detail — tool calling, streaming, auto-retry, FreeRTOS thread safety
- Confirmed: no ESP32-specific library for the OpenAI Responses API, but it's a simple HTTP POST
- Confirmed: XiaoZhi doesn't call LLMs directly — its server does
- Wrote comprehensive research summary with provider comparison and M5StackChan recommendations

### Why

The Realtime API is for voice. But a chatbot needs text reasoning too — for tool calling, knowledge queries, conversation management. The user specifically wants to understand what text APIs are available and how to use them on ESP32.

### What worked

- ESP32_AI_Connect is exactly the right tool — unified interface across all major providers
- The GitHub search for "esp32 anthropic claude api" surfaced it as the top result
- Reading the source code confirmed it handles Claude's unique requirements (required max_tokens, system prompt format)
- The Ollama option is practical for offline/privacy use

### What didn't work

- GitHub search for "openai responses api esp32" returns almost nothing — the Responses API is too new
- No one has built an ESP32 library specifically for the Responses API yet
- The XiaoZhi server README is mostly in Chinese, making it harder to extract API details

### What I learned

- **Text LLM APIs on ESP32 are straightforward** — just HTTP POST with JSON. No WebSocket complexity like Realtime API.
- **ESP32_AI_Connect is the clear winner** — one library, all providers, tool calling, streaming, auto-retry.
- **The Responses API vs Chat Completions distinction matters less on ESP32** — both are HTTP POST, and Chat Completions has broader library support. The Responses API's main advantage (built-in tools like web search, file search) is less useful on ESP32 where you're doing local tool calling via MCP or similar.
- **Claude's API has unique requirements** — `max_tokens` is required, system prompt is top-level, `anthropic-version` header needed. ESP32_AI_Connect handles all of this.
- **Gemini is the easiest to start with** — API key in URL (no Bearer token), free tier, fast flash model.
- **Ollama on LAN is ideal for privacy/offline** — no TLS, no API key, local models.
- **XiaoZhi's architecture separates concerns** — ESP32 handles audio I/O, server handles LLM/TTS. This is why the ESP32 never calls LLM APIs directly in the factory firmware.

### What was tricky to build

- The GitHub search results were thin for "openai responses api esp32" — the Responses API is simply too new for the embedded ecosystem.
- Multiple GitHub searches were needed to find all the relevant repos — no single search query surfaced everything.

### What warrants a second pair of eyes

- ESP32_AI_Connect's tool calling flow — verify the follow-up mechanism handles multi-turn tool calls correctly
- Memory usage with large LLM responses — ESP32_AI_Connect uses DynamicJsonDocument which allocates on heap. With 8MB PSRAM this should be fine, but verify the library uses PSRAM allocation.
- The OpenAI Responses API might be worth a direct HTTP implementation — it could simplify conversation management via `previous_response_id`

### What should be done in the future

- Build a small test with ESP32_AI_Connect on M5StackChan hardware
- Evaluate whether the Responses API's `previous_response_id` simplifies conversation management vs manual message arrays
- Test Ollama integration for offline/privacy use case
- Consider contributing Ollama support to ESP32_AI_Connect (it's in progress)

### Code review instructions

- Start with `sources/research-text-chatbot-apis-esp32.md` for the full analysis
- Read `sources/ESP32_AI_Connect/src/ESP32_AI_Connect.h` for the API surface
- Check `sources/ESP32_AI_Connect/src/AI_API_Claude.h` for Claude-specific handling
- Review the tool calling example in `sources/ESP32_AI_Connect/examples/`

### Technical details

- New repos cloned: ESP32_AI_Connect, openai-c, ESP32-AI-Chatbot-Ollama
- Total repos now: 18
- Key providers: OpenAI (Chat Completions + Responses), Claude (Messages), Gemini (generateContent), DeepSeek, Grok, Ollama, OpenRouter
