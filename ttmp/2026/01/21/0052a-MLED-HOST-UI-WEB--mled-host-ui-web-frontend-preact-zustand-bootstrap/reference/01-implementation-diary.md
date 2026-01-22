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
      Note: "API layer: re-exports from apiClient + loadInitialData (commit 79e4300)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/app.tsx
      Note: "Main App shell with 3-tab navigation, uses connectionManager (commit 79e4300)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/lib/useStableSelector.ts
      Note: "Pattern 1: Stable selector hooks to prevent render loops (commit b62ce3c)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/lib/connectionManager.ts
      Note: "Pattern 2: SSE connection manager with exponential backoff (commit b62ce3c)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/lib/apiClient.ts
      Note: "Pattern 3: API client with backend/frontend type transformers (commit b62ce3c)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Nodes.tsx
      Note: "Nodes screen: uses useNodes, useSortedNodeIds, apiClient (commit 79e4300)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Patterns.tsx
      Note: "Patterns screen: uses usePresets, useEditingPreset, apiClient (commit 79e4300)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Status.tsx
      Note: "Status screen: uses useNodes, useOnlineNodes, useSortedNodes (commit 79e4300)"
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/web/src/screens/Settings.tsx
      Note: "Settings modal: uses usePresets, apiClient (commit 79e4300)"
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

### What didn't work

- First attempt at `vite.config.ts` proxy had wrong port (8080 instead of 8765)
- Bootstrap's dark mode requires significant CSS overrides for proper dark theme
- EventSource connection attempted before initial data load completed

### What I learned

- Vite's `create vite` with `preact-ts` template is minimal - need to add routing, state, CSS manually
- Zustand's Map/Set support requires careful handling (new instances trigger re-renders)
- Bootstrap's form controls need explicit dark theme styling
- SSE EventSource auto-reconnects on error (important for Step 2 fix)

### What I'd do differently next time

- Start with a checklist of required dependencies before scaffolding
- Create the Zustand store shape first, then implement UI around it
- Add the CSS theme variables before implementing any components

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

**Zustand store shape:**
```typescript
interface AppState {
  // Node state
  nodes: Map<string, Node>;
  selectedNodeIds: Set<string>;
  
  // Presets
  presets: Preset[];
  editingPresetId: string | null;
  
  // UI state
  currentTab: 'nodes' | 'patterns' | 'status';
  globalBrightness: number;
  showSettings: boolean;
  
  // Connection
  status: ControllerStatus;
  settings: Settings;
  connected: boolean;
  lastError: string | null;
  activityLog: LogEntry[];
  
  // Actions
  setNodes: (nodes: Node[]) => void;
  updateNode: (node: Node) => void;
  selectNode: (id: string) => void;
  toggleNodeSelection: (id: string) => void;
  // ... many more
}
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

**Commands to run:**
```bash
cd mled-server/web
npm install
npm run dev  # Starts on http://localhost:3000
```

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

### Technical details

**The render loop mechanism:**
```
1. Component renders
2. Selector runs: Array.from(nodes.values()) → new array instance
3. Zustand: "new reference! trigger re-render"
4. Goto 1 (infinite loop)
```

**The fix (memoize outside selector):**
```typescript
// Selector returns stable Map reference
const nodesMap = useAppStore((s) => s.nodes);

// useMemo only re-runs when Map changes
const nodes = useMemo(
  () => (nodesMap ? Array.from(nodesMap.values()) : []),
  [nodesMap]
);
```

**SSE debounce pattern:**
```typescript
let lastErrorTime = 0;

eventSource.onerror = () => {
  const now = Date.now();
  if (now - lastErrorTime < 5000) return; // debounce
  lastErrorTime = now;
  // ... handle error
};
```

**Key error encountered (Firefox):**
```
Warning: A script on this page is using resources.
Script: http://localhost:3000/src/screens/Nodes.tsx
```

### What I'd do differently next time

- Start with the `useMemo` pattern from the beginning for any derived arrays
- Add ESLint rule to flag `Array.from()` inside Zustand selectors
- Test without backend earlier to catch SSE reconnection issues

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

### What I learned

- Always check the actual API response format before assuming what the backend returns
- `null` vs `[]` matters for TypeScript strict mode - need to handle both gracefully
- Field naming conventions differ (snake_case from Go, camelCase in TS) - transformers help

### What was tricky to build

- Mapping between backend field names (`epoch_id`) and frontend names (`epochId`)
- Deciding whether to transform in the API layer or let callers handle it

### Technical details

**Backend vs Frontend field mappings:**
```
Backend                  Frontend
-------------------------------------------------
node_id                  node_id (same)
last_seen                last_seen (same, timestamp)
epoch_id                 epochId (camelCase)
running                  connected (semantic rename)
show_ms                  showMs (camelCase)
```

**Defensive patterns for null API responses:**
```typescript
// Presets can be null
const data = await fetchJson<Preset[] | null>('/presets');
return data || [];

