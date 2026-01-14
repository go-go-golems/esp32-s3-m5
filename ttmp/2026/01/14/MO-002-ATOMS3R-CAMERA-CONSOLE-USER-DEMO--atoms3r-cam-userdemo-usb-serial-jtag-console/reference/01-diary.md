---
Title: Diary
Ticket: MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO
Status: active
Topics:
    - camera
    - esp32
    - firmware
    - usb
    - console
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/CMakeLists.txt
      Note: Exclude TinyUSB/UVC components when UVC is disabled.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/components/usb_device_uvc/CMakeLists.txt
      Note: Skip UVC component registration when UVC is disabled.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/dependencies.lock
      Note: |-
        Managed component versions updated during build.
        Resolved esp_insights 1.3.1 and updated managed component versions.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/idf_component.yml
      Note: Pin esp_insights version in project manifest.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/CMakeLists.txt
      Note: Make gen_single_bin include optional to avoid build failure.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp
      Note: Guard UVC service compilation behind CONFIG_USB_WEBCAM_ENABLE_UVC.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: Remove unused UVC headers to avoid component dependency.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/managed_components/espressif__esp_insights/src/esp_insights_cbor_encoder.c
      Note: Build failure due to SHA_SIZE undefined.
    - Path: 0041-atoms3r-cam-jtag-serial-test/.envrc
      Note: Local IDF 5.1.4 environment for the minimal test app (ignored by git).
    - Path: 0041-atoms3r-cam-jtag-serial-test/CMakeLists.txt
      Note: Project entry point for the minimal JTAG serial test firmware.
    - Path: 0041-atoms3r-cam-jtag-serial-test/README.md
      Note: Build/flash steps for the minimal test app.
    - Path: 0041-atoms3r-cam-jtag-serial-test/dependencies.lock
      Note: Component manager lockfile for the 0041 camera test.
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/CMakeLists.txt
      Note: Registers the minimal main component.
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/Kconfig.projbuild
      Note: Camera pin and power Kconfig options for the minimal probe app.
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/camera_pin.h
      Note: AtomS3R-CAM pin definitions used by the camera-only app.
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/main.c
      Note: USB Serial/JTAG tick loop used to validate console enumeration.
    - Path: 0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults
      Note: Forces USB Serial/JTAG console in the minimal test app.
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/sources/local/Research
      Note: esp_insights SHA_SIZE.md:Imported research on SHA_SIZE dependency drift.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-14T17:03:20.928121044-05:00
WhatFor: ""
WhenToUse: ""
---








# Diary

## Goal

Track the steps taken to isolate USB Serial/JTAG console enumeration in the AtomS3R-CAM UserDemo, focusing on disabling the UVC/TinyUSB stack when UVC is not in use.

## Step 1: Gate UVC component and code behind CONFIG_USB_WEBCAM_ENABLE_UVC

This step isolates the USB device UVC stack so it is not built or linked when UVC is disabled. The goal is to remove any TinyUSB usage that could be grabbing the USB peripheral and preventing USB Serial/JTAG enumeration.

I updated the UVC component build logic and the app code paths so a UVC-disabled build no longer includes UVC headers or TinyUSB descriptors. This keeps the USB Serial/JTAG path clean while still allowing UVC builds when explicitly enabled.

### What I did
- Made the `usb_device_uvc` component a no-op when `CONFIG_USB_WEBCAM_ENABLE_UVC=n`.
- Wrapped `main/service/service_uvc.cpp` in `#if CONFIG_USB_WEBCAM_ENABLE_UVC`.
- Removed unused UVC header includes from `main/utils/camera/camera_init.c`.

### Why
- TinyUSB/UVC can claim the USB peripheral even when the runtime UVC service is disabled, blocking USB Serial/JTAG enumeration.

### What worked
- UVC component registration and descriptor injection are now skipped when UVC is disabled.

### What didn't work
- N/A (no build run yet).

### What I learned
- It is not enough to disable the UVC service at runtime; the component must be excluded at build time to avoid USB conflicts.

### What was tricky to build
- Ensuring the UVC component is fully skipped without breaking include paths or service symbols.

### What warrants a second pair of eyes
- Confirm no other components pull in TinyUSB or use the UVC headers when `CONFIG_USB_WEBCAM_ENABLE_UVC=n`.

