# Changelog

## 2026-01-11

- Initial workspace created


## 2026-01-11

Created reuse-map analysis and JS API brainstorm; documented diary steps and task status.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/analysis/01-existing-firmware-analysis-and-reuse-map.md — Reusable firmware map and notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/01-js-api-and-runtime-brainstorm.md — API/runtime options and scoring
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md — Diary steps
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/tasks.md — Task status updates


## 2026-01-11

Added high-frequency GPIO pattern VM brainstorm with expert panel; updated diary and index links.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/02-high-frequency-gpio-pattern-vm-brainstorm.md — VM brainstorm and debate
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/index.md — Added link to VM brainstorm
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md — Diary step for VM design


## 2026-01-11

Added concrete GPIO/I2C implementation plan and seeded implementation tasks.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/03-concrete-implementation-plan-js-gpio-exercizer-gpio-i2c.md — Detailed plan and pseudocode
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/index.md — Linked new design doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md — Diary step for concrete plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/tasks.md — Added implementation tasks


## 2026-01-11

Updated GPIO/I2C implementation plan to use C++ components; aligned tasks and diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/03-concrete-implementation-plan-js-gpio-exercizer-gpio-i2c.md — Component-based plan updates
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md — Diary step for component update
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/tasks.md — Aligned tasks with component plan


## 2026-01-11

Implemented ControlPlane/GPIO/I2C components, added new exercizer firmware, and updated MicroQuickJS stdlib bindings.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp — Firmware wiring
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/exercizer_control/src/ControlPlane.cpp — Control plane queue + C bridge
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/exercizer_gpio/src/GpioEngine.cpp — Timer-based GPIO engine
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/exercizer_i2c/src/I2cEngine.cpp — I2C engine worker task
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c — JS bindings for gpio/i2c
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md — Diary step 7


## 2026-01-11

Step 8: Stabilize control plane lifetime + esp_timer init (commit 520973b)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp — Keep control plane alive and tolerate esp_timer already-initialized state


## 2026-01-11

Step 9: Multi-pin GPIO engine + gpio.setMany JS API (commit 7203698)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/exercizer_gpio/src/GpioEngine.cpp — Introduce multi-channel GPIO scheduling


## 2026-01-11

Step 10: Flash + validate multi-pin GPIO on hardware

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md — Record flash/monitor validation step


## 2026-01-11

Step 11: Add I2C scan + register helpers (commit 788ba5f)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/exercizer_i2c/src/I2cEngine.cpp — I2C scan implementation

