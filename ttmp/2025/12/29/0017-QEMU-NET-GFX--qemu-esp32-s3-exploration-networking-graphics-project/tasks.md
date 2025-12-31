# Tasks

## TODO

- [ ] Confirm QEMU toolchain install path for ESP-IDF 5.4.1 in this environment (qemu-xtensa)
- [ ] Create a new ESP-IDF project folder for QEMU exploration (esp32s3 target, reproducible build wrapper)
- [ ] Establish a baseline: “Hello world + console IO” in QEMU (monitor attached)
- [ ] Networking probe #1: minimal TCP client/server behavior under QEMU (document what works)
- [ ] Networking probe #2: LwIP + esp_netif initialization differences under QEMU (document what breaks)
- [ ] Graphics probe #1: decide what “graphics in QEMU” means (options + constraints), then implement the smallest test
- [ ] Determine whether `idf_monitor` can reliably deliver input to UART-based REPL/apps in QEMU (link to existing bug ticket)
- [ ] Write a short “capabilities matrix” for QEMU: serial, flash/partitions, filesystem, time, networking, graphics

