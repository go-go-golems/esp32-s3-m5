# ESP-02-ESPER-TUI scripts

These scripts capture the exact, repeatable command sequences used for hardware testing and TUI smoke tests.

## Usage

Run from the repo root:

- Detect port: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh`
- Build firmware: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/02-build-firmware.sh`
- Flash firmware: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/03-flash-firmware.sh`
- Scan ports: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/04-esper-scan.sh`
- Run Esper TUI: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh`

## Environment overrides

- Set `ESPPORT` to override auto-detection:
  - `ESPPORT=/dev/ttyACM0 ./.../03-flash-firmware.sh`
- Set `ESPIDF_EXPORT` to override ESP-IDF export script:
  - `ESPIDF_EXPORT=$HOME/esp/esp-idf-5.4.1/export.sh`
- Set `ESPER_ELF` to override the ELF used by Esper for decoding:
  - `ESPER_ELF=$PWD/esper/firmware/esp32s3-test/build/esp32s3-test.elf`
- Set `TOOLCHAIN_PREFIX` to override addr2line prefix:
  - `TOOLCHAIN_PREFIX=xtensa-esp32s3-elf-`