### What should be done in the future
- Rebuild and flash to confirm USB Serial/JTAG enumeration with UVC disabled.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/components/usb_device_uvc/CMakeLists.txt` for the conditional `return()`.
- Review `../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp` for the config guard.
- Review `../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c` for removed UVC includes.

### Technical details
- `CONFIG_USB_WEBCAM_ENABLE_UVC` is now a build-time gate for the UVC component.

## Step 2: Exclude TinyUSB when UVC is disabled and attempt a clean build

This step adds a CMake-level exclusion so the managed TinyUSB component is removed when UVC is disabled. The goal is to ensure the build system doesn’t pull in TinyUSB at all, since it can claim the USB peripheral even if the UVC service isn’t started.

I also made the `gen_single_bin` include optional to keep CMake configuration from failing when `espressif__cmake_utilities` gets removed. Then I attempted a full build using the ESP-IDF 5.1.4 toolchain and confirmed TinyUSB was deleted from managed components during dependency resolution, though the build later failed in `esp_insights`.

### What I did
- Added a `sdkconfig`/`sdkconfig.defaults` check in the top-level `CMakeLists.txt` and excluded `usb_device_uvc` + `espressif__tinyusb` when UVC is disabled.
- Made `include(gen_single_bin)` optional in `main/CMakeLists.txt`.
- Rebuilt using IDF 5.1.4 with the correct Python environment.

### Why
- We need to confirm that TinyUSB is not even built when UVC is disabled.

### What worked
- CMake reported `NOTICE: Deleting 2 unused components ... espressif__tinyusb`, confirming TinyUSB was removed from the build graph.

### What didn't work
- Build failed in `espressif__esp_insights`:
  ```
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/managed_components/espressif__esp_insights/src/esp_insights_cbor_encoder.c:199:22: error: 'SHA_SIZE' undeclared here (not in a function)
  ```

### What I learned
- Excluding TinyUSB via `EXCLUDE_COMPONENTS` works, but the build still has other managed-component issues unrelated to USB.

### What was tricky to build
- Ensuring the exclusion happens before `project()` while still using the correct IDF environment.

### What warrants a second pair of eyes
- Confirm the CMake `sdkconfig` parsing logic is robust enough for fresh builds.

### What should be done in the future
- Decide whether to disable or patch `esp_insights` to allow a full build.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/CMakeLists.txt` for the UVC/TinyUSB exclusion logic.
- Review `../ATOMS3R-CAM-UserDemo/main/CMakeLists.txt` for the optional `gen_single_bin` include.

### Technical details
- **Commands**
- `export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; source /home/manuel/esp/esp-idf-5.1.4/export.sh; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo build`
- **Build signal**
- `NOTICE: Deleting 2 unused components`
- `NOTICE:  espressif__cmake_utilities`
- `NOTICE:  espressif__tinyusb`

## Step 3: Trace esp_insights failure via git history and dependency lock

This step investigates why the build now fails in `esp_insights` after removing TinyUSB. I looked at the repository history and the dependency lock changes to see if the failure is tied to a recent commit or a component version shift.

The git history shows no commits that touch `esp_insights`. The new error appears after the component manager updated `dependencies.lock` during the build, which bumped several managed component versions and removed TinyUSB. The failure is therefore unrelated to TinyUSB and is more likely an incompatibility or bug in the current `esp_insights` component or its managed dependencies.

### What I did
- Reviewed the git log for recent commits in the UserDemo repo.
- Diffed `dependencies.lock` to see what changed when UVC/TinyUSB was excluded.
- Inspected the failing `esp_insights_cbor_encoder.c` and noted `SHA_SIZE` is undefined.

### Why
- To determine whether the failure is linked to TinyUSB removal or to a dependency update.

### What worked
- The lockfile diff shows multiple managed component version updates that can explain a new build failure.

### What didn't work
- The build still fails in `espressif__esp_insights`:
  ```
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/managed_components/espressif__esp_insights/src/esp_insights_cbor_encoder.c:199:22: error: 'SHA_SIZE' undeclared here (not in a function)
  ```

### What I learned
- The issue is not caused by TinyUSB; it appears after dependency resolution updates managed components.

