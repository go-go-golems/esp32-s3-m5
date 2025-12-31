---
Title: 'Linux: Scan, Pair, Trust, and Connect a Bluetooth Keyboard (BlueZ/bluetoothctl)'
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - cardputer
    - sensors
    - esp32s3
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-30T18:37:50.686657257-05:00
WhatFor: ""
WhenToUse: ""
---

# Linux: Scan, Pair, Trust, and Connect a Bluetooth Keyboard (BlueZ/bluetoothctl)

## Purpose

Provide a **repeatable** procedure to:

- bring up the Linux Bluetooth stack,
- **scan** for devices,
- **pair + trust** a Bluetooth keyboard (including legacy keyboards that require **typing a PIN on the keyboard**),
- **connect** it,
- and verify (via `bluetoothctl` + logs) that it’s actually connected.

## Environment Assumptions

- **Linux + BlueZ** installed (`bluetoothctl` available).
- `bluetooth.service` running (`bluetoothd`).
- You have physical access to the keyboard to put it into pairing mode and (if prompted) **type a PIN/passkey**.

Tested here with:
- `bluetoothctl` 5.72
- BlueZ via `systemctl status bluetooth`

Important host gotcha we hit:
- Adapter was **rfkill soft-blocked** and `bluetoothctl show` reported **Powered: no**, which causes `scan on` to fail with `org.bluez.Error.NotReady`.

## Commands

```bash
# 0) Quick health check (run first)
rfkill list && systemctl status bluetooth --no-pager && bluetoothctl --version && bluetoothctl show

# If you see:
# - "Soft blocked: yes" for hci0
# - or "Powered: no" in bluetoothctl show
# then do:
rfkill unblock bluetooth && bluetoothctl power on && bluetoothctl show

# 1) Scan for devices (run while keyboard is in pairing mode)
# NOTE: many keyboards only advertise briefly; re-run if needed.
bluetoothctl --timeout 40 scan on || true
bluetoothctl devices

# 2) Pick the keyboard MAC from the output above, e.g.:
#   DC:2C:26:FD:03:B7  ZAGG Slim Book
MAC="DC:2C:26:FD:03:B7"

# 3) Pair using INTERACTIVE bluetoothctl (required for PIN prompts)
# 
# IMPORTANT: For keyboards that require PIN entry, you MUST use interactive mode.
# The --agent flag doesn't work reliably in non-interactive mode.
#
# Run this interactively:
bluetoothctl
# Then inside bluetoothctl:
#   [bluetooth]# agent KeyboardDisplay
#   [bluetooth]# default-agent
#   [bluetooth]# scan on
#   (wait for keyboard to appear, note MAC)
#   [bluetooth]# pair DC:2C:26:FD:03:B7
#   (bluetoothctl will show a PIN/passkey - TYPE IT ON THE KEYBOARD and press Enter)
#   [bluetooth]# trust DC:2C:26:FD:03:B7
#   [bluetooth]# connect DC:2C:26:FD:03:B7
#   [bluetooth]# info DC:2C:26:FD:03:B7
#   (verify Bonded: yes appears)
#   [bluetooth]# quit
#
# Alternative (non-interactive, but may not show PIN prompt):
# bluetoothctl --agent KeyboardDisplay --timeout 180 pair "$MAC" || true

# 4) Trust the device so it can reconnect automatically
bluetoothctl trust "$MAC"

# 5) Connect (some keyboards connect only after pairing succeeds)
bluetoothctl connect "$MAC" || true

# 6) Inspect state
bluetoothctl info "$MAC"
```

### Expected outputs / success indicators

- After pairing, `bluetoothctl info $MAC` should show:
  - `Paired: yes`
  - **`Bonded: yes`** ← **This is critical!** If `Bonded: no`, keys weren't persisted and HID may be blocked.
  - `Trusted: yes` (after `trust`)
  - `Connected: yes` (after `connect`)

**How to verify bonding succeeded:**

```bash
# Check if bonding directory exists (keys persisted)
ls -la /var/lib/bluetooth/$(bluetoothctl list | grep -o '[0-9A-F:]\{17\}')/DC:2C:26:FD:03:B7/

# Should show files like:
# - info
# - attributes
# - (and possibly link keys)
```

If the directory doesn't exist, bonding didn't complete and you need to re-pair with proper PIN entry.

