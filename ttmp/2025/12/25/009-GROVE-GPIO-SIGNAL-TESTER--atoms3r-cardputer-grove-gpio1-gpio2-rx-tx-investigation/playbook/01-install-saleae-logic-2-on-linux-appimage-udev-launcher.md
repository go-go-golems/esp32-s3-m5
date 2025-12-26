---
Title: Install Saleae Logic 2 on Linux (AppImage + udev + launcher)
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - serial
    - usb-serial-jtag
    - debugging
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../../../etc/udev/rules.d/99-SaleaeLogic.rules
      Note: udev rules for non-root USB device access
    - Path: ../../../../../../../../../../.local/apps/Logic-2.4.40-linux-x64.AppImage
      Note: Saleae Logic 2 AppImage used in this playbook
    - Path: ../../../../../../../../../../.local/share/applications/saleae-logic2.desktop
      Note: Desktop launcher entry created for app launcher integration
    - Path: ../../../../../../../../../../.local/share/icons/hicolor/256x256/apps/saleae-logic2.png
      Note: Optional launcher icon extracted from AppImage
ExternalSources:
    - https://support.saleae.com/logic-software/logic-2:Saleae Logic 2 official docs + downloads
    - https://crbug.com/638180:Chromium/Electron note about running as root and sandboxing
Summary: Install and run Saleae Logic 2 on Linux from an AppImage, including udev permissions and desktop launcher integration.
LastUpdated: 2025-12-24T19:35:19.135626983-05:00
WhatFor: Make Saleae Logic 2 usable as a normal user (no root), show up in the desktop app launcher, and reliably connect to Saleae USB devices.
WhenToUse: When setting up Saleae Logic 2 on Linux via AppImage (especially on Ubuntu systems where Electron sandboxing may fail due to AppArmor user-namespace restrictions).
---


# Install Saleae Logic 2 on Linux (AppImage + udev + launcher)

## Purpose

This playbook installs Saleae Logic 2 (Logic Analyzer software) on Linux using the official AppImage, makes it show up in your desktop app launcher, and configures udev so you can access the Saleae device **without running Logic as root**.

It also documents the most common “it crashes on launch” failure mode on Ubuntu: AppArmor restricting unprivileged user namespaces, which breaks Electron’s sandbox and causes a zygote crash unless you adjust your system policy or run with `--no-sandbox`.

## Environment Assumptions

This playbook assumes:

- You are running a modern Linux desktop (GNOME/KDE/etc).
- You can run `sudo` (for installing udev rules).
- You have a writable per-user apps directory (example: `~/.local/apps/`).
- You want Logic 2 to be startable via your app launcher (via a `.desktop` entry).

Notes about Ubuntu:

- On some Ubuntu versions, `kernel.apparmor_restrict_unprivileged_userns=1` blocks the Electron sandbox used by Logic 2 AppImage, causing a crash at startup unless you use a workaround.

## Commands

This section is written as an executable “do this, then verify” set of steps. Run the commands top-to-bottom and stop when you hit a troubleshooting section that matches your symptoms.

### 1) Download the AppImage and make it executable

Download Logic 2 from Saleae, then place it somewhere stable (example: `~/.local/apps/`).

