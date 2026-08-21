# Official Arduino four-chip I2C trace provenance

- Captured: 2026-08-21
- Physical condition: four NFC chips placed on the literal top edge of the StackChan head
- Device: M5Stack CoreS3 at `/dev/ttyACM0`, 115200 baud
- Firmware instrumentation commit: `04c8a7c26ead2dcfcdd6f009c9b1012e846b4632`
- Trace ring: 6,000 entries; no dropped records in the first detection phase
- Raw capture: `/tmp/esp60-arduino-trace-four-tags.log`
- Raw capture SHA-256: `60daa11f661a557cd811cc49fc115d7b7efdddab46a6eeb3551ecc69fbbf8894`
- Exact gzip: `02-official-arduino-four-chip-full-i2c-trace.log.gz`
- Gzip SHA-256: `f04fd99758b267dfe3ac88808a530925e47e332c69cc0e5bb76ea18efd450e67`
- Analysis JSON: `02-official-arduino-four-chip-full-i2c-trace.analysis.json`
- Analysis SHA-256: `35d8e8b2dcce135ce7a6ccc1ab510279354550fb4c106a947b1365b07114914d`
- Build log: `/tmp/esp60-arduino-trace-full-ring-build.log`
- Flash log: `/tmp/esp60-arduino-trace-four-tags-flash.log`

Opening the serial port caused a board reset and therefore captured complete initialization. The capture ended immediately after the second detection summary. The first five phases are complete. The second detection phase reports 4,768 successful transactions in firmware; 4,767 individual records reached the host before capture stopped.
