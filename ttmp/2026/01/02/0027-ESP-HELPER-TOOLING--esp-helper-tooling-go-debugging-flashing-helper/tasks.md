# Tasks

## TODO

- [ ] Decide tool name + scope (repo-local vs shared)
- [ ] Define config format (IDF export path, default target, port policy)
- [ ] Implement `env doctor` (IDF_PATH/python/module diagnostics)
- [ ] Implement port enumeration + deterministic `--port auto`
- [ ] Implement `build` wrapper with target enforcement
- [ ] Implement `flash-monitor` wrapper (single-process, no serial contention)
- [ ] Implement optional `flash --stable-usb-jtag` (esptool + build/flash_args)
- [ ] Implement optional tmux session management (`tmux flash-monitor`)
- [ ] Add “procedures” scaffolding (timeouts, log capture, artifacts)
