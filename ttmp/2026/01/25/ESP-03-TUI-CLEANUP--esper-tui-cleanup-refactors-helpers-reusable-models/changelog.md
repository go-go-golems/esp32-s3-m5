# Changelog

## 2026-01-25

- Initial workspace created
- Refactor: unified overlay routing under `appModel` in the nested `esper/` repo (commit `fcbcd29`), validated via `go test ./...` and tmux screenshot capture against the connected device
- Refactor: extracted a reusable selectable list core and migrated port picker / command palette / inspector event selection (commit `0ccf5df`), validated via `go test ./...` and the tmux screenshot harness (virtual PTY)
