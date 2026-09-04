SPDX-License-Identifier: MIT
---
# gogolem_nfc

Reusable native ESP-IDF NFC component for the M5StackChan ST25R3916 front end.

This component wraps the pinned M5Unit-NFC protocol layer behind a stable,
board-independent API. The application owns the ESP-IDF I²C bus; the component
owns NFC protocol state. It does not create a bus, start a console, initialize
NVS, reboot the MCU, or print diagnostics.

See ticket `ESP-61-REUSABLE-NFC-COMPONENT` for the full design and the phased
implementation guide.

## Phase 1 scope

Phase 1 ships the dependency-free foundation only:

- `gogolem/nfc/types.hpp` — domain enums and records (Mode, LifecycleState,
  ErrorLayer, Operation, TagFamily, Error, TagInfo) plus name helpers.
- `gogolem/nfc/result.hpp` — `Result<T>` and `Result<void>` success/failure
  values without C++ exceptions.
- `gogolem/nfc/version.hpp` — component version accessors.

These headers are host-clean: they include only standard C++ headers and compile
under a plain `g++`. Higher phases add the synchronous Engine, the worker
Service, NDEF, MIFARE Classic, and target emulation on top of M5Unit-NFC.

## Build (target)

```bash
source ~/esp/esp-idf-5.5.4/export.sh
idf.py build
```

Consuming projects add `gogolem_nfc` to their `REQUIRES` (or
`EXTRA_COMPONENT_DIRS`) and `#include <gogolem/nfc/types.hpp>`.

## Host tests

```bash
cd components/gogolem_nfc
./test_host/build.sh
```

No ESP-IDF installation is required for the host tests.

## Ownership contract

- The caller creates and owns the `i2c_master_bus_handle_t`.
- The caller must keep the bus valid for the lifetime of any Engine/Service that
  uses it, and must stop the component before deleting the bus.
- One execution context may call the Engine at a time. Use the Service for
  multi-task applications.

## Dependencies

Phase 1 depends only on ESP-IDF. Later phases pin M5Unit-NFC at
`93745b547364f310cd64b5155a870103a7800a5d` (recorded in each consuming
project's `dependencies.lock`).
