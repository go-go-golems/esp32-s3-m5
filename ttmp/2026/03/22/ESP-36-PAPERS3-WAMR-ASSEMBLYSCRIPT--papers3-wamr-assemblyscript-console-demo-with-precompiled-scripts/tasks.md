# Tasks

## Completed

- [x] Create ticket `ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT`
- [x] Add topic vocabulary entries for `papers3`, `wasm`, and `assemblyscript`
- [x] Review the last PaperS3 tickets and the earlier script-runtime ticket for local prior art
- [x] Inspect the existing PaperS3 app structure and the ESP-IDF `esp_console` APIs used in this repo
- [x] Write a detailed analysis, design, and implementation guide for a new intern
- [x] Write and store a diary entry for this investigation/documentation pass
- [x] Validate the ticket and upload the bundle to reMarkable

## Planned Firmware Work

- [x] Task 1: Create new firmware project `0079-papers3-wamr-assemblyscript-console`
- [x] Task 1.1: Add `CMakeLists.txt`, `README.md`, `sdkconfig.defaults`, `partitions.csv`, and `main/CMakeLists.txt`
- [x] Task 1.2: Reuse the donor PaperS3 component stack via `EXTRA_COMPONENT_DIRS`
- [x] Task 1.3: Keep the target pinned to ESP-IDF `5.3.4`
- [x] Task 1.4: Keep the interactive console on USB Serial/JTAG
- [x] Task 1.5: Commit scaffold + ticket bookkeeping

- [x] Task 2: Bring up the console skeleton
- [x] Task 2.1: Add `app_main.cpp` and a minimal app class or startup module
- [x] Task 2.2: Add `console_repl.*` to bootstrap `esp_console`
- [x] Task 2.3: Add `wasm_command.*` with `examples`, `list`, and placeholder `run`
- [x] Task 2.4: Verify a clean `idf.py build`
- [x] Task 2.5: Commit console bootstrap + diary update

- [ ] Task 3: Integrate WAMR
- [x] Task 3.1: Add an `idf_component.yml` dependency for upstream WAMR
- [x] Task 3.2: Configure WAMR for interpreter-first bring-up
- [x] Task 3.3: Add `wasm_runtime_service.*`
- [x] Task 3.4: Initialize the runtime and report status/errors
- [x] Task 3.5: Commit WAMR integration + diary update

- [ ] Task 4: Add host-side AssemblyScript build assets
- [x] Task 4.1: Create `wasm-src/package.json`
- [x] Task 4.2: Create `wasm-src/asconfig.json`
- [x] Task 4.3: Create shared host import declarations
- [x] Task 4.4: Add a script that builds all demo modules to `.wasm` and `.wat`
- [ ] Task 4.5: Commit AssemblyScript pipeline + diary update

- [ ] Task 5: Add embedded demo registry
- [ ] Task 5.1: Create generated or embedded wasm asset inputs
- [ ] Task 5.2: Add `wasm_module_registry.*`
- [ ] Task 5.3: Implement `wasm list` and `wasm info`
- [ ] Task 5.4: Verify embedded assets are available in firmware builds
- [ ] Task 5.5: Commit registry + diary update

- [ ] Task 6: Add the first working host API and first runnable demo
- [ ] Task 6.1: Add `papers3_canvas.*`
- [ ] Task 6.2: Add `wasm_host_api.*` with a small numeric drawing ABI
- [ ] Task 6.3: Add the first AssemblyScript demo module
- [ ] Task 6.4: Implement `wasm run <name>`
- [ ] Task 6.5: Verify the guest export runs successfully
- [ ] Task 6.6: Commit first end-to-end demo + diary update

- [ ] Task 7: Expand the curated demo pack
- [ ] Task 7.1: Add at least four more visually distinct AssemblyScript demos
- [ ] Task 7.2: Add command examples and runtime status output
- [ ] Task 7.3: Rebuild and smoke test all demos
- [ ] Task 7.4: Commit demo pack + diary update

- [ ] Task 8: Hardware validation
- [ ] Task 8.1: Flash to PaperS3 hardware
- [ ] Task 8.2: Verify console execution and display output
- [ ] Task 8.3: Measure memory headroom and repeated-run stability
- [ ] Task 8.4: Record findings in ticket docs
