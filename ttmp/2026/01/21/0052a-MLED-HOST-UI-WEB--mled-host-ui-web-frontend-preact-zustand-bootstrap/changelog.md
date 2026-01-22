# Changelog

## 2026-01-21

- Initial workspace created


## 2026-01-21

Step 1: Scaffolded complete Vite + Preact + Zustand frontend (commit 2fdd682). Implemented all 4 screens: Nodes, Patterns, Status, Settings. TypeScript compiles cleanly.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/api.ts — API client with HTTP and SSE
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/app.tsx — Main app shell with tab navigation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/store.ts — Zustand store with all state and actions


## 2026-01-21

Step 2: Fixed infinite render loop and runtime errors (commit 7875160). SSE debounce, useMemo/useCallback patterns, defensive null checks.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/api.ts — SSE error handling with debounce
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Nodes.tsx — useMemo/useCallback for stable renders


## 2026-01-21

Step 3: Fixed API response format to match backend (commit ac7d8db). Frontend now works with real backend and displays real MLED nodes.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/api.ts — API response format fixes

