# Changelog

## 2026-01-21

- Initial workspace created
- Extracted WS281x engine into `components/ws281x` and updated `0049` to consume it (commit 17b83eb)
- Debugged multicast discovery regression by adding host `--bind-ip` and node-side PING/PONG instrumentation (commits 07edd6a, 8c5b566)
- Fixed `mled_node` stack protection fault during cue runs (commit e576d5f)
- Validation: flashed `0049` and drove all WS281x patterns via `tools/mled_smoke.py` (visual confirmation)