Official docs + downloads:
- [Saleae Logic 2 documentation](https://support.saleae.com/logic-software/logic-2)

Example (adjust filename/version to match your download):

```bash
APP="$HOME/.local/apps/Logic-2.4.40-linux-x64.AppImage" \
&& chmod +x "$APP"
```

### 2) First run (and what to do if it crashes)

Try launching it as your normal user:

```bash
APP="$HOME/.local/apps/Logic-2.4.40-linux-x64.AppImage" \
&& "$APP"
```

If it **launches successfully**, skip to the udev rules section.

If it **crashes with messages like**:
- “The setuid sandbox is not running as root”
- “Failed to move to new namespace: … Operation not permitted”
- `zygote_host_impl_linux.cc` fatal

…then you are likely hitting an Ubuntu/AppArmor restriction on unprivileged user namespaces.

#### Workaround A (pragmatic): run with `--no-sandbox`

This is the quickest way to get moving:

```bash
APP="$HOME/.local/apps/Logic-2.4.40-linux-x64.AppImage" \
&& "$APP" --no-sandbox
```

Security note: `--no-sandbox` disables Electron’s sandboxing. It’s a common workaround, but you should treat it as a trade-off (especially if you open untrusted captures/files).

#### Workaround B (system policy): allow user namespaces for the sandbox

If you want to keep the Electron sandbox, you’ll need to adjust the host policy that’s blocking it. On Ubuntu, a key indicator is:

```bash
sysctl kernel.apparmor_restrict_unprivileged_userns
```

If it prints `1`, that restriction is enabled.

This playbook doesn’t force a specific policy change (because it’s distro/security-model dependent), but it’s useful context for debugging and for deciding whether you want to prefer a “native package” install method over an AppImage.

### 3) Add Logic 2 to your desktop app launcher

Desktop launchers are `.desktop` files under `~/.local/share/applications/`. This makes Logic discoverable via your app launcher search.

Create a launcher entry (using the `--no-sandbox` workaround in the `Exec=` line):

```bash
APP="$HOME/.local/apps/Logic-2.4.40-linux-x64.AppImage" \
&& DESK="$HOME/.local/share/applications/saleae-logic2.desktop" \
&& APP_ABS="$(readlink -f "$APP")" \
&& cat > "$DESK" <<EOF
[Desktop Entry]
Type=Application
Name=Saleae Logic 2
Comment=Logic Analyzer software (AppImage)
Exec=$APP_ABS --no-sandbox %U
Terminal=false
Categories=Development;Electronics;
StartupNotify=true
Icon=saleae-logic2
EOF
```

This “absolute path only” approach avoids the common gotcha that many desktop environments do not expand `~` or `$HOME` inside `.desktop` files.

#### Optional: install an icon for a nicer launcher tile

Logic’s AppImage includes PNG icons. You can extract them and copy one into your local icon theme directory (only if it already exists).

```bash
APP="$HOME/.local/apps/Logic-2.4.40-linux-x64.AppImage" \
&& rm -rf /tmp/squashfs-root \
&& cd /tmp \
&& "$APP" --appimage-extract \
&& ICON_SRC="/tmp/squashfs-root/usr/share/icons/hicolor/256x256/apps/Logic.png" \
&& ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps" \
&& test -f "$ICON_SRC" \
&& test -d "$ICON_DIR" \
&& cp -f "$ICON_SRC" "$ICON_DIR/saleae-logic2.png"
```

If your system doesn’t have `~/.local/share/icons/hicolor/256x256/apps` already, either create it manually (if you want), or skip the icon step and let the launcher fall back to a generic icon.

### 4) Install udev rules (required for USB device access without root)

Saleae ships udev rules that set appropriate permissions for the USB device node. Without this, Logic often can’t open the device unless you run it as root (which Electron explicitly discourages).

Install the rules file into `/etc/udev/rules.d/`:

```bash
RULE_SRC=$(find /tmp/squashfs-root -path '*/resources/linux-x64/99-SaleaeLogic.rules' -print -quit) \
&& cat "$RULE_SRC" | sudo tee /etc/udev/rules.d/99-SaleaeLogic.rules > /dev/null \
&& echo "finished installing /etc/udev/rules.d/99-SaleaeLogic.rules" \
&& sudo udevadm control --reload-rules \
&& sudo udevadm trigger
```

If `/tmp/squashfs-root` doesn’t exist anymore, re-extract the AppImage first:

```bash
APP="$HOME/.local/apps/Logic-2.4.40-linux-x64.AppImage" \
&& cd /tmp \
&& "$APP" --appimage-extract
```

### 5) Make the new permissions take effect (replug is usually best)

udev rules are applied when the device node is created. The most reliable workflow is:

- Unplug the Saleae device
- Plug it back in

Logging out is typically not required.

### 6) Quick verification

Check that the device is visible:

```bash
lsusb | grep -i saleae || true
```

If you want to be extra sure udev permissions are applied, inspect the USB node permissions (replace `BBB` and `DDD` with your bus/device numbers from `lsusb`):

```bash
ls -l /dev/bus/usb/BBB/DDD
```

If you see `crw-rw-rw-` (or otherwise permissive read/write bits), the udev rules are doing their job.

## Exit Criteria

You’re done when all of the following are true:

- Logic 2 starts normally (either sandboxed, or via `--no-sandbox` workaround).
- Logic 2 shows up in your app launcher (search for “Saleae Logic 2”).
- Plugging in a Saleae device makes it usable inside Logic **without** running Logic as root.

## Notes

### Running as root is not the right fix

If you try `sudo ./Logic-*.AppImage`, Electron typically exits with:

- “Running as root without --no-sandbox is not supported”

This is a Chromium/Electron constraint (see [crbug 638180](https://crbug.com/638180)).

### About the udev rules (security trade-off)

The shipped rules typically use `MODE="0666"`, which means any local user can access the device node. It’s convenient and common for dev tools, but it’s permissive by design. If you need a stricter setup, you can adapt the rules to use a dedicated group and `MODE="0660"`.
