# M5Unit-NFC source snapshot used for ESP-60 debugging

Authoritative upstream: https://github.com/m5stack/M5Unit-NFC

Snapshot commit: `93745b547364f310cd64b5155a870103a7800a5d`
Captured: 2026-08-21

Files:

- `unit_ST25R3916.cpp`: chip initialization, Space-A/Space-B access, field control, IRQ/FIFO helpers.
- `unit_ST25R3916_nfca.cpp`: exact ISO14443-A configuration, REQA/WUPA, anticollision/select.
- `unit_ST25R3916_util.cpp`: integer timer conversions (`calculate_nrt`).
- `nfc_layer_a.cpp`: high-level `detect()` loop used by the official StackChan example.

These implementation files are required for line-by-line comparison. Earlier ticket sources only included headers and therefore missed reader-critical Space-B writes in `configure_nfc_a()`.
