# Tasks

## TODO

- [x] Move JS API design doc into ticket and update frontmatter + plan
- [x] Create zigctl jsruntime module skeleton (zigctl/pkg/jsruntime/zigctlmod with Doc/Loader)
- [x] Add go-go-goja dependency to zigctl/go.mod
- [x] Implement config parsing + MQTT connect wrapper (zigctl/pkg/zigbee)
- [x] Add core API methods: bridgeInfo, devices, permitJoin, publish, request
- [x] Add streaming/watch support (safe JS-friendly API)
- [x] Create zigctl jsruntime runtime builder and register module
- [x] Add zigctl js command group (run, repl) to execute scripts
- [x] Add JS examples in zigctl/testdata (join/watch, plug control)
- [x] Add temporary go.mod replace for local go-go-goja (not needed; module version works)
- [x] Add smoke test script / runbook notes for manual validation
- [ ] Update doc references + diary entries, then commit changes
