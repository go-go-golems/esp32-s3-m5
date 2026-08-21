# Arduino continuous screen monitor runtime provenance

- Captured: 2026-08-21
- Firmware commit: `64cd7e94e732ab31701e4703b1f0369b89f8429c`
- Physical condition: four NFC chips remained on the literal top edge
- Device: M5Stack CoreS3 at `/dev/ttyACM0`, 115200 baud
- Duration: 20 seconds after attachment-induced reset
- Raw capture SHA-256: `6be111d362f43aa1d4d430d44565aef4a7b1e8adb8f7f75cd17fc1e353e64e51`
- Exact gzip SHA-256: `ff8a0da691825ebb979fbbfaf2b1f42f9524769d8eea8d7ac9935321f9cb7341`
- Build log: `/tmp/esp60-arduino-continuous-screen-build.log`
- Flash log: `/tmp/esp60-arduino-continuous-screen-flash.log`

The monitor completed 49 poll cycles. WUPA succeeded in 47, selection in 31,
and identification in 30. It rotated among three UIDs and completed 8,126
cumulative ST25R3916 transactions with zero M5Unified-level failures. The
firmware remained responsive and continued rendering the 320x240 screen.
