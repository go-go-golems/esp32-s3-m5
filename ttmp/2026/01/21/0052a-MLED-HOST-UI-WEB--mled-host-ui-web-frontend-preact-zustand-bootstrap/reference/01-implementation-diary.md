---
Title: Implementation Diary
Ticket: 0052a-MLED-HOST-UI-WEB
Status: active
Topics:
    - ui
    - preact
    - zustand
    - frontend
    - vite
    - bootstrap
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/store.ts
      Note: "Zustand store with nodes, presets, settings, selection state"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/api.ts
      Note: "API client module with HTTP endpoints and SSE event handling"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/app.tsx
      Note: "Main App shell with 3-tab navigation"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Nodes.tsx
      Note: "Nodes screen: node list, preset bar, brightness control"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Patterns.tsx
      Note: "Patterns screen: preset list and editor with dynamic params"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Status.tsx
      Note: "Status screen: system banner, problem list, node table"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Settings.tsx
      Note: "Settings modal: network config, timing, import/export"
ExternalSources: []
Summary: "Step-by-step implementation diary for the MLED Controller web frontend built with Vite + Preact + Zustand + Bootstrap."
LastUpdated: 2026-01-21T19:11:10.883982216-05:00
WhatFor: "Track implementation progress, decisions, and learnings for the MLED Controller web UI."
WhenToUse: "Reference when reviewing implementation, onboarding to the codebase, or continuing work on the frontend."
---

# Implementation Diary

## Goal

Build a web frontend for the MLED Controller using Vite + Preact + Zustand + Bootstrap. The UI implements the "Exhibition Edition" design with 3 main tabs (Nodes, Patterns, Status) plus Settings.

## Context

This frontend is part of ticket 0052a-MLED-HOST-UI-WEB, which focuses on the JS/HTML side while a colleague works on the Go backend (0052-MLED-HOST-UI). The frontend will be embedded into the Go binary via go:embed.

Key requirements:
- Preact + Zustand (not React + RTK) for minimal bundle size
- Bootstrap for CSS (dark theme with industrial control panel aesthetic)
- Vite for fast dev experience and production builds
- SSE (EventSource) for live updates, not WebSockets
- API proxy in dev mode to Go backend on :8080

---

## Step 1: Project Scaffolding

Initial setup of the Vite + Preact + TypeScript project with all required dependencies.

**Commit (code):** 2fdd682 — "Web UI: Scaffold Vite + Preact + Zustand frontend for MLED Controller"

### What I did

1. Created `mled-server/web/` directory
2. Scaffolded Vite + Preact + TypeScript project using `npm create vite@latest . -- --template preact-ts`
3. Installed dependencies:
   - `preact`, `zustand`, `bootstrap`, `preact-router`, `@types/node`
4. Configured `vite.config.ts`:
   - API proxy to `http://localhost:8080` for `/api` routes
   - Dev server on port 3000
5. Created TypeScript types in `src/types.ts`:
   - `Node`, `Preset`, `Settings`, `PatternConfig`, `ControllerStatus`
   - SSE event types: `NodeUpdateEvent`, `ApplyAckEvent`, etc.
6. Created Zustand store in `src/store.ts`:
   - State: nodes (Map), selectedNodeIds (Set), presets, settings, currentTab, etc.
   - Actions: selectNode, toggleSelection, selectAll, applyPreset, etc.
   - Derived selectors: useOnlineNodes, useProblematicNodes, etc.
   - Default presets: Rainbow, Calm, Warm, Pulse, Off
7. Created API client in `src/api.ts`:
   - HTTP endpoints: fetchNodes, discoverNodes, applyPattern, CRUD presets, settings
   - SSE connection with `EventSource` for live updates
   - `loadInitialData()` for initial hydration
8. Implemented all 4 screens:
   - **Nodes**: NodeList with checkboxes, status dots, RSSI, pattern info; PresetBar with quick-apply buttons; BrightnessControl slider with Apply button
   - **Patterns**: PresetList with selectable items; PresetEditor with name, icon, type dropdown, dynamic PatternParams by type, preview/save/delete
   - **Status**: SystemBanner with health status; ProblemList with auto-detection; NodeTable with sortable columns
   - **Settings**: Modal with network config (bind IP, multicast), timing settings, import/export presets
9. Created custom CSS in `src/index.css`:
   - Dark theme with industrial control panel aesthetic
   - CSS variables for consistent colors
   - Custom styling for nodes, presets, status indicators
   - JetBrains Mono font for monospace feel

### Why

