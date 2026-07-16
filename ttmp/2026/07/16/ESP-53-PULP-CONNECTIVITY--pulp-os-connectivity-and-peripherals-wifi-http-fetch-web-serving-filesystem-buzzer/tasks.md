# Tasks

## TODO

- [x] Phase 0 - Orientation: build/flash 0114, read guide + ESP-51 §6-7, skim go-go-goja modules + widgetdsl <!-- t:uo37 -->
- [x] [P1.1] Buzzer module: LEDC GPIO21 port from UserDemo, tone/beep/stop/melody, owner-tick sequencer <!-- t:rpjk -->
- [x] [P1.2] buzz console command + stdlib regen; audible gate on device <!-- t:h59l -->
- [x] [P1.3] Product chimes: tea READY, postcard SEAL, 2048 merge <!-- t:22os -->
- [x] [P2.1] files module: path sanitizer (reject .., absolutes, state dir), sync exists/stat <!-- t:580w -->
- [x] [P2.2] files list/read/write/append/remove with mailboxes; probes for caps + denials <!-- t:ejvc -->
- [x] [P3.1] ModuleDone event kind + completion-mailbox plumbing in owner loop <!-- t:qqi3 -->
- [x] [P3.2] wifi module: lazy esp_wifi init, scan mailbox (16 APs), status block, nvs_flash init <!-- t:yub5 -->
- [x] [P3.3] S3WF credentials file in s3paper_storage (8 records, CRC, atomic) + fault-injection kind 5 <!-- t:xz4v -->
- [x] [P3.4] join/joinSaved/forget flows with timeout+retry; net console command; hardware gate: IP acquired, survives reboot <!-- t:ay8p -->
- [ ] [P4.1] http fetch builder: single slot, 4 headers, limit, worker task, PSRAM body mailbox <!-- t:8bla -->
- [ ] [P4.2] TLS cert bundle; probes: http+https fetch, truncation, timeout <!-- t:2lor -->
- [ ] [P5.1] serve module: route table (8 exact paths), request slot, semaphore handoff with dual timeouts <!-- t:43jw -->
- [ ] [P5.2] Static mount /sdcard/www (httpd-task streaming) + default device-status site <!-- t:6v92 -->
- [ ] [P5.3] Gate: curl hits JS route + static file; concurrent 503; owner-wedge 503 <!-- t:43b7 -->
- [ ] [P6.1] Settings app: scan list, join with keyboard, saved/forget, serve toggle + URL, margin toggle relocation <!-- t:fmuw -->
- [ ] [P6.2] Launcher wifi status glyph (dynamic text); Radio demo app (fetch feed to shelf) <!-- t:oh6o -->
- [ ] [P7.1] Hardening: module fault probes, 30-min serve soak under curl, sleep sequence with radio-down step 0 <!-- t:s8ra -->
- [ ] [P7.2] Docs: diary complete, changelog, doctor clean, host suite green <!-- t:4gai -->