// Nodes should always be array
return fetchJson<Node[]>('/nodes');
```

### Code review instructions

- Check `api.ts` lines 34-37 (fetchNodes), 68-71 (fetchPresets), 88-98 (fetchStatus)
- Test by running backend and frontend together

---

## Step 3.5: Create Design Patterns Library

After debugging the render loops and API mismatches, I extracted three reusable patterns into dedicated modules. This step creates the infrastructure; Step 4 applies it throughout the app.

**Commit (code):** b62ce3c — "Add design patterns to prevent render loops and connection storms"

### What I did

1. Created `src/lib/useStableSelector.ts`:
   - `useNodes()` - all nodes as stable array (memoized from Map)
   - `useOnlineNodes()` - filtered to non-offline
   - `useSortedNodes()` - sorted by status then name
   - `useSortedNodeIds()` - just IDs for rendering keys
   - `useSelectedCount()` - primitive (no memo needed)
   - `useIsNodeSelected(id)` - boolean per node
   - `usePresets()` - all presets with null fallback
   - `useEditingPreset()` - find preset by editingPresetId
   - `useSelectedNodeIds()` - selected IDs as array

2. Created `src/lib/connectionManager.ts`:
   - `SSEConnectionManager` class with lifecycle management
   - Exponential backoff: 1s → 2s → 4s → ... → 30s max
   - Debounces rapid errors (within 1s)
   - Proper `connect()`/`disconnect()` API
   - Singleton pattern via `getConnectionManager()`
   - Registers all SSE event handlers: node, node.offline, ack, log

3. Created `src/lib/apiClient.ts`:
   - Explicit `BackendNode`, `BackendStatus`, `BackendSettings` types
   - Transformer functions: `transformNode()`, `transformStatus()`, `transformSettings()`
   - `ApiError` class with status code
   - `apiClient` object with all API methods
   - Safe defaults for missing fields

4. Created design doc: `design-doc/01-design-patterns-for-preact-zustand-frontend.md`
   - Documents the three patterns with rationale
   - Explains the problems they solve
   - Shows before/after code examples

### Why

Extracting these patterns:
- Prevents re-introducing the same bugs
- Makes the codebase more readable (hooks hide complexity)
- Documents the "why" alongside the code
- Creates a template for future Preact/Zustand projects

### What worked

- Hooks pattern is very Preact-idiomatic
- Connection manager singleton works well for SSE
- Backend types + transformers catch drift at compile time

### What was tricky to build

- Deciding which hooks to create (too few = still duplicating, too many = over-abstraction)
- Getting the exponential backoff timing right
- Ensuring transformers handle all nullable fields

### What warrants a second pair of eyes

1. `useIsNodeSelected(id)` uses inline closure - verify it doesn't cause re-renders
2. Connection manager's singleton pattern - is this the right approach for testing?
3. Transformer completeness - are we handling all edge cases from the backend?

### What should be done in the future

- Add unit tests for transformers (null handling, edge cases)
- Add integration tests for connection manager (mock EventSource)
- Consider adding a hook for connection state (`useConnectionStatus()`)

### Code review instructions

1. Start with `useStableSelector.ts` - verify `useMemo` dependencies are correct
2. Check `connectionManager.ts` - verify backoff math and event handler registration
3. Review `apiClient.ts` - verify backend types match actual API
4. Read design doc for rationale

### Technical details

**Stable selector pattern:**
```typescript
// Select raw Map (stable reference)
const nodesMap = useAppStore((s) => s.nodes);