The goal is to build a complete, functional UI that can work with mock data immediately while waiting for the backend to be implemented. This allows parallel development.

### What worked

- Vite + Preact scaffolding is very fast and clean
- Zustand provides Redux-like ergonomics with much less boilerplate
- Bootstrap provides a solid foundation but needed heavy customization for the dark theme
- TypeScript caught several type issues early

### What was tricky to build

- Dynamic PatternParams component that renders different controls based on pattern type
- Managing preset editing state (new vs edit, cancel resets, etc.)
- SSE event handling with proper type narrowing for different event types
- Making the Map/Set state work properly with Zustand (need to create new instances to trigger re-renders)

### What warrants a second pair of eyes

1. SSE reconnection logic - currently relies on EventSource auto-reconnect but may need manual backoff
2. State normalization - nodes stored in Map, selection in Set - verify this works with Zustand's shallow comparison
3. Preset persistence - currently using API calls, need to verify the backend contract matches

### What should be done in the future

- Add loading spinners for async operations
- Add toast notifications for success/error feedback
- Add keyboard shortcuts (Ctrl+A for select all, Enter for apply)
- Consider virtualization for large node lists (100+ nodes)
- Add offline mode with localStorage persistence

### Code review instructions

1. Start in `src/store.ts` - understand the state shape and actions
2. Check `src/api.ts` - verify API contract matches the backend spec in 0052-MLED-HOST-UI
3. Review screens in order: Nodes → Patterns → Status → Settings
4. Test with: `cd mled-server/web && npm run dev`

### Technical details

**File structure:**
```
mled-server/web/
├── index.html
├── package.json
├── vite.config.ts
├── tsconfig.json
└── src/
    ├── main.tsx           # Entry point
    ├── app.tsx            # App shell + tab navigation
    ├── store.ts           # Zustand store
    ├── api.ts             # HTTP + SSE client
    ├── types.ts           # TypeScript interfaces
    ├── index.css          # Custom dark theme
    └── screens/
        ├── Nodes.tsx      # Main nodes view
        ├── Patterns.tsx   # Preset editor
        ├── Status.tsx     # System status
        └── Settings.tsx   # Settings modal
```

**API endpoints expected from backend:**
- `GET /api/nodes` - list all nodes
- `POST /api/nodes/discover` - trigger discovery
- `POST /api/apply` - apply pattern to nodes
- `GET/POST/PUT/DELETE /api/presets` - CRUD presets
- `GET/PUT /api/settings` - read/write settings
- `GET /api/status` - controller status
- `GET /api/events` - SSE stream
- `GET /api/net/interfaces` - list network interfaces

---

## Step 2: Fix Infinite Render Loop and Runtime Errors

After initial testing, the UI caused Firefox to warn about excessive CPU usage due to an infinite render loop. Investigation revealed two root causes, both related to Zustand + Preact reactivity patterns.

**Commit (code):** 7875160 — "Web UI: Fix infinite render loop and defensive null checks"

### What I did

1. Fixed SSE reconnection loop in `api.ts`:
   - Added `lastErrorTime` debounce to prevent rapid state updates on repeated errors
   - Added `sseEnabled` flag to disable SSE after errors (re-enabled after 10s timeout)
   - Close EventSource on error to prevent auto-reconnect storm

2. Fixed Zustand selector re-render loops in all screen components:
   - Changed from inline `Array.from(s.nodes.values())` to `useMemo` pattern
   - Added `useCallback` for all event handlers passed as props
   - Stored Map reference directly, derived arrays in `useMemo`

3. Added defensive null checks:
   - `setNodes` now handles `undefined` input: `(nodes || []).map(...)`
   - All `useMemo` calls check `nodesMap ? Array.from(...) : []`

4. Fixed vite.config.ts API proxy port: `8080` → `8765`

5. Fixed type import: split `import { useAppStore, TabId }` into value + type imports

### Why

Zustand's shallow comparison triggers re-renders when selector returns a new object/array each time. Creating `new Array` inside a selector (via `Array.from()`) causes infinite re-renders because each render creates a new array reference.

### What worked

- `useMemo` with the Map as dependency correctly memoizes derived arrays
- `useCallback` stabilizes event handler references passed to child components
- SSE debounce + disable pattern prevents connection storm when backend is unavailable

### What didn't work

- Initial approach of just using `[]` dependency in useEffect wasn't enough
- Inline arrow functions in props (`onClick={() => fn(x)}`) caused PresetButton to re-render infinitely

### What I learned

