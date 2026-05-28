# Changelog

## 2026-05-28

- Initial workspace created


## 2026-05-28

Created proper 3D planet renderer ticket, added host prototype, wrote intern-oriented design guide and investigation diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/01-proper-3d-planet-renderer-analysis-and-implementation-guide.md — Primary analysis and implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/reference/01-investigation-diary.md — Chronological research diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py — Host-side coarse 3D planet renderer experiment


## 2026-05-28

Validated ticket with docmgr doctor, added rendering topic vocabulary entries, and uploaded design bundle to reMarkable at /ai/2026/05/28/0097.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/01-proper-3d-planet-renderer-analysis-and-implementation-guide.md — Uploaded in reMarkable bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/reference/01-investigation-diary.md — Updated with validation and upload evidence


## 2026-05-28

Generated host-side buffer comparison screenshots, revised prototype to include JSX-style split ring composition, and wrote v2 JSX-matched comparison report.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/buffer-config-comparison-v2/resolution-z16-montage.png — Updated ringed planet resolution montage
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/03-jsx-matched-buffer-configuration-report.md — Revised report with JSX-matched screenshots
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/02-compare-buffer-configs.py — Batch renderer for buffer configuration screenshots


## 2026-05-28

Implemented and flashed first M5Dial proper 3D planet firmware backend: heap diagnostics, backend switching, 80x80 uint16-Z renderer, split ring/moon composition, and dumpfb capture (code commit 66ec490).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/console_commands.cpp — backend
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/renderer3d.cpp — Proper 3D planet renderer implementation (commit 66ec490)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-first.png — First on-device dumpfb capture of the planet3d backend


## 2026-05-28

Corrected planet geometry and color model so the PLANET body reads as a spherical disk: removed radial noise displacement, kept noise as color texture only, added base red/blue density across the sphere, rebuilt, flashed, and captured updated hardware output.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/renderer3d.cpp — Sphere geometry and color-density correction
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-spherical.png — Updated hardware capture with rounder planet body
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py — Host prototype updated to keep silhouette spherical

