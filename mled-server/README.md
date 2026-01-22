# mled-server (Go)

Host-side controller for the MLED/1 UDP multicast protocol with an HTTP + WebSocket API intended to back the exhibition UI in `fw-screens.md`.

## Quickstart (dev)

```bash
cd esp32-s3-m5/mled-server
go run ./cmd/mled-server serve --http-addr localhost:8765
```

## Endpoints

- `GET /api/nodes` — JSON array of nodes
- `POST /api/apply` — apply a pattern to a set of nodes (wire-level mapping uses `CUE_PREPARE` + `CUE_FIRE`)
- `GET /ws` — WebSocket events (`node.update`, `node.offline`, `apply.ack`, `error`)
