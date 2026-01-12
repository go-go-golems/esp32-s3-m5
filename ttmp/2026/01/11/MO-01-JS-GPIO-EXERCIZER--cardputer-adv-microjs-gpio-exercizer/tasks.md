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
