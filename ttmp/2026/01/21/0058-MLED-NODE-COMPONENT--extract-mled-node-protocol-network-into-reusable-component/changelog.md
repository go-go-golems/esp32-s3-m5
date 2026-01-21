# Changelog

## 2026-01-21

- Initial workspace created
- Extracted MLED node protocol/network into `components/mled_node` and updated tutorial `0049` to consume it (commit 5cfb36f)
- Validated: `idf.py -C 0049-xiao-esp32c6-mled-node build` (ESP-IDF v5.4.1)
- Validated: flashed to XIAO ESP32C6 + host `mled_ping.py` / `mled_smoke.py` end-to-end protocol flow (TIME_RESP refinement + cue APPLY observed)
