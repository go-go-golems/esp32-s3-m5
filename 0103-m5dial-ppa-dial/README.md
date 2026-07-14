# 0103 — M5Dial PPA Dial (Four Audio PPA scene controller)

ESP-IDF firmware for the M5Stack Dial that switches scenes on Four Audio PPA
amplifier modules over UDP (port 5001). Design doc + protocol reference:
`ttmp/2026/07/14/M5DIAL-PPA-CONTROL--sick-m5dial-firmware-to-control-four-audio-ppa-modules/`.

## Build

Uses **ESP-IDF 5.4.1** (repo default for M5Dial projects).

```bash
cd 0103-m5dial-ppa-dial
source ~/esp/esp-idf-5.4.1/export.sh
idf.py set-target esp32s3     # first time only
idf.py build
```

## Flash + monitor

The M5Dial enumerates as an Espressif USB JTAG/serial device (`303a:1001`),
usually `/dev/ttyACM0`.

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

- Monitor keys: `Ctrl-]` quits, `Ctrl-T Ctrl-R` resets the board and reloads.
- If esptool can't connect: hold the button on the underside of the dial while
  plugging in (download mode), flash, then replug normally.
- Serial log tags: `ppa_dial`, `wifi_mgr`, `web_setup`, `ppa_client`.

Running it inside tmux works fine, e.g.:

```bash
tmux new-session -s ppadial "source ~/esp/esp-idf-5.4.1/export.sh && idf.py -p /dev/ttyACM0 flash monitor"
```

## First boot / provisioning

1. Display shows "PPA Dial / verbinde mit WLAN …" with a spinner.
2. With no WiFi configured (or after retries fail), it falls back to the red
   **Setup** screen: hotspot **PPA-Dial** / password **ppadial123**.
3. Join that hotspot and open **http://192.168.4.1**.
4. Enter WLAN SSID + password and paste the full contents of the Mac app's
   presets file (`~/Library/Application Support/PPA Group Control/presets.json`).
   Invalid JSON is rejected with an inline error and nothing is saved.
5. Save → the dial reboots, joins your WLAN, and is reachable at
   **http://ppadial.local** for later preset updates.

## Operation

- **Rotate** = select scene (dots on the top rim show the position).
- **Press** (encoder button or touch tap) = switch the scene; a progress arc
  fills per confirmed module, then shows OK (green) or Fehler (red).
- Bottom rim arc + `n/m online` = how many modules of the selected scene are
  currently discovered.

## Testing without amplifiers: ppa_sim.py

`tools/ppa_sim.py` simulates a PPA module on the host (must be on the same LAN
as the dial):

```bash
python3 tools/ppa_sim.py --uid 0x1234ABCD                  # acks everything
python3 tools/ppa_sim.py --uid 0x1234ABCD --behavior busy2 # busy twice, then ok
python3 tools/ppa_sim.py --uid 0x1234ABCD --behavior error # hard error
python3 tools/ppa_sim.py --uid 0x1234ABCD --behavior silent # never answers recalls
```

Then use a presets.json whose scene entries reference the simulator, keyed by
`uid_1234ABCD` (discovered via broadcast ping) or `ip_<host-ip>` (fixed IP):

```json
{
  "scenes": {
    "Sprache": { "uid_1234ABCD": { "id": 2, "sub": 0, "name": "Speech" } },
    "Musik":   { "uid_1234ABCD": { "id": 5, "sub": 1, "name": "Music" } }
  }
}
```

The simulator logs every received packet hex-decoded, so it doubles as a
protocol tracer.
