# Ticket scripts

## Run server in tmux

Start:

```bash
SESSION=mled-server-test HTTP_ADDR=localhost:18765 DATA_DIR=/tmp/mled-server-test-var \
  ./tmux-run-mled-server.sh
```

Stop:

```bash
SESSION=mled-server-test ./tmux-stop-mled-server.sh
```

## E2E REST verb test

Assumes the server is already running:

```bash
SERVER_URL=http://localhost:18765 ./e2e-rest-verbs.sh
```

Or start a fresh tmux server, run tests, then stop it:

```bash
SESSION=mled-server-e2e HTTP_ADDR=localhost:18765 DATA_DIR=/tmp/mled-server-e2e-var \
  ./e2e-rest-verbs-tmux.sh
```

If you already have a server running that binds UDP `:4626`, set a different multicast port for the test server:

```bash
MCAST_PORT=14626 ./e2e-rest-verbs-tmux.sh
```

Set `KEEP_RUNNING=1` to keep the tmux session after tests:

```bash
KEEP_RUNNING=1 ./e2e-rest-verbs-tmux.sh
```

## Process sanity

List related processes:

```bash
./ps-mled-server.sh
```

Stop any stray `mled-server serve` processes (including `go run` wrappers):

```bash
./kill-mled-server-serve.sh
```
