# Changelog

## 2025-12-29

- Initial workspace created


## 2025-12-29

Step 2: chmod +x build.sh; rebuilt UART echo firmware (code commit a9d186a)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware/build.sh — Executable bit fix to unblock ./build.sh usage


## 2025-12-29

Steps 3-4: QEMU fg (mon:stdio) reveals early Task WDT panic (wdt_disable knob ineffective); compared against imports mqjs-repl transcript; decided to disable WDTs via sdkconfig.defaults and force regen via fullclean

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware/sdkconfig.defaults — WDT disable setting for QEMU
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/sources/track-c-qemu-mon-stdio-piped-20251229-153152.txt — Transcript showing Task WDT panic loop

