# Tasks

## TODO

- [ ] Capture/attach full serial log around the crash (including ELF SHA)
- [ ] Confirm crash trigger: press `P` in demo-suite (B3) while host reads serial
- [ ] Identify root cause in code (expected: stack usage in `LGFXBase::createPng`)
- [ ] Fix: run PNG encode/send on a dedicated task with a large stack (or increase main task stack)
- [ ] Validate: host capture script writes a valid PNG and device stays responsive
- [ ] Document “why” and add prevention guidance (stack budgets / avoid heavy work in `main`)
