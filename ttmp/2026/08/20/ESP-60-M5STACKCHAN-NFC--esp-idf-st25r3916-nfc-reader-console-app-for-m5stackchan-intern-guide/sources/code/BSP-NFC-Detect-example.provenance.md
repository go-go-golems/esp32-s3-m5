# Provenance: official StackChan NFC Detect example

Upstream: https://github.com/m5stack/StackChan-BSP/blob/f7ed40e6f5d9a1d08440cb926f3a0865b81882f8/examples/NFC/Detect/Detect.ino
Repository commit: `f7ed40e6f5d9a1d08440cb926f3a0865b81882f8`
Captured: 2026-08-21

The example initializes `UnitNFC` on `M5.In_I2C`, calls `Units.update()`, then repeatedly invokes `NFCLayerA::detect(piccs)`, identifies each PICC, and deactivates it.
