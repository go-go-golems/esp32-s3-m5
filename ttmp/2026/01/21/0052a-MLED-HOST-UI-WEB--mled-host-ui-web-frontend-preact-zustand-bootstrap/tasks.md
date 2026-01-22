# Tasks

## TODO

- [x] Add tasks here

- [x] Initialize Vite + Preact + TypeScript project in mled-server/web/
- [x] Install and configure dependencies: preact, zustand, bootstrap, preact-router
- [x] Configure vite.config.ts with Preact preset and API proxy to Go backend (:8080)
- [x] Create Zustand store with: nodes, selectedNodeIds, presets, settings, connection state
- [x] Implement API client module (api.ts) for HTTP endpoints + SSE/EventSource for /api/events
- [x] Implement App shell with Bootstrap layout and 3-tab navigation (Nodes, Patterns, Status)
- [x] Implement Nodes screen: NodeList, NodeRow (checkbox, status, name, rssi, pattern)
- [x] Implement Nodes screen: PresetBar with quick-apply buttons + Select All
- [x] Implement Nodes screen: BrightnessControl slider + Apply to Selected button
- [x] Implement Patterns screen: PresetList with selectable items + New button
- [x] Implement Patterns screen: PresetEditor form (name, icon, type dropdown, dynamic params)
- [x] Implement Patterns screen: PatternParams components for each type (solid/rainbow/gradient/pulse/off)
- [x] Implement Patterns screen: Preview/Save/Delete actions
- [x] Implement Status screen: SystemBanner with health summary (Healthy/Degraded/Offline)
- [x] Implement Status screen: ProblemList with auto-detection (weak signal, offline nodes)
- [x] Implement Status screen: NodeTable with sortable columns (name, signal, uptime, pattern, status)
- [x] Implement Settings modal/page: network config (bind IP, multicast), timing, import/export presets
- [x] Implement SSE event handling for live node updates (node.update, node.offline, apply.ack)
- [x] Add default presets (Rainbow, Calm, Warm, Pulse, Off) with proper config structures
- [x] Add loading states, error handling, and toast notifications
- [ ] Test UI with mock API responses and SSE events
