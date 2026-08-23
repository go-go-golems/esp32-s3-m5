# Tasks

## TODO

- [ ] Flash on real hardware (needs `sudo modprobe cdc_acm` -> /dev/ttyACM0); run `qr status` to confirm the on-device probe (firmware version reply)
- [ ] Validate scan pump / \r\n terminator against real codes; tune quiet-time if reads glue/split
- [ ] Phase 5 (future): NVS history, touch start/stop, symbology badge, optional image preview

## DONE

- [x] P0: probe realized as on-device firmware (host pyserial can't reach stacked module UART); script validated
- [x] P1: project skeleton + display boot (build clean, 447KB) — commit 9981d58f
- [x] P2: scanner driver (UART+I2C expander) + `qr status` probe + build (513KB) — commit bd0e0a5b
- [x] P3: on-screen UI (current code + history + buttons) + build (514KB) — commit 79eeb454
- [x] P4: full qr console (start/stop/mode/light/brightness/beep/reset) + README; fullclean reproducible (515KB) — commit 52d01d75
- [x] Diary Steps 3-6; sources/scripts saved; design guide uploaded to reMarkable
- [ ] Physically verify Module13.2 UART interface switch, G13/G14 QR routing DIPs, H2 NC DIPs, and stack seating; rerun status/scan <!-- t:o1q3 -->
