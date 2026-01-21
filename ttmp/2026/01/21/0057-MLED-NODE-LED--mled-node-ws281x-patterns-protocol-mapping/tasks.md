# Tasks

## TODO

- [x] Relate core files to design doc (0049 node + 0044 led_task + protocol docs)
- [x] Copy/port `0044` WS281x engine into `0049` node project (no console required)
- [x] Implement `PatternConfig` → `led_pattern_cfg_t` mapping (RAINBOW/CHASE/BREATHING/SPARKLE)
- [x] Wire node cue apply to LED task (`led_task_send`) and report status in PONG
- [x] Add host test script support for all patterns (extend `tools/mled_smoke.py`)
- [x] Write host test playbook (flash + run python + expected visuals)
- [ ] Ask user to test; record results in diary