// Derive array outside selector, memoized
return useMemo(
  () => (nodesMap ? Array.from(nodesMap.values()) : []),
  [nodesMap]
);
```

**Exponential backoff implementation:**
```typescript
private scheduleReconnect(): void {
  this.retryTimeout = window.setTimeout(() => {
    this.connect();
  }, this.retryDelay);
  
  // Increase delay for next failure
  this.retryDelay = Math.min(
    this.retryDelay * this.config.backoffMultiplier,  // 2x
    this.config.maxRetryDelay  // 30s max
  );
}
```

**Backend type + transformer pattern:**
```typescript
interface BackendNode {
  node_id: string;
  name: string;
  // ... exact backend fields
}

function transformNode(backend: BackendNode): Node {
  return {
    node_id: backend.node_id,
    name: backend.name || backend.node_id,  // default if missing
    // ...
  };
}
```

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

### What worked

- Hooks provide clean abstraction over selector complexity
- `apiClient.` method chaining is intuitive
- Connection manager's `disconnectSSE()` provides proper cleanup on unmount
- Code is significantly more readable

### What didn't work

- Initial attempt to just change imports broke some component type checks
- Had to ensure `usePresets()` returned `[]` not `undefined` for `.map()` calls

### What I learned

- Refactoring in layers works: create patterns → apply patterns → verify
- TypeScript strict mode catches most integration issues at compile time
- Screen components become much simpler when state derivation is externalized

### What was tricky to build

- Ensuring all import paths were correct after moving to `lib/`
- Verifying that no inline selectors remained (could cause subtle bugs)
- Making sure `apiClient.*` methods were called instead of old `import { fn } from '../api'`

### What warrants a second pair of eyes

1. Check that no component still uses raw `useAppStore((s) => Array.from(...))` pattern
2. Verify `app.tsx` cleanup in useEffect return properly calls `disconnectSSE()`
3. Confirm backwards compatibility via `api.ts` re-exports works

### What should be done in the future

- Remove the backwards-compat re-exports in `api.ts` once all callers use `apiClient` directly
- Add ESLint rule to prevent raw `Array.from()` in Zustand selectors
- Add bundle size analysis to verify Zustand + Preact stays small

### Code review instructions

1. `git diff b62ce3c..79e4300` to see all changes
2. Grep for `useAppStore` in screens - should only select primitives or Maps/Sets directly
3. Verify each screen imports from `../lib/useStableSelector` and `../lib/apiClient`
4. Run app, open DevTools Network tab, verify API calls work

### Technical details

**Before (Nodes.tsx):**
```typescript
const nodesMap = useAppStore((s) => s.nodes);
const selectedNodeIds = useAppStore((s) => s.selectedNodeIds);
const nodes = useMemo(() => nodesMap ? Array.from(nodesMap.values()) : [], [nodesMap]);
const sortedNodeIds = useMemo(() => [...nodes].sort(...).map((n) => n.node_id), [nodes]);
const onlineNodes = useMemo(() => nodes.filter((n) => n.status !== 'offline'), [nodes]);
const offlineNodes = useMemo(() => nodes.filter((n) => n.status === 'offline'), [nodes]);
```

**After (Nodes.tsx):**
```typescript
const nodes = useNodes();
const sortedNodeIds = useSortedNodeIds();
const onlineNodes = useOnlineNodes();
const offlineCount = nodes.length - onlineNodes.length;
```

**Line count comparison:**
| File | Before | After | Δ |
|------|--------|-------|---|
| Nodes.tsx | 274 | 265 | -9 |
| Patterns.tsx | 519 | 408 | -111 |
| Status.tsx | 270 | 254 | -16 |
| Settings.tsx | 209 | 210 | +1 |
| api.ts | 271 | 64 | -207 |
| **Total** | **1543** | **1201** | **-342** |

(Note: Patterns.tsx had some duplicate code that was cleaned up; Settings.tsx gained `useCallback` wrapping)

### What works now

- All screens use stable selectors (no render loops)
- SSE connection has proper exponential backoff
- API calls go through type-safe transformers
- Clean separation of concerns
- Smaller, more readable screen components

---

## Step 5: Add Emoji Picker and Fix Text Contrast

Added an emoji picker component with search functionality (user doesn't have an emoji keyboard). Fixed multiple dark-on-dark contrast issues where text was unreadable.

**Commit (code):** 17318f0 — "Add emoji picker with search + fix dark-on-dark text contrast"

### What I did

1. Created `src/components/EmojiPicker.tsx`:
   - Dropdown-based picker with 70+ categorized emojis
   - Search by keyword (fire, rainbow, calm, pulse, etc.)
   - Categories: Lighting/Effects, Colors, Nature, Mood, Objects, Patterns, Power
   - Click to select, Escape to close
   - Highlights currently selected emoji

2. Updated `src/screens/Patterns.tsx`:
   - Replaced text input for icon with `<EmojiPicker />` component
   - Changed `handleIconChange` to accept string directly

3. Fixed text contrast issues in `src/index.css`:
   - `.node-row .fw-medium` → explicit `color: var(--text-primary)`
   - `.form-label` → explicit `color: var(--text-secondary)`
   - `.form-control, .form-select` → added `!important` for color overrides
   - `.form-control::placeholder` → uses `var(--text-muted)`
   - `.table > tbody > tr > td` → explicit `color: var(--text-primary)`
   - `.table > tbody > tr > td:first-child` → extra weight for node names

4. Added CSS for emoji picker:
   - `.emoji-picker-container` with relative positioning
   - `.emoji-picker-dropdown` absolute positioned with dark theme
   - `.emoji-grid` 8-column responsive grid
   - `.emoji-option` with hover scale and selected highlight

### Why

- User requested emoji picker because they don't have an emoji keyboard
- Dark-on-dark contrast was making node names hard to read
- Bootstrap's default text colors don't work well with custom dark theme

### What worked

- Keyword search works well (type "fire" → shows 🔥)
- Emoji categories cover typical lighting/effects use cases
- Dropdown closes on selection (good UX)
- Contrast fixes make all text readable

### What didn't work

- Initial attempt to use `onInput` event didn't trigger filtering fast enough
- Had to add `!important` to override Bootstrap's dark mode specificity

### What I learned

- Bootstrap's `.fw-medium` and other utility classes don't set color explicitly
- When using custom CSS variables, need to override with `!important` on form controls
- Preact's `onInput` vs `onChange` - use `onInput` for real-time filtering

### What was tricky to build

- Getting the dropdown positioning right (absolute inside relative)
- Ensuring search clears when dropdown closes
- Balancing emoji count (too few = missing options, too many = overwhelming)

### What warrants a second pair of eyes

1. Emoji selection - verify all relevant emojis for lighting presets are included
2. Contrast fixes - verify no regressions in other screens (Status, Settings)
3. Dropdown z-index - verify it doesn't get clipped by other elements

### What should be done in the future

- Consider adding emoji categories as tabs (lighting, colors, mood)
- Add keyboard navigation (arrow keys, Enter to select)
- Consider allowing custom emoji input as fallback

### Code review instructions

1. Open Patterns screen, click on a preset
2. Click the emoji button → verify dropdown appears
3. Type "rainbow" → verify only rainbow-related emojis show
4. Click an emoji → verify selection updates
5. Navigate to Nodes → verify node names are readable
6. Check form labels in Settings → verify labels are visible

### Technical details

**Emoji data structure:**
```typescript
interface EmojiData {
  emoji: string;
  keywords: string[];
}