### What was tricky to build
- The component manager updates the lockfile automatically, which can introduce new breakages unrelated to local changes.

### What warrants a second pair of eyes
- Confirm whether the project should pin or downgrade `esp_insights` or `esp_diag_data_store` to avoid the `SHA_SIZE` mismatch.

### What should be done in the future
- Decide whether to exclude `esp_insights` for console-only builds or patch it to use `RTC_STORE_SHA_SIZE`.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/dependencies.lock` for managed component version shifts.
- Review `../ATOMS3R-CAM-UserDemo/managed_components/espressif__esp_insights/src/esp_insights_cbor_encoder.c` around the `SHA_SIZE` usage.

### Technical details
- **Commands**
- `git -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo log --oneline -n 20`
- `git -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo diff -- dependencies.lock`

## Step 4: Import and assess esp_insights SHA_SIZE research

I imported the intern’s research note into the ticket and reviewed it to determine the most likely fix for the `SHA_SIZE` build error. The note confirms the break is caused by dependency drift: `esp_insights` v1.2.0 allows newer `esp_diag_data_store`/`esp_diagnostics` releases where the legacy `SHA_SIZE` macro no longer exists.

The recommendation is to either update/pin component versions so the expected macros align, or patch `esp_insights` to use `DIAG_SHA_SIZE`/`DIAG_HEX_SHA_SIZE` (documented in Espressif’s diagnostics docs) instead of `SHA_SIZE`. This gives us a clear decision point for the build fix.

### What I did
- Imported `/tmp/research-esp-insights.md` into the ticket sources.
- Reviewed the findings and extracted the actionable options for fixing the build.

### Why
- We need to understand the root cause before changing component versions or patching `esp_insights`.

### What worked
- The research provides a clear explanation of the dependency drift and viable fixes.

### What didn't work
- N/A

### What I learned
- `esp_insights` v1.2.0 is compatible only with older `esp_diag_data_store`/`esp_diagnostics` versions; newer versions remove the legacy `SHA_SIZE` macro.

### What was tricky to build
- N/A (research-only step).

### What warrants a second pair of eyes
- Decide whether to update `esp_insights` to v1.2.2+ or patch `esp_insights` locally to reference `DIAG_*` macros.

### What should be done in the future
- Choose a remediation path: pin dependencies or patch `esp_insights`.

### Code review instructions
- Review the imported research note under ticket sources for the dependency analysis.

### Technical details
- Source imported: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/sources/local/Research: esp_insights SHA_SIZE.md`

## Step 5: Bump esp_insights dependency to restore UserDemo build

I chose the least invasive path from the research: bump `esp_insights` to a version that matches the newer diagnostics dependencies so the `SHA_SIZE` macro mismatch goes away. The goal was to get the UserDemo build green without reintroducing TinyUSB/UVC.

After updating `idf_component.yml` and running a managed-components refresh, the build succeeded with the IDF 5.1.4 toolchain and the correct Python environment. This confirms the esp_insights mismatch was the last blocker, not the UVC/TinyUSB changes.

### What I did
- Updated `idf_component.yml` to request `espressif/esp_insights` version `1.2.2`.
- Ran `idf.py update-dependencies` to refresh `dependencies.lock`.
- Rebuilt the UserDemo with IDF 5.1.4 and the IDF 5.1 Python environment.

### Why
- The `SHA_SIZE` error was caused by managed component drift; updating `esp_insights` aligns it with newer diagnostics dependencies.

### What worked
- The build completed successfully after the dependency update.

### What didn't work
- N/A

### What I learned
- The build failure was independent of TinyUSB removal; it was a pure managed-component mismatch.

### What was tricky to build
- Ensuring the correct IDF 5.1 Python environment was active during `update-dependencies` and build.

### What warrants a second pair of eyes
- Confirm the `esp_insights` version bump is acceptable for the broader project and doesn’t conflict with any runtime expectations.

### What should be done in the future
- Reflash and verify USB Serial/JTAG enumeration on the actual device with UVC disabled.

### Code review instructions
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/idf_component.yml` for the esp_insights bump.
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/dependencies.lock` for the resolved managed versions.

### Technical details
- **Commands**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo update-dependencies`
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo build`

