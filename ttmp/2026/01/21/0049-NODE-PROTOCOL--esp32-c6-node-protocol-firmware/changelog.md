# Changelog

## 2026-01-21

- Initial workspace created
- Added protocol sources reference + detailed C implementation notes
- Wrote textbook-style ESP32-C6 node firmware design (Wi‑Fi via esp_console + MLED/1 node protocol)
- Implemented ESP32-C6 node firmware (`0049-xiao-esp32c6-mled-node`) and extracted reusable components (`components/mled_node`, `components/ws281x`)
- Hardened host test tooling (`--bind-ip`, auto multicast egress selection) and fixed node runtime issues (stack protection fault)
