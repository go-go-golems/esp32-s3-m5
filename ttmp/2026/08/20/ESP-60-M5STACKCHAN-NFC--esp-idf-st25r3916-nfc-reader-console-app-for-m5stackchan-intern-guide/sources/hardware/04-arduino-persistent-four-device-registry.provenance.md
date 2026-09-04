# Arduino persistent four-device registry provenance

- Captured: 2026-08-21
- Firmware commit: `32d64476e6c4fdcb3e174957a75d97d36c360377`
- Multi-tag implementation commit: `eca56bf693d514c42e73d4ac00fbc760e7b78979`
- Device: M5Stack CoreS3 at `/dev/ttyACM0`, 115200 baud
- Continuous capture PID: 3726922; stopped cleanly before another serial owner
- Raw capture SHA-256: `f6351db0377f7ca24d09aa7e74e46ba397c4214bb5420781444cb51e5cffaf43`
- Exact gzip SHA-256: `866abc5a505bb3f5abeca892c1fda2a4bd4c966de9911cba8df27be0faa4c14c`

The uninterrupted session ran for 197 cycles. It retained four unique UIDs.
At cycle 197 the current scan contained zero PICCs while `seen=4`, directly
proving empty scans do not clear the registry. No `M5_I2C_FAIL` records or
phase-level transport failures occurred.

Retained UIDs and maximum observation counts:

- `04DAF74D9E6180`: 8
- `04ACE84D9E6180`: 10
- `0491D44C9E6180`: 10
- `04C9C54C9E6180`: 3