## Step 6: Create minimal JTAG Serial test firmware (0041) and build

To isolate USB Serial/JTAG enumeration issues, I created a brand-new ESP-IDF project that does nothing but print a 1 Hz tick over the USB Serial/JTAG console. The intent is to remove all other subsystems (camera, Wi-Fi, UVC, Arduino) so we can validate the console path in the simplest possible environment.

I also set the target to `esp32s3`, added explicit USB Serial/JTAG console defaults, and built with the IDF 5.1.4 toolchain. The build succeeded, producing a flashable image we can test against the device.

**Commit (code):** f573748 — "Add minimal JTAG serial test firmware"

### What I did
- Created the `0041-atoms3r-cam-jtag-serial-test` project with minimal CMake and a single `main.c`.
- Added `sdkconfig.defaults` to force the USB Serial/JTAG console and disable UART console.
- Copied `.envrc` from the UserDemo to target IDF 5.1.4 (local-only; ignored by repo).
- Set target to `esp32s3` and built the project with the IDF 5.1.4 Python environment.

### Why
- A minimal firmware isolates console enumeration from the rest of the camera stack.

### What worked
- `idf.py build` completed and produced `atoms3r_cam_jtag_serial_test.bin`.

### What didn't work
- The first build attempt failed because the shell was still pointing at the IDF 5.4 Python env; clearing `IDF_PYTHON_ENV_PATH` resolved it.

### What I learned
- The default environment in this workspace still points at IDF 5.4.x, so the IDF 5.1.4 Python env must be set explicitly.

### What was tricky to build
- Ensuring the correct IDF toolchain and Python env were in effect; otherwise builds fail early.

### What warrants a second pair of eyes
- Sanity-check that the USB Serial/JTAG defaults are sufficient for enumeration on the AtomS3R-CAM hardware.

### What should be done in the future
- Flash the 0041 firmware to hardware and confirm USB Serial/JTAG enumeration behavior.

### Code review instructions
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c` for the minimal logging loop.
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults` for console configuration.

### Technical details
- **Commands**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test set-target esp32s3`
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test build`

## Step 7: Attempt flash/monitor for 0041 and capture USB port failure

I tried to flash and monitor the minimal JTAG Serial test firmware to validate USB Serial/JTAG enumeration, but the flash step failed because `/dev/ttyACM0` was busy or not present. The intent here was to move from a successful build to an on-device check.

The failure indicates the port is either occupied by another process (monitor, modem manager, etc.) or the device enumerated under a different node. We need the correct port or an exclusive lock before proceeding.

### What I did
- Ran `idf.py flash monitor` for the 0041 project using the IDF 5.1.4 environment.

### Why
- To validate that the minimal firmware enumerates the USB Serial/JTAG console on real hardware.

### What worked
- The project rebuilt and generated the binary during the flash attempt.

### What didn't work
- Flash failed due to an unavailable port:
  ```
  A fatal error occurred: Could not open /dev/ttyACM0, the port is busy or doesn't exist.
  ([Errno 11] Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable)
  ```

### What I learned
- The current device node is either incorrect or locked; we need to identify the active USB device or free the port.

### What was tricky to build
- N/A (flash-stage failure).

### What warrants a second pair of eyes
- Confirm which device node the AtomS3R-CAM enumerates on this host when USB Serial/JTAG is enabled.

### What should be done in the future
- Retry flashing with the correct port once the device is free.

### Code review instructions
- N/A (no code changes in this step).

