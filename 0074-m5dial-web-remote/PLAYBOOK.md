# 0074 M5Dial Web Remote Playbook

This playbook is for running the full stack during development:

- Go server on `:18080`
- Vite dev server
- ESP-IDF build / flash / monitor on `/dev/ttyACM0`

The commands below deliberately start `tmux` windows as normal shells first, then send commands into those shells. That matters because stopping `go run`, `npm run dev`, or `idf.py monitor` should return you to a shell prompt instead of closing the `tmux` window.

## Assumptions

- Repo root:

```bash
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
```

- Project root:

```bash
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote
```

- Firmware device:

```bash
/dev/ttyACM0
```

- Go server listen address:

```bash
:18080
```

- Firmware remote URL:

```bash
ws://<host>:18080/ws/device
```

## Recommended tmux Layout

Use one session with dedicated windows:

- `server`
- `vite`
- `firmware`
- `monitor`
- optional `scratch`

## Start a Fresh tmux Session

Create the session and windows:

```bash
tmux new-session -d -s m5dial0074
tmux rename-window -t m5dial0074:0 server
tmux new-window -t m5dial0074 -n vite
tmux new-window -t m5dial0074 -n firmware
tmux new-window -t m5dial0074 -n monitor
tmux new-window -t m5dial0074 -n scratch
tmux attach -t m5dial0074
```

This gives you plain interactive shells in every window.

## Start the Go Server

From outside tmux:

```bash
tmux send-keys -t m5dial0074:server 'cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server' C-m
tmux send-keys -t m5dial0074:server 'go run . -listen :18080' C-m
```

What to expect:

- HTTP server on `http://127.0.0.1:18080`
- browser websocket on `ws://127.0.0.1:18080/ws/browser`
- device websocket on `ws://127.0.0.1:18080/ws/device`

Stop it without closing the tmux window:

```text
Ctrl-C
```

Restart it:

```bash
tmux send-keys -t m5dial0074:server C-c
tmux send-keys -t m5dial0074:server 'go run . -listen :18080' C-m
```

## Start the Vite Dev Server

From outside tmux:

```bash
tmux send-keys -t m5dial0074:vite 'cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web' C-m
tmux send-keys -t m5dial0074:vite 'npm run dev' C-m
```

Notes:

- Vite proxies `/api` and `/ws` to `http://127.0.0.1:18080` by default.
- Override with `VITE_BACKEND_ORIGIN` only if the Go server is elsewhere.

Stop it without closing the tmux window:

```text
Ctrl-C
```

Restart it:

```bash
tmux send-keys -t m5dial0074:vite C-c
tmux send-keys -t m5dial0074:vite 'npm run dev' C-m
```

## Build Firmware

Use the `firmware` window for all ESP-IDF commands:

```bash
tmux send-keys -t m5dial0074:firmware 'cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware' C-m
tmux send-keys -t m5dial0074:firmware '. $HOME/esp/esp-idf-5.4.1/export.sh' C-m
tmux send-keys -t m5dial0074:firmware 'idf.py build' C-m
```

Re-run a build:

```bash
tmux send-keys -t m5dial0074:firmware 'idf.py build' C-m
```

## Flash Firmware to `/dev/ttyACM0`

From the `firmware` window:

```bash
tmux send-keys -t m5dial0074:firmware 'idf.py -p /dev/ttyACM0 flash' C-m
```

If you want a build and flash in one step:

```bash
tmux send-keys -t m5dial0074:firmware 'idf.py -p /dev/ttyACM0 build flash' C-m
```

Stop a flash attempt:

```text
Ctrl-C
```

## Run `idf.py monitor`

Use the dedicated `monitor` window:

```bash
tmux send-keys -t m5dial0074:monitor 'cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware' C-m
tmux send-keys -t m5dial0074:monitor '. $HOME/esp/esp-idf-5.4.1/export.sh' C-m
tmux send-keys -t m5dial0074:monitor 'idf.py -p /dev/ttyACM0 monitor' C-m
```

Stop the monitor and stay in the shell:

```text
Ctrl-]
```

If you used plain serial monitor mode and need to interrupt it:

```text
Ctrl-C
```

Restart the monitor:

```bash
tmux send-keys -t m5dial0074:monitor 'idf.py -p /dev/ttyACM0 monitor' C-m
```

## Common Firmware Console Commands

Once the device is up and the monitor is attached:

```text
help
wifi scan
wifi join <index> <password> --save
remote set-url ws://192.168.0.39:18080/ws/device
remote connect
remote status
```

## Fast Restart Sequences

### Restart only the server

```bash
tmux send-keys -t m5dial0074:server C-c
tmux send-keys -t m5dial0074:server 'go run . -listen :18080' C-m
```

### Restart only Vite

```bash
tmux send-keys -t m5dial0074:vite C-c
tmux send-keys -t m5dial0074:vite 'npm run dev' C-m
```

### Rebuild and flash firmware

```bash
tmux send-keys -t m5dial0074:firmware 'idf.py build' C-m
tmux send-keys -t m5dial0074:firmware 'idf.py -p /dev/ttyACM0 flash' C-m
```

### Re-attach monitor after flashing

```bash
tmux send-keys -t m5dial0074:monitor 'idf.py -p /dev/ttyACM0 monitor' C-m
```

## Rebuild Embedded Web Assets for the Go Server

Use this when you want the Go server to serve the current built React app instead of the Vite dev server:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web
npm run build
mkdir -p ../server/static
cp -R dist/. ../server/static/
```

Then restart the Go server:

```bash
tmux send-keys -t m5dial0074:server C-c
tmux send-keys -t m5dial0074:server 'go run . -listen :18080' C-m
```

## How to Check Whether the Board Is Present

Before flashing or attaching a monitor:

```bash
ls -l /dev/ttyACM0
```

Also useful:

```bash
ls /dev/ttyACM*
ls /dev/serial/by-id/
lsusb | rg Espressif
```

## If `/dev/ttyACM0` Disappears

Symptoms:

- `idf.py -p /dev/ttyACM0 flash` cannot open the port
- `idf.py monitor` exits because the device vanished
- `/dev/ttyACM0` is missing after a reset or cable movement

Checklist:

1. Confirm the port is actually gone:

```bash
ls -l /dev/ttyACM0
```

2. Re-seat the cable or power-cycle the device.
3. Check whether it re-enumerated under a different device:

```bash
ls /dev/ttyACM*
ls /dev/ttyUSB*
ls /dev/serial/by-id/
```

4. Check USB enumeration:

```bash
lsusb | rg Espressif
```

5. Retry the flash or monitor command once the port is back.

## Useful tmux Commands

Attach:

```bash
tmux attach -t m5dial0074
```

List windows:

```bash
tmux list-windows -t m5dial0074
```

Capture the last lines of the server window:

```bash
tmux capture-pane -pt m5dial0074:server | tail -n 40
```

Capture the last lines of the monitor window:

```bash
tmux capture-pane -pt m5dial0074:monitor | tail -n 80
```

Kill the whole session:

```bash
tmux kill-session -t m5dial0074
```

## Suggested Daily Workflow

1. Start `tmux` session and windows.
2. Start Go server in `server`.
3. Start Vite in `vite`.
4. Build and flash in `firmware`.
5. Attach `idf.py monitor` in `monitor`.
6. Use `remote status` on the device console to confirm it is connected to `ws://HOST:18080/ws/device`.
7. Open the Vite app in a browser and test commands.
8. When changing firmware, rebuild and flash again.
9. When changing the Go server or Vite app, restart only that window.
