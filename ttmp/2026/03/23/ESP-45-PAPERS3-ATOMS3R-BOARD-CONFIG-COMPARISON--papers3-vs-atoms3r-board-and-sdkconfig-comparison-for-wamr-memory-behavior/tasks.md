# Tasks

## Objective

Separate board-level and config-level differences between PaperS3 and AtomS3R from the now-stronger loader-root-cause theory.

## Task List

- [x] Task 1: Diff PaperS3 and AtomS3R sdkconfig/defaults for memory, flash, and PSRAM relevant settings.
- [x] Task 2: Collect official board documentation, schematics, and datasheet references for both boards.
- [x] Task 2.1: Port the minimal direct-embedded loader proof surface to `0081` so AtomS3R can run the same root-cause experiment as PaperS3.
- [ ] Task 2.2: Run `empty-module`, `return-42`, and `return-42 binary_freeable` direct-embedded loads on AtomS3R with the same bounded WAMR const-string trace.
- [ ] Task 2.3: Compare whether AtomS3R takes the same in-place rewrite path and whether later PSRAM touch survives it.
- [ ] Task 3: Write an interpreted comparison, not just a raw diff.
- [ ] Task 4: Keep the diary current and store any comparison scripts in this ticket’s `scripts/` directory.