### Technical details
- **Command**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test -p /dev/ttyACM0 flash monitor`

## Step 11: Flash 0041 camera-only firmware and capture init failure logs

I flashed the updated 0041 camera-only firmware and captured the boot logs over USB Serial/JTAG. The firmware boots and the camera init path runs, but SCCB/I2C scan finds no devices and `esp_camera_init` fails with `ESP_ERR_NOT_FOUND`.

The logs also show `PSRAM disabled (CONFIG_SPIRAM not set)` despite the defaults file, which suggests `sdkconfig` retained an older value. This likely affects frame buffer placement, but the failure happens earlier during sensor probe.

### What I did
- Flashed and monitored 0041 in a tmux session, then captured the boot logs.

### Why
- To verify whether the camera-only firmware can detect the sensor and capture frames.

### What worked
- Flash succeeded and the firmware booted to `app_main`.
- The SCCB scan and camera config logging ran as expected.

### What didn't work
- SCCB scan found no devices and `esp_camera_init` failed:
  ```
  W (241) cam_test: sccb scan: no devices found
  E (251) camera: Camera probe failed with error 0x105(ESP_ERR_NOT_FOUND)
  E (251) cam_test: esp_camera_init failed: ESP_ERR_NOT_FOUND
  ```
- PSRAM reported disabled:
  ```
  W (241) cam_test: PSRAM disabled (CONFIG_SPIRAM not set)
  ```

### What I learned
- The failure is at sensor probe time (SCCB), not frame capture.
- The current `sdkconfig` is not honoring `CONFIG_SPIRAM=y` from defaults.

### What was tricky to build
- N/A (flash/boot outcome).

### What warrants a second pair of eyes
- Confirm whether the camera power rail is actually enabled on GPIO18 for the non‑M12 board.
- Confirm whether `sdkconfig` needs regeneration to apply PSRAM defaults.

### What should be done in the future
- Regenerate `sdkconfig` (or `fullclean`) to force `CONFIG_SPIRAM=y`.
- Reflash and recheck SCCB scan results.

### Code review instructions
- N/A (no code changes in this step).

### Technical details
- **Command**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test -p /dev/ttyACM0 flash monitor`

## Step 8: Add camera-only init/capture loop to 0041

I replaced the minimal tick firmware with a camera-only probe app that initializes the AtomS3R-CAM sensor, logs configuration and sensor status, and captures frames in a loop. The goal is to isolate camera bring-up from Wi-Fi/USB so we can compare logs against the failing firmware.

This step pulls in the esp32-camera component locally, adds pin configuration and power GPIO controls, and adds verbose logging for SCCB/I2C, PSRAM, sensor IDs, and frame metadata. I also fixed a build break caused by an unnecessary esp_timer include.

**Commit (code):** efc7afc — "Add camera-only test firmware for AtomS3R"

### What I did
- Copied the `esp32-camera` component into `0041-atoms3r-cam-jtag-serial-test/components`.
- Added camera pin configuration (`camera_pin.h`) and a new `Kconfig.projbuild`.
- Rewrote `main.c` to initialize the camera and capture frames with verbose logging.
- Updated `sdkconfig.defaults` with camera, PSRAM, and SCCB defaults.
- Updated `README.md` to reflect the camera test focus.

### Why
- We need a minimal camera-only firmware to diagnose sensor init failures without Wi-Fi/USB complexity.

### What worked
- The project builds successfully with IDF 5.1.4, producing `atoms3r_cam_jtag_serial_test.bin`.

### What didn't work
- Initial build failed due to a missing component dependency:
  ```
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c:11:10: fatal error: esp_timer.h: No such file or directory
  ```
  I removed the unused include to fix it.

### What I learned
- The camera-only app can stay minimal without pulling in extra component dependencies.

### What was tricky to build
- Ensuring the new camera component and Kconfig options stayed minimal while still mirroring AtomS3R-CAM pin and SCCB settings.

### What warrants a second pair of eyes
- Validate that the chosen pixel format/framesize and power GPIO assumptions match the non-M12 AtomS3R-CAM board.

### What should be done in the future
- Flash and capture logs from the new camera-only firmware to compare sensor init behavior.

### Code review instructions
- Start at `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c` for the camera init flow and logging.
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/Kconfig.projbuild` for pin/power config options.
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults` for the default settings.