- Zustand selectors that return derived data (arrays from Maps, filtered lists) must be memoized outside the selector
- EventSource auto-reconnects on error by default - must explicitly close() and manage reconnection manually
- Preact's HMR + Zustand devtools can amplify render issues during development

### What was tricky to build

- Tracking down which component was causing the loop (PresetButton via stack trace)
- Understanding Zustand's shallow comparison semantics with Map/Set state

### What warrants a second pair of eyes

1. The SSE error handling logic - may need to distinguish between "backend down" vs "temporary network glitch"
2. The 10-second timeout for re-enabling SSE is arbitrary - should this be configurable?

### What should be done in the future

- Consider using `zustand/shallow` for multi-value selectors
- Add a "reconnect" button in UI when SSE is disabled
- Profile render performance with React DevTools (works with Preact via compat)

### Code review instructions

1. Review `api.ts` lines 115-155 for SSE error handling
2. Review `screens/Nodes.tsx` for `useMemo`/`useCallback` patterns
3. Test by:
   - Starting frontend without backend → should show "Disconnected", no CPU spike
   - Navigating between tabs → should be instant, no re-render loops

---

## Step 3: API Response Format Fixes

Connected the frontend to the real backend and fixed response format mismatches that prevented data from loading.

**Commit (code):** ac7d8db — "Web UI: Fix API response format to match backend"

### What I did

1. Fixed `fetchNodes()`: Backend returns array directly `[...]`, not `{ nodes: [...] }`
2. Fixed `fetchPresets()`: Backend returns `null` when no presets saved, not empty array
3. Fixed `fetchStatus()`: Backend returns `{ epoch_id, running, show_ms }`, mapped to frontend's `{ epochId, showMs, connected }`

### Why

The design doc specified one format but the backend implemented a slightly different one. Normal integration work.

### What worked

- Vite proxy correctly forwards `/api/*` requests to `localhost:8765`
- Real MLED node discovered and displayed: "c6-node" with -72dBm signal
- Presets can be created and saved to backend
- Node status (online/weak/offline) computed correctly

### What warrants a second pair of eyes

1. SSE endpoint returns `text/plain` instead of `text/event-stream` (backend fix needed)
2. When backend has no presets, UI shows only user-created presets (default presets don't persist to backend)

### What should be done in the future

- Backend should seed default presets on first run
- SSE MIME type fix in backend
- Consider merging backend presets with frontend defaults

### Code review instructions

- Check `api.ts` lines 34-37 (fetchNodes), 68-71 (fetchPresets), 88-98 (fetchStatus)
- Test by running backend and frontend together

---

## Step 4: Apply Design Patterns Throughout the App

Refactored all screens to use the three design patterns created in Step 3.

**Commit (code):** 79e4300 — "Refactor Web UI to use design patterns throughout"

### What I did

1. **Nodes.tsx**: Replaced inline selectors with hooks:
   - `useNodes()`, `useSortedNodeIds()`, `useOnlineNodes()`
   - `usePresets()`, `useSelectedCount()`, `useIsNodeSelected()`
   - Uses `apiClient.applyPattern()` instead of direct import

2. **Patterns.tsx**: Replaced inline selectors with hooks:
   - `usePresets()`, `useEditingPreset()`, `useSelectedNodeIds()`
   - Uses `apiClient.*` for all API calls

3. **Status.tsx**: Replaced inline selectors with hooks:
   - `useNodes()`, `useOnlineNodes()`, `useSortedNodes()`
   - Uses `apiClient.discoverNodes()` for refresh

4. **Settings.tsx**: Uses `usePresets()` hook

5. **app.tsx**: 
   - Uses `connectSSE`/`disconnectSSE` from connectionManager
   - Proper cleanup on unmount

6. **api.ts**: Slimmed down to:
   - Re-exports from `apiClient` for backwards compatibility
   - Only keeps `loadInitialData()` function

### Why

Applying the patterns consistently across the codebase prevents the issues we debugged earlier from recurring.

### Code size reduction

The refactoring reduced code by ~226 lines (from 325 removed to 99 added), showing that abstracting the patterns makes code more concise.

### What works now

- All screens use stable selectors (no render loops)
- SSE connection has proper exponential backoff
- API calls go through type-safe transformers
- Clean separation of concerns

---

## Related

- Parent ticket: [0052-MLED-HOST-UI](../../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/index.md)
- Design doc: [02-design-go-host-http-api-preact-zustand-ui-embedded.md](../../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/02-design-go-host-http-api-preact-zustand-ui-embedded.md)
- UI screens spec: [fw-screens.md](../../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/sources/local/fw-screens.md)
