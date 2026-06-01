### OS

Windows

### Operating System version

Windows 11 PRO

### Visual Studio Code version

1.112

### ESP-IDF version

v2.0.2

### Python version

3.8.7

### Doctor command output

All latest versions

### Extension

_No response_

### Description

My project will compile and build using my original menuconfig but flash fails because the chip revision is not in the range v3.1 - v3.99. If I set the menuconfig entry to tick <v3.0 the project compiles but will not build:
`-esp-elf/14.2.0/../../../../riscv32-esp-elf/bin/ld.exe: error: --enable-non-contiguous-regions discards section `.sdata.s_time_update_lock' from `esp-idf/esp_timer/libesp_timer.a(esp_timer_impl_common.c.obj)'

etc...  etc... (Another 82kB of similar errors)

C:/Espressif/tools/riscv32-esp-elf/esp-14.2.0_20251107/riscv32-esp-elf/bin/../lib/gcc/riscv32-esp-elf/14.2.0/../../../../riscv32-esp-elf/bin/ld.exe: error: --enable-non-contiguous-regions discards section `.sdata.__global_locale_ptr' from `C:/Espressif/tools/riscv32-esp-elf/esp-14.2.0_20251107/riscv32-esp-elf/bin/../lib/gcc/riscv32-esp-elf/14.2.0/../../../../riscv32-esp-elf/lib/rv32imafc_zicsr_zifencei/ilp32f/no-rtti\libc.a(libc_a-locale.o)'
C:/Espressif/tools/riscv32-esp-elf/esp-14.2.0_20251107/riscv32-esp-elf/bin/../lib/gcc/riscv32-esp-elf/14.2.0/../../../../riscv32-esp-elf/bin/ld.exe: error: Total discarded sections size is 20486 bytes
collect2.exe: error: ld returned 1 exit status
ninja: build stopped: subcommand failed.`

### Debug Message

ninja: build stopped: subcommand failed

### Other Steps to Reproduce

Reverting to an older version did not resolve the issue which appears to be caused by different memory mappings.

### I have checked existing issues, online documentation and the Troubleshooting Guide

- [x] I confirm I have checked existing issues, online documentation and Troubleshooting guide.