### Technical details
- **Commands**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test build`

## Step 9: Align 0041 camera init flow with UserDemo

I compared the new 0041 camera-only app with the AtomS3R-CAM UserDemo and brought the init sequence closer to the demo’s behavior. The goal was to remove differences that could explain why our other firmware fails camera init.

The updated 0041 app now performs the same GPIO18 power enable style, runs a SCCB/I2C scan before camera init, and applies the same sensor tuning (vflip/brightness/saturation adjustments) that the UserDemo uses.

**Commit (code):** 5e8f1f4 — "Align 0041 camera init flow with UserDemo"

### What I did
- Added a SCCB/I2C bus scan before `esp_camera_init`, mirroring the demo’s scan.
- Set the camera power GPIO pull-down mode like the demo’s `enable_camera_power`.
- Applied the same sensor tuning logic from `camera_init.c` (vflip, brightness/saturation tweaks by PID).

### Why
- To make the camera-only probe behave like the known-good UserDemo implementation.

### What worked
- The project builds successfully after the alignment changes.

### What didn't work
- N/A

### What I learned
- The UserDemo explicitly scans SCCB and configures GPIO18 pull-down before init; matching this sequence is safer for comparisons.

### What was tricky to build
- Keeping the alignment minimal while retaining the extra debug logging we need.

### What warrants a second pair of eyes
- Confirm that the SCCB scan doesn’t interfere with the sensor on your hardware (it should be non-invasive).

### What should be done in the future
- Flash 0041 and capture the SCCB scan + sensor ID logs for comparison.

### Code review instructions
- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c` for the SCCB scan and sensor tuning additions.

### Technical details
- **Command**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test build`

## Step 10: Flash attempt blocked by busy /dev/ttyACM0

I attempted to flash and monitor the updated 0041 camera-only firmware, but the flash failed because the USB serial device was busy. This prevents verifying the camera init logs on hardware until the port is freed.

### What I did
- Ran `idf.py -p /dev/ttyACM0 flash monitor` for 0041 inside tmux.

### Why
- To validate the camera-only firmware and capture SCCB/sensor logs.

### What worked
- Build artifacts were ready; flashing started before the port lock failure.

### What didn't work
- Flash failed due to port lock:
  ```
  A fatal error occurred: Could not open /dev/ttyACM0, the port is busy or doesn't exist.
  ([Errno 11] Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable)
  ```

### What I learned
- Something else is holding `/dev/ttyACM0` (likely an existing monitor session).

### What was tricky to build
- N/A (flash-stage failure).

### What warrants a second pair of eyes
- Confirm which process owns `/dev/ttyACM0` before the next flash attempt.

### What should be done in the future
- Free `/dev/ttyACM0` and retry flashing to capture logs.

### Code review instructions
- N/A (no code changes in this step).

### Technical details
- **Command**
- `unset IDF_PYTHON_ENV_PATH; source /home/manuel/esp/esp-idf-5.1.4/export.sh; export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test -p /dev/ttyACM0 flash monitor`

## Step 5: Bump esp_insights via project manifest and rebuild

This step adds a project-level `idf_component.yml` to request a newer `esp_insights` release and refreshes managed dependencies. The dependency solver resolved to `esp_insights` 1.3.1 (not 1.2.2), and the subsequent build completed successfully with a generated `usb_webcam.bin`.

Notably, the component list no longer includes `espressif__tinyusb`, confirming that TinyUSB remains excluded when UVC is disabled.

### What I did
- Added `idf_component.yml` to request `espressif/esp_insights` 1.2.2.
- Ran `idf.py update-dependencies` under IDF 5.1.4.
- Built the project to confirm the `SHA_SIZE` error is gone.

### Why
- We needed to move off `esp_insights` 1.2.0 to avoid the undefined `SHA_SIZE` macro.

### What worked
- Dependency resolution completed and selected `esp_insights` 1.3.1.
- Build succeeded and produced `build/usb_webcam.bin`.

### What didn't work
- The solver did not pick 1.2.2; it resolved to 1.3.1 instead.

### What I learned
- The component manager may select a newer `esp_insights` than requested, likely due to compatibility constraints.

### What was tricky to build
- Ensuring the correct IDF 5.1.4 Python environment was active for dependency updates and builds.

### What warrants a second pair of eyes
- Verify whether we should accept `esp_insights` 1.3.1 or enforce 1.2.2 explicitly.

### What should be done in the future
- Flash the new build and test USB Serial/JTAG enumeration with UVC disabled.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/idf_component.yml` for the esp_insights pin.
- Review `../ATOMS3R-CAM-UserDemo/dependencies.lock` for resolved component versions.

### Technical details
- **Commands**
- `export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; source /home/manuel/esp/esp-idf-5.1.4/export.sh; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo update-dependencies`
- `export IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.1_py3.11_env; source /home/manuel/esp/esp-idf-5.1.4/export.sh; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo build`
