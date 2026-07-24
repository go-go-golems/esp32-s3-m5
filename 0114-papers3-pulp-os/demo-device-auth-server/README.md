# PULP device-auth demo server

Development server for ticket `ESP-54-PULP-DEVICE-AUTH`. It embeds tiny-idp's strict Fosite provider in `DevMode`, stores identity/protocol state in SQLite, exposes bearer-protected REST APIs, and streams deterministic fake sensor data over an authenticated WebSocket.

No tiny-idp source modification is required. During development this module is resolved through the workspace at `/home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/go.work`.

## Security boundary

The provider rejects a non-loopback HTTP issuer, even in development mode. LAN hardware testing therefore uses a short-lived development CA and HTTPS/WSS. The CA certificate is embedded in the test firmware. Never commit `var/`, private keys, passwords, bearer tokens, or device codes.

## Start

```bash
export GOWORK=/home/manuel/workspaces/2026-07-23/tiny-idp-device-auth-eink/go.work
mkdir -p var
printf '%s\n' 'correct horse battery staple' > var/demo-password
chmod 600 var/demo-password
./scripts/generate-dev-cert.sh var/tls 192.168.0.39

go run ./cmd/pulp-auth-demo \
  --listen 0.0.0.0:8790 \
  --public-base-url https://192.168.0.39:8790 \
  --state-dir ./var/state \
  --demo-login alice \
  --demo-password-file ./var/demo-password \
  --tls-cert ./var/tls/server.crt \
  --tls-key ./var/tls/server.key \
  --log-level debug
```

Use tmux for the long-running server. Stop any existing listener first with `lsof-who -p 8790 -k`.

## Routes

| Route | Auth |
|---|---|
| `/idp/*` | tiny-idp provider/browser flow |
| `GET /healthz` | public |
| `GET /api/v1/me` | bearer + `demo.read` |
| `GET /api/v1/demo/fortune` | bearer + `demo.read` |
| `GET /api/v1/sensors/snapshot` | bearer + `sensors.read` |
| `GET /api/v1/sensors/ws` | bearer + `sensors.read`, WebSocket upgrade |

The device client is `pulp-papers3`, requests `openid profile demo.read sensors.read`, and uses resource indicator `https://192.168.0.39:8790/api` for this workstation.

## Validation

```bash
gofmt -w .
go test ./...
go test -race ./...
go vet ./...
go build ./...
```

Use `curl --cacert var/tls/ca.crt` for host requests. Never add `-k` to reusable acceptance scripts; doing so would hide certificate mistakes that the firmware must reject.
