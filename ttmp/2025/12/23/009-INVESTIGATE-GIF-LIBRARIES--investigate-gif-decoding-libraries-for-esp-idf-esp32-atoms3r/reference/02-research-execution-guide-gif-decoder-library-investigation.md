---
Title: 'Research execution guide: GIF decoder library investigation'
Ticket: 009-INVESTIGATE-GIF-LIBRARIES
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - assets
    - serial
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T21:50:00.000000000-05:00
WhatFor: "Step-by-step guide for investigating GIF decoder libraries for ESP-IDF/ESP32-S3 (AtomS3R)"
WhenToUse: "When executing the library research phase of ticket 009"
---

# Research execution guide: GIF decoder library investigation

## Goal

This guide provides a **reproducible, step-by-step procedure** to investigate candidate GIF decoder libraries and document factual findings (API, license, limitations, integration complexity) so we can make an informed recommendation for ticket 008 (AtomS3R serial-controlled GIF display).

**Your output**: A comparison table and recommendation document (`analysis/02-gif-decoder-survey-esp-idf-candidates-constraints-and-recommendation.md`) that answers: "Which library should we use, and why?"

## Context (read this first)

Before starting, read these documents to understand the constraints:

1. **Parent ticket context**:
   - `../008-ATOMS3R-GIF-CONSOLE/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
   - `../008-ATOMS3R-GIF-CONSOLE/reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md`

2. **Research plan**:
   - `analysis/01-research-plan-gif-decoding-options-for-atoms3r.md` (evaluation criteria)

3. **Known-good display pattern** (you'll need this for testing):
   - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp` (M5GFX canvas + `waitDMA()`)

**Key constraints to remember**:
- Target: ESP-IDF 5.4.1, ESP32-S3
- Display: 128×128 RGB565 canvas (M5GFX)
- Input: Flash-mapped blob (via `esp_partition_mmap`) is preferred over filesystem
- Output: Must convert to RGB565 for M5GFX canvas
- Correctness matters: disposal modes, transparency, local palettes

## Candidate libraries (investigate these)

Investigate each library in this order:

1. **esp-idf-libnsgif** (ESP-IDF native)
   - Repo: `https://github.com/UncleRus/esp-idf-libnsgif`
   - Wraps NetSurf's libnsgif for ESP-IDF

2. **gifdec** (minimal C decoder)
   - Repo: `https://github.com/lecram/gifdec`
   - Small, public domain

3. **AnimatedGIF** (Arduino-focused, but has ESP32 examples)
   - Repo: `https://github.com/bitbank2/AnimatedGIF`
   - Widely used, callback-based

4. **LVGL GIF support** (only if LVGL is acceptable)
   - Repo: `https://github.com/lvgl/lv_lib_gif` (archived; merged into LVGL main)
   - LVGL docs: `https://docs.lvgl.io/master/details/libs/gif.html`

## Investigation procedure (for each library)

### Step 1: Clone and inspect the repository

```bash
# Create a temporary workspace
cd /tmp
mkdir gif-research
cd gif-research

# Clone the repo
git clone --depth 1 <REPO_URL> <LIBRARY_NAME>
cd <LIBRARY_NAME>
```

**What to look for**:
- README.md (license, usage, examples)
- LICENSE file (exact license text)
- Source code structure (header files, main implementation)
- Examples directory (ESP32/ESP-IDF examples?)
- Build files (`CMakeLists.txt`, `component.mk`, `idf_component.yml`)

### Step 2: Read the README and documentation

**Questions to answer**:
- What is the license? (MIT, BSD, GPL, public domain?)
- What platforms does it claim to support? (ESP32? ESP-IDF specifically?)
- Are there ESP-IDF examples or just Arduino?
- What are the known limitations or requirements?

**Record findings** in your notes document (see template below).

### Step 3: Inspect the API (header files)

Find the main public header file(s) and read them:

```bash
# Find header files
find . -name "*.h" -type f | grep -v test | grep -v example | head -20

# Read the main header
cat <MAIN_HEADER>.h
```

**What to document**:

1. **Input model** (how does it read GIF data?):
   - File path (`const char* filename`)?
   - File descriptor (`int fd`)?
   - Memory buffer (`const uint8_t* data, size_t len`)?
   - Can it read from a memory-mapped region? (important for flash-mapped assets)

2. **Output model** (what does decoding produce?):
   - Line-by-line callback? (`void callback(int y, uint8_t* line, int width)`)
   - Full-frame buffer? (`uint8_t* frame_buffer`)
   - Paletted indices? (`uint8_t* indices, palette_t* palette`)
   - RGB888? (`uint8_t* rgb_buffer`)
   - RGB565? (rare, but check)

3. **Frame control**:
   - How to advance to next frame?
   - How to get frame delay?
   - How to rewind/loop?
   - How to query frame count?

4. **Memory model**:
   - Does it allocate memory internally? (`malloc`/`new`)?
   - Can you provide your own buffers?
   - What's the peak RAM usage? (documented or must measure)

### Step 4: Check ESP-IDF integration

**For each library, determine**:

- **Packaging**:
  - Is there an `idf_component.yml` or `CMakeLists.txt` with `idf_component_register`?
  - Or is it Arduino-only (`library.properties`)?
  - Can it be added as an ESP-IDF component easily?

- **Dependencies**:
  - What does it require? (standard C library? POSIX file I/O? C++?)
  - Any external dependencies? (LVGL? other libraries?)

- **Examples**:
  - Are there ESP-IDF examples in the repo?
  - Or only Arduino sketches?
  - Can Arduino examples be adapted? (how much work?)

### Step 5: Check correctness claims