### If pairing requires a PIN/passkey (common for keyboards)

**Critical for bonding**: Many keyboards require PIN entry to complete bonding (not just pairing).

Typical experience:
- `bluetoothctl` prints something like:
  - `[agent] Request PIN code`
  - or `[agent] Passkey: 123456`
- You must **type the digits on the Bluetooth keyboard** and press **Enter**.
- **This step is required for bonding** - if you skip it or it times out, you'll get `Paired: yes` but `Bonded: no`.

**Why bonding matters:**
- `Paired: yes` = keys exchanged during pairing session
- `Bonded: yes` = keys persisted to `/var/lib/bluetooth/.../` (device can reconnect automatically)
- BlueZ's `ClassicBondedOnly=true` (default) blocks HID from unbonded devices

**If PIN prompt doesn't appear:**
- Make sure you're in **interactive** `bluetoothctl` (not using `--agent` flag)
- Run `agent KeyboardDisplay` and `default-agent` inside bluetoothctl before pairing
- Some keyboards may not prompt for PIN (bonding may complete automatically)

If you don't type the PIN quickly, the keyboard often exits pairing mode and the attempt fails.

### If scan/pair fails with NotReady

This usually means the adapter is not powered or rfkill-blocked.

Run:

```bash
rfkill list && bluetoothctl show
rfkill unblock bluetooth && bluetoothctl power on
```

### If the device “disappears” / `Device ... not available`

BlueZ can evict temporary devices after a timeout (see `TemporaryTimeout` in `/etc/bluetooth/main.conf`, default 30s).

Practical workaround:
- keep scanning running while you pair, or re-scan and immediately run `pair` again.

```bash
bluetoothctl --timeout 20 scan on || true
bluetoothctl devices | grep -i keyboard -n || true
```

### Debugging: capture what BlueZ thinks is happening

Run these in separate terminals while pairing/connecting:

```bash
# BlueZ daemon logs (look for pairing / HID / bonding messages)
journalctl -u bluetooth -f

# Low-level HCI tracing (super useful when connect/pair fails)
sudo btmon
```

### Common failure mode: HID profile rejects unbonded devices

We saw a BlueZ log like:
- `hidp_add_connection() Rejected connection from !bonded device ...`

That means: **the keyboard tried to connect as a HID device, but BlueZ considered it not bonded**.

**Root cause**: Pairing completed (`Paired: yes`) but bonding didn't (`Bonded: no`), usually because:
- PIN/passkey wasn't entered on the keyboard during pairing
- Agent wasn't properly registered to handle PIN prompts
- Keys weren't persisted to `/var/lib/bluetooth/.../`

**Fix options:**

1. **Complete bonding properly** (recommended):
   ```bash
   bluetoothctl remove DC:2C:26:FD:03:B7
   bluetoothctl
   # Inside bluetoothctl:
   agent KeyboardDisplay
   default-agent
   scan on
   # (wait for keyboard, then)
   pair DC:2C:26:FD:03:B7
   # (when PIN appears, TYPE IT ON THE KEYBOARD and press Enter)
   trust DC:2C:26:FD:03:B7
   connect DC:2C:26:FD:03:B7
   # Verify: info DC:2C:26:FD:03:B7 shows Bonded: yes
   ```

