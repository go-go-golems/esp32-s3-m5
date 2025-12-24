# Changelog

## 2025-12-22

- Initial workspace created


## 2025-12-22

Scope pivot: ticket 003 is now a Cardputer on-screen typewriter (keyboard → display) using M5GFX; added diary and rewrote design doc.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/design-doc/01-cardputer-keyboard-serial-echo-esp-idf.md — Typewriter design
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md — Implementation diary


## 2025-12-22

Improved Cardputer keyboard bring-up UX: clarify inline echo, add per-key log option, add IN0/IN1 autodetect + settle delay for scan (code commit c0b5124).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md — Diary step
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild — New debug/autodetect Kconfig options
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c — Keyboard scan/echo improvements


## 2025-12-22

Note: some host serial monitors are line-buffered; inline echo may only become visible when Enter emits a newline. Documented workaround (per-key log option).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md — Explain host-side line buffering behavior


## 2025-12-22

Closing ticket: scope pivoted to Cardputer typewriter; keyboard bring-up notes captured; continuing work in new ticket.