const EMOJI_DATA: EmojiData[] = [
  { emoji: '✨', keywords: ['sparkles', 'magic', 'shine'] },
  { emoji: '🔥', keywords: ['fire', 'hot', 'flame', 'warm'] },
  // ... 70+ entries
];
```

**Search implementation:**
```typescript
const filteredEmojis = useMemo(() => {
  if (!search.trim()) return EMOJI_DATA;
  const searchLower = search.toLowerCase();
  return EMOJI_DATA.filter((item) =>
    item.keywords.some((kw) => kw.includes(searchLower)) ||
    item.emoji === search
  );
}, [search]);
```

**CSS contrast fix pattern:**
```css
/* Override Bootstrap with explicit colors + !important */
.form-control {
  background: var(--bg-tertiary) !important;
  color: var(--text-primary) !important;
}
```

---

## Step 6: Update Patterns to Match MLED/1 Protocol

The frontend pattern types were invented (solid, gradient, pulse) but don't match the actual MLED/1 wire protocol. Updated to use the real pattern types from `0049-NODE-PROTOCOL` design doc.

**Commit (code):** a9e7af6 — "Update patterns to match MLED/1 protocol (0049-NODE-PROTOCOL)"

### What I did

1. Updated `types.ts` with protocol-compliant types:
   - `PatternType = 'off' | 'rainbow' | 'chase' | 'breathing' | 'sparkle'`
   - Typed interfaces: `RainbowParams`, `ChaseParams`, `BreathingParams`, `SparkleParams`, `OffParams`
   - Each interface documents the wire format mapping (e.g., `data[0]` → `speed`)

2. Updated `store.ts` with new default presets:
   - Rainbow: 10 rpm, 100% saturation
   - Calm: breathing 4 bpm, blue
   - Magic: sparkle rainbow mode
   - Fire Chase: 3 trains, orange/red, fade tail
   - Pulse: fast breathing 12 bpm, white
   - Off: all LEDs off

3. Updated `Patterns.tsx` with full editors:
   - Created helper components: `SliderRow`, `ColorRow`, `SelectRow`
   - RAINBOW: speed (0-20 rpm), saturation (0-100%), spread (1-50)
   - CHASE: speed (1-255 LED/s), tail/gap lengths, trains, fg/bg colors, direction, fade_tail
   - BREATHING: speed (1-20 bpm), color, min/max brightness, curve (sine/linear/ease)
   - SPARKLE: speed, color, density, fade_speed, color_mode (fixed/random/rainbow), bg_color
   - OFF: simple message, no params

### Why

The frontend needs to generate valid wire payloads that nodes can decode. Using invented pattern types would cause nodes to ignore commands.

### What worked

- Type-safe param interfaces prevent invalid configurations
- Helper components (SliderRow, ColorRow) reduce code duplication
- Dropdown selects for enums (direction, curve, color_mode) are intuitive

### What didn't work

- Initial pattern editor had manual onChange handlers for each field — refactored to use `useParamChange` helper

### What I learned

- Always check the wire protocol before implementing UI patterns
- Fixed-size byte arrays (`data[12]`) require careful range limits in UI
- Enum mappings (0/1/2 → forward/reverse/bounce) should be explicit

### What was tricky to build

- Mapping protocol byte ranges to sensible slider limits (e.g., speed 0-20 vs 0-255)
- Ensuring default values match what nodes expect

### What warrants a second pair of eyes

1. Verify param byte positions match `protocol.h` exactly
2. Check range limits are correct (some params are 0-255, others 0-20, others 0-100)
3. Verify enum values match wire format (e.g., curve: 0=sine, 1=linear, 2=ease)

### What should be done in the future

- Add validation on param ranges before sending to API
- Add visual preview of patterns (canvas animation)
- Add pattern import/export as JSON

### Code review instructions

1. Compare `types.ts` PatternParams with `0049-NODE-PROTOCOL` section 6.12
2. Open Patterns screen, click each pattern type, verify all params are editable
3. Check that changing type resets params to sensible defaults

### Technical details

**Protocol mapping (from 0049-NODE-PROTOCOL):**
```
RAINBOW (type=1):
  data[0] = speed (0-20, rotations/min)
  data[1] = saturation (0-100)
  data[2] = spread_x10 (1-50)

