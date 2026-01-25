# Changelog

## 2026-01-24

- Initial workspace created


## 2026-01-24

Step 2: Implement registry load/save + nickname resolution (esper commit 91d291d)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/devices/registry.go — New devices.json registry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/devices/resolve.go — Resolve nickname to port


## 2026-01-24

Step 3: Add devices commands + scan annotation + monitor resolution (esper commit 72f2c72)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/cmd/esper/main.go — Monitor resolves nickname
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/commands/devicescmd/root.go — devices CLI
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/commands/scancmd/scan.go — Scan shows nickname

