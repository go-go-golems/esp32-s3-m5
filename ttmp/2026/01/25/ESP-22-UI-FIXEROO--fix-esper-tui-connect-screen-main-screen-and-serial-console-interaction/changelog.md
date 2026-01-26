# Changelog

## 2026-01-25

- Initial workspace created


## 2026-01-25

Initial analysis complete: identified root causes (styles.go minimal styling, port_picker.go layout bugs), reviewed existing UX specs and screenshots, created detailed fix plan with 5 phases


## 2026-01-25

Deep dive complete: identified exact bugs in port_picker.go - (1) form field width math error causing wrapping (line 342,369), (2) PlaceCentered consuming all height causing footer truncation (line 261). Both bugs have clear root causes and fixes documented.


## 2026-01-25

Captured fresh screenshots using virtual PTY. Created visual analysis document with annotated screenshots. Key findings: NO log level colors (I/W/E all same gray), title shows raw path, no accent colors. Command palette and search are functional. Inspector needs events.


## 2026-01-25

Created style-to-component mapping (reference/01-style-to-component-mapping.md) with complete index of all styles, their definitions, and usage locations. Created monitor_view refactor proposal (design-doc/01-monitor-view-refactor-proposal.md) synthesizing ESP-14 and ESP-12. Updated visual analysis with code location annotations. Key discovery: log colors DO work (ANSI codes applied), screenshots strip them.


## 2026-01-25

Implemented monitor_view.go split and UI fixes: extracted ui_helpers.go, monitor_model.go, monitor_serial.go, monitor_inspector.go, monitor_sessionlog.go. Fixed port_picker.go layout (footer visibility, form field wrapping). Enhanced styles.go with modern color palette (rounded borders, primary purple accent, log level colors).

