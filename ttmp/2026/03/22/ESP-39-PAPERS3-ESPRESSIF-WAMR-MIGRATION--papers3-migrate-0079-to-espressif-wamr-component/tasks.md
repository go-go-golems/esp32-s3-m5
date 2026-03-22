# Tasks

## Completed

- [x] Create ticket `ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION`
- [x] Inspect the current `0079` WAMR dependency layout
- [x] Write the migration guide
- [x] Create the migration diary

## In Progress

- [x] Task 1: Switch `0079` from the upstream WAMR package to Espressif's official component package
- [x] Task 1.1: Replace the upstream dependency in `main/idf_component.yml`
- [x] Task 1.2: Update `main/CMakeLists.txt` to depend on the new component alias
- [x] Task 1.3: Refresh the resolved dependency state so `dependencies.lock` reflects the new package
- [x] Task 1.4: Rebuild `0079` against `ESP-IDF 5.3.4`
- [ ] Task 1.5: Record the outcome in the ticket and commit the migration slice

## Planned

- [ ] Task 2: Compare the migrated build surface against the previous upstream integration
- [x] Task 2.1: Confirm whether the Kconfig symbols consumed by `sdkconfig.defaults` and `wasm_runtime_service.cpp` remain compatible
- [x] Task 2.2: Record the exact resolved Espressif package version
- [x] Task 2.3: Note any new warnings, missing symbols, or build regressions introduced by the migration
- [ ] Task 2.4: Decide whether the next ticket should be runtime A/B validation on hardware or additional static integration cleanup
- [ ] Task 2.5: Decide whether to explicitly clean the stale generated `bytecodealliance__wasm-micro-runtime` directory or leave it as a harmless local cache artifact