CHASE (type=2):
  data[0] = speed (0-255, LED/s)
  data[1] = tail_len (1-255)
  data[2] = gap_len (0-255)
  data[3] = trains (1-255)
  data[4..6] = fg RGB
  data[7..9] = bg RGB
  data[10] = direction (0=fwd, 1=rev, 2=bounce)
  data[11] = fade_tail (0/1)

BREATHING (type=3):
  data[0] = speed (0-20, breaths/min)
  data[1..3] = color RGB
  data[4] = min_bri (0-255)
  data[5] = max_bri (0-255)
  data[6] = curve (0=sine, 1=linear, 2=ease)

SPARKLE (type=4):
  data[0] = speed (0-20, spawn rate)
  data[1..3] = color RGB
  data[4] = density_pct (0-100)
  data[5] = fade_speed (1-255)
  data[6] = color_mode (0=fixed, 1=random, 2=rainbow)
  data[7..9] = bg RGB
```

---

## Related

- Parent ticket: [0052-MLED-HOST-UI](../../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/index.md)
- Design doc: [02-design-go-host-http-api-preact-zustand-ui-embedded.md](../../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/02-design-go-host-http-api-preact-zustand-ui-embedded.md)
- UI screens spec: [fw-screens.md](../../../0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/sources/local/fw-screens.md)
