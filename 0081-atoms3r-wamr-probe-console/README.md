# 0081 AtomS3R WAMR Probe Console

This project is a control-board probe for the WAMR investigation. It combines:

- known-good AtomS3R display and backlight bring-up from `0013`
- the current minimal WAMR runtime and embedded module workflow from `0079`

The purpose is not to recreate the full PaperS3 demo. The purpose is to answer whether
the remaining failures survive on a second ESP32-S3 board with a non-EPD display path.

Initial command surface:

- `wasm status`
- `wasm list`
- `wasm replay hello-frame`
- `wasm run-preflush return-42`
- `wasm run-preflush log-only`
- `wasm run-preflush hello-frame`
