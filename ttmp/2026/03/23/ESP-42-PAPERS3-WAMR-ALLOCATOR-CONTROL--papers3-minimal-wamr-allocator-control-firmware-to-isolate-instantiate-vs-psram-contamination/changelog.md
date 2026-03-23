# Changelog

## 2026-03-23

- Initial workspace created
- Added the first reduced-firmware implementation plan, explicit task list, and debugging diary scaffold for the minimal PaperS3 allocator-control harness
- Reduced `0082` into a displayless allocator-control harness with only `return-42`, `log-only`, instantiate lifecycle commands, and PSRAM/internal-RAM replay probes
- Added reusable serial probe scripts under `ESP-42/scripts` for build/flash/probe and multi-command single-boot capture
- Built `0082` successfully against `ESP-IDF 5.3.4`
- Verified on attached PaperS3 that fresh-boot persistent PSRAM init/touch still succeeds in `0082`
- Verified on attached PaperS3 that `instantiate-bare-keepalive return-42` still poisons later persistent PSRAM touch in the same boot, reproducing the old boundary in the reduced harness
