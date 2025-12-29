# Tasks

## TODO

- [ ] Reproduce + capture evidence: scope screenshot/video showing the “tx low” spike on both GPIO1/GPIO2 (include probe setup + grounding).
- [ ] Validate firmware sequencing hypothesis: determine whether the spike correlates with pin reconfiguration (safe reset + output enable) vs sustained drive.
- [ ] Distinguish coupling vs real drive: repeat with the inactive pin strapped via a resistor (e.g. 10k to GND or 3V3) and compare spike amplitude.
- [ ] If needed, implement a mitigation experiment in 0016: preload output level before enabling output (and document measured change).
- [ ] Relate captured evidence (images) to this ticket, and summarize conclusions in the analysis doc.

