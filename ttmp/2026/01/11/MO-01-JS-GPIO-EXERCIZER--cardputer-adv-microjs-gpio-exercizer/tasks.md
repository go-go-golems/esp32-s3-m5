# Tasks

## TODO

- [x] Analyze existing ESP32-S3 firmware examples for GPIO/protocol signal generation + REPL patterns
- [x] Brainstorm JS APIs + runtime architecture for protocol generation and scheduling
- [x] Define implementation plan: MicroQuickJS integration + GPIO/I2C/UART/RMT modules
- [x] Create C++ components for ControlPlane, GpioEngine, and I2cEngine
- [x] Create firmware target for JS GPIO exercizer (new esp32-s3-m5 project)
- [x] Implement control-plane component + GPIO engine (timer-based)
- [x] Implement I2C engine component + transaction queue
- [x] Add JS bindings for GPIO + I2C commands
- [x] Wire app_main to start engines + REPL; validate basic flows
- [x] Implement multi-channel GPIO engine for concurrent pins
- [x] Add gpio.setMany + stop(pin) JS bindings and regenerate stdlib
- [x] Rebuild exercizer firmware after multi-pin changes
- [x] (2) Define JS protocol helper APIs (i2c.scan/readReg/writeReg, gpio burst if needed)
- [x] (2) Add I2C scan control-plane event + engine implementation (probe range, log hits)
- [x] (2) Implement JS helpers: i2c.scan/readReg/writeReg (+ wiring in stdlib generator)
- [x] (2) Regenerate stdlib/atoms + sync into 0039 firmware
- [ ] (2) Validate protocol helpers in REPL (scan + reg read/write)
- [ ] (3) Design status/query API for active GPIO/I2C state (JS shape + control-plane transport)
- [ ] (3) Implement status/query responses (engine snapshots + JS return objects)
- [ ] (3) Validate status/query output in REPL
- [ ] (1) Evaluate LEDC for multi-pin square wave backend + limits
- [ ] (1) Prototype RMT backend for high-frequency pulse/burst patterns
- [ ] (1) Integrate LEDC/RMT backends into GPIO engine + JS selection
- [ ] (UART) Define JS UART API + framing options (uart.config/tx/txHex/break)
- [ ] (UART) Add control-plane types for UART config + tx + break
- [ ] (UART) Implement UART engine component (driver init, pin mux, tx queue)
- [ ] (UART) Implement JS helpers + stdlib generator hooks
- [ ] (UART) Regenerate stdlib/atoms + sync into 0039 firmware
- [ ] (UART) Wire UART engine into app_main control task
- [ ] (UART) Validate UART output on scope/logic analyzer (baud/format/break)
- [x] (2) Add i2c.deconfig + allow reconfig in engine
- [x] Switch i2c.config to object-only API (sda/scl/addr/hz/port)
- [ ] Add I2C pull enable configuration
