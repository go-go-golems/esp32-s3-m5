# Tasks

## TODO

- [x] Define component API (start/stop, on-apply callback, effect status setter)
- [x] Create `components/mled_node` (CMake + Kconfig + include/src layout)
- [x] Move protocol/time/node sources into component (no behavior change)
- [x] Update `0049-xiao-esp32c6-mled-node` to consume component
- [x] Validate: `idf.py -C 0049-xiao-esp32c6-mled-node build`
- [x] Validate: flash + host `mled_ping.py` and `mled_smoke.py`
