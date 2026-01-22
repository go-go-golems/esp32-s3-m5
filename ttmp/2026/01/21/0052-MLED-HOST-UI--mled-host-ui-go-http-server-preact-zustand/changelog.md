# Changelog

## 2026-01-21

- Initial workspace created.
- f1ca094: Implemented Glazed CLI bootstrap (logging + registration), REST client verbs (apply/preset/settings), SSE `GET /api/events`, and added tmux + e2e scripts + playbook.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/cmd/mled-server/main.go — Centralized Glazed registration and logging init
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/httpapi/sse.go — Server-Sent Events stream used by the UI
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/restclient/client.go — REST client backing new CLI verbs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/scripts/e2e-rest-verbs-tmux.sh — Repeatable tmux-backed end-to-end validation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/playbook/01-playbook-test-patterns-on-end-device.md — Pattern testing workflow on real devices
