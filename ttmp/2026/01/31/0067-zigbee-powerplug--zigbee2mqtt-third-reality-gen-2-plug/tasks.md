# Tasks

## TODO

- [x] Add tasks here

- [x] Update design doc: note Glazed handles command parsing (registering over Cobra)
- [x] Define custom Zigbee Glazed layer with broker/base-topic/tls/cafile/cert/key/qos/timeout flags
- [x] Ensure every command has LongDescription with examples in design doc
- [x] Revise implementation plan to include custom layer + LongDescription requirements
- [x] Add code organization rules: one directory per group, one file per verb, root.go per group
- [x] Scaffold zigctl module with root command, config loader, MQTT client, and custom Zigbee Glazed layer
- [ ] Implement bridge group with info, devices, permit-join commands (LongDescription examples)
- [ ] Implement listen group with state and raw listeners (LongDescription examples)
- [ ] Implement mqtt group with pub/sub helpers (LongDescription examples)