**Look for**:
- README mentions of "disposal modes", "transparency", "local palette"
- Test files or test suite
- Known limitations (e.g., "no local color table support")

**For gifdec specifically** (known limitation):
- Check if it supports local color tables (it may not)
- Document this limitation clearly

### Step 6: Check license compatibility

**Record**:
- Exact license name and text
- Is it compatible with our use case? (MIT/BSD/public domain = yes; GPL = problematic)
- Any attribution requirements?

### Step 7: Estimate integration effort

**For each library, estimate** (low/medium/high):

1. **ESP-IDF integration effort**:
   - Low: Drop-in ESP-IDF component
   - Medium: Minor CMake/component.mk changes needed
   - High: Arduino-only, requires significant porting

2. **Flash-mapped input effort**:
   - Low: Already supports memory buffer input
   - Medium: Can be adapted (e.g., wrap `esp_partition_mmap` as file I/O)
   - High: Requires file path/FD, hard to adapt

3. **RGB565 output effort**:
   - Low: Already outputs RGB565 or paletted (easy conversion)
   - Medium: Outputs RGB888 (need conversion function)
   - High: Only line callbacks, need to build full frame yourself

## Documentation template

Create a document: `analysis/02-gif-decoder-survey-esp-idf-candidates-constraints-and-recommendation.md`

Use this structure:

```markdown
# GIF decoder survey (ESP-IDF): candidates, constraints, and recommendation

## Executive summary

[1-2 paragraphs: what you investigated, what you found, what you recommend]

## Evaluation criteria

[Copy from analysis/01-research-plan-gif-decoding-options-for-atoms3r.md]

## Candidate comparison table

| Library | License | ESP-IDF Support | Input Model | Output Model | Correctness | Integration Effort | Recommendation |
|---------|---------|-----------------|-------------|--------------|-------------|-------------------|----------------|
| esp-idf-libnsgif | MIT | ✅ Native | Memory buffer | Full frame RGB888 | Full | Low | ⭐ Recommended |
| gifdec | Public domain | ⚠️ C-only | File path | Full frame RGB888 | ⚠️ No local palette | Medium | ⚠️ Limited |
| AnimatedGIF | [License] | ⚠️ Arduino | [Model] | [Model] | [Claims] | [Effort] | [Verdict] |
| LVGL gif | MIT | ✅ Via LVGL | [Model] | LVGL widget | Full | ⚠️ Requires LVGL | ❌ Only if LVGL |

## Detailed findings

### esp-idf-libnsgif

**Repository**: https://github.com/UncleRus/esp-idf-libnsgif

**License**: MIT

**API Model**: [Describe how it works]

**Input**: [File path / FD / memory buffer?]

**Output**: [What format?]

**Correctness**: [Global palette? Local palette? Transparency? Disposal modes?]

**Limitations**: [Any known issues?]

**ESP-IDF Integration**: [How easy?]

**Flash-mapped input**: [Can it read from esp_partition_mmap?]

**RGB565 conversion**: [How easy?]

**Recommendation**: [Why recommend or not]

### gifdec

[Same structure for each library]

### AnimatedGIF

[Same structure]

### LVGL GIF support

[Same structure]

## Recommendation

[Clear recommendation with rationale]

**Recommended approach**: [Library name or "pre-convert to frame pack"]

**Rationale**:
- [Reason 1]
- [Reason 2]
- [Reason 3]

**Next steps**:
- [What to do next if this library is chosen]
```

## Verification commands

### Check if a library has ESP-IDF component support

```bash
cd <LIBRARY_DIR>
ls -la | grep -E "(idf_component|CMakeLists|component.mk)"
cat idf_component.yml 2>/dev/null || cat CMakeLists.txt 2>/dev/null | head -30
```

### Check license

```bash
find . -name "LICENSE*" -o -name "COPYING*" | head -5
cat LICENSE* 2>/dev/null | head -50
```

### Find main API header

```bash
find . -name "*.h" -type f | grep -v test | grep -v example | xargs grep -l "gif\|GIF" | head -5
```

### Check for ESP32/ESP-IDF examples

```bash
find . -type d -name "*esp*" -o -name "*idf*" -o -name "*ESP32*"
find . -name "*.c" -o -name "*.cpp" | xargs grep -l "esp_idf\|ESP_IDF\|esp32" | head -10
```

## Common pitfalls to avoid

1. **Don't assume Arduino libraries work in ESP-IDF**: Many require porting
2. **Don't trust README claims alone**: Check the actual source code
3. **Don't forget to check local palette support**: Many GIFs use local palettes
4. **Don't ignore the I/O model**: Flash-mapped input is important for our use case
5. **Don't skip license checking**: GPL libraries are problematic for commercial use

## When you're done

1. **Fill out the comparison table** with factual findings
2. **Write the recommendation section** with clear rationale
3. **Update the diary** (`reference/01-diary.md`) with what you did
4. **Update tasks.md** to check off completed items
5. **Update changelog.md** with your findings
6. **Commit your work** (only the 009 ticket directory)

## Questions?

If you encounter unclear information or need help:
- Check the parent ticket 008 docs for context
- Look at existing AtomS3R display code (`esp32-s3-m5/0010...`) to understand the integration target
- Document what you couldn't find (that's valuable information too)

## Quick reference: candidate URLs

- **esp-idf-libnsgif**: https://github.com/UncleRus/esp-idf-libnsgif
- **gifdec**: https://github.com/lecram/gifdec
- **AnimatedGIF**: https://github.com/bitbank2/AnimatedGIF
- **LVGL GIF docs**: https://docs.lvgl.io/master/details/libs/gif.html
- **LVGL GIF repo** (archived): https://github.com/lvgl/lv_lib_gif