2. **Disable ClassicBondedOnly** (workaround if bonding can't be completed):
   ```bash
   sudo sed -i 's/^#ClassicBondedOnly=true/ClassicBondedOnly=false/' /etc/bluetooth/input.conf
   sudo systemctl restart bluetooth
   bluetoothctl disconnect DC:2C:26:FD:03:B7 && bluetoothctl connect DC:2C:26:FD:03:B7
   ```

### "Connected: yes" but keyboard doesn't work / no input device appears

**Symptom**: `bluetoothctl info $MAC` shows `Connected: yes` and `Paired: yes`, but:
- typing on the keyboard does nothing,
- `/proc/bus/input/devices` doesn't list the keyboard,
- `journalctl -u bluetooth` shows `Rejected connection from !bonded device`.

**Root cause**: BlueZ's `ClassicBondedOnly` setting (in `/etc/bluetooth/input.conf`) defaults to `true`, which blocks HID connections from devices that are paired but not fully bonded.

**Fix**:

```bash
# 1) Edit BlueZ input profile config
sudo nano /etc/bluetooth/input.conf

# 2) Find the line (usually commented out):
#   #ClassicBondedOnly=true
#   Change it to:
#   ClassicBondedOnly=false

# 3) Restart bluetoothd
sudo systemctl restart bluetooth

# 4) Reconnect the keyboard
bluetoothctl disconnect "$MAC"
bluetoothctl connect "$MAC"

# 5) Verify HID connection succeeded
journalctl -u bluetooth -n 20 | grep -i 'hidp\|input'
# Should NOT see "Rejected connection from !bonded device"

# 6) Check if input device appeared
grep -i 'bluetooth\|keyboard\|zagg' /proc/bus/input/devices
# Should now show your keyboard

# 7) Test typing (should work now)
```

**Alternative**: If you can't modify `/etc/bluetooth/input.conf`, try forcing bonding by:
- removing the device,
- re-pairing with proper PIN entry on the keyboard,
- ensuring `Bonded: yes` appears in `bluetoothctl info`.

## Exit Criteria

- `bluetoothctl info $MAC` shows `Paired: yes` and `Connected: yes`.
- **Keystrokes work in a text field** (practical verification — this is the real test).
- Optional: `journalctl -u bluetooth -f` shows HID connection without `!bonded` rejection.
- Optional: `/proc/bus/input/devices` lists the keyboard (confirms kernel input layer sees it).

## Understanding Pairing vs Bonding

**Pairing** (`Paired: yes`):
- Devices exchange **link keys** (encryption keys) during the pairing session
- Keys are stored **in memory** for the current session
- Allows connection and encrypted communication
- **Does NOT persist** across reboots or reconnections

**Bonding** (`Bonded: yes`):
- Link keys are **persisted to disk** (`/var/lib/bluetooth/<adapter>/<device>/`)
- Device can reconnect **automatically** without re-pairing
- Keys survive reboots
- **Required for HID** if `ClassicBondedOnly=true` (default)

**Why some keyboards show `Bonded: no`:**

1. **Legacy pairing without PIN**: Some keyboards use "Just Works" pairing (no PIN required), but BlueZ may not persist keys automatically
2. **No PIN prompt**: If the keyboard doesn't require PIN entry, bonding may not complete automatically
3. **Keys not persisted**: Even if pairing succeeds, keys might not be written to `/var/lib/bluetooth/.../`

**How to check if bonding completed:**

```bash
# Check bonding directory exists (keys persisted)
ls -la /var/lib/bluetooth/$(bluetoothctl list | grep -o '[0-9A-F:]\{17\}')/DC:2C:26:FD:03:B7/

# If directory doesn't exist, bonding didn't complete
# If directory exists with files (info, attributes, etc.), bonding succeeded
```

**Practical solution when bonding doesn't complete:**

If your keyboard pairs successfully but never achieves `Bonded: yes` (common with legacy keyboards), you have two options:

1. **Disable ClassicBondedOnly** (allows HID from paired but unbonded devices):
   ```bash
   sudo sed -i 's/^#ClassicBondedOnly=true/ClassicBondedOnly=false/' /etc/bluetooth/input.conf
   sudo systemctl restart bluetooth
   bluetoothctl disconnect DC:2C:26:FD:03:B7 && bluetoothctl connect DC:2C:26:FD:03:B7
   ```

2. **Accept that re-pairing may be needed** after reboots (if ClassicBondedOnly stays enabled)

## Notes

- **Non-interactive `bluetoothctl`** (heredoc/scripts) can fail to register an agent; prefer:
  - interactive `bluetoothctl`, or
  - `bluetoothctl --agent KeyboardDisplay ...` as shown above.
- If the keyboard frequently leaves pairing mode, keep it close to the host and consider fresh batteries/charge.
- **Some keyboards never achieve `Bonded: yes`** even with successful pairing - this is normal for legacy devices, and disabling `ClassicBondedOnly` is the practical workaround.

## GUI options (often easier for keyboards)

If you want a UI that handles PIN prompts more smoothly:

- **Blueman** (`blueman-manager`): lightweight GTK manager, good pairing UX.
- **GNOME Settings → Bluetooth** (if you’re on GNOME).
- **KDE Bluedevil** (if you’re on KDE).

On Ubuntu/Debian, Blueman is typically:

```bash
sudo apt update && sudo apt install -y blueman
```
