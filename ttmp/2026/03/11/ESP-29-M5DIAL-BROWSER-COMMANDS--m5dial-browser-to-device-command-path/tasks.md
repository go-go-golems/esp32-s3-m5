# Tasks

## TODO

- [x] Create ticket `ESP-29-M5DIAL-BROWSER-COMMANDS`
- [x] Write the implementation plan for browser-to-device commands
- [x] Add a firmware-side command queue and wire it into `app_task`
- [x] Parse inbound websocket command frames on the ESP32
- [x] Add command handling on the ESP32 for `show_message`
- [x] Add command handling on the ESP32 for `set_position`
- [x] Track active device websocket connections in the Go hub
- [x] Accept `ui_command` frames from browser websocket clients and route them to the target device
- [x] Add React UI controls to send commands to the selected device
- [x] Add request-correlated `ui_command_ack` messages from firmware back to the browser
- [x] Validate the round trip with logs and live hardware
