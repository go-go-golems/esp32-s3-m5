# PaperS3 User Demo

User demo source code of [PaperS3](https://docs.m5stack.com/en/core/PaperS3).

## Build

### Fetch Dependencies

```bash
python ./fetch_repos.py
```

### Tool Chains

[ESP-IDF v5.3.3](https://docs.espressif.com/projects/esp-idf/en/v5.3.3/esp32s3/index.html)

### Build

```bash
idf.py build
```

### Flash

```bash
idf.py flash
```

## Acknowledgments

This project references the following open-source libraries and resources:

- https://github.com/m5stack/M5GFX.git
- https://github.com/m5stack/M5Unified.git
- https://github.com/Forairaaaaa/mooncake
- https://github.com/Forairaaaaa/mooncake_log


## ESP-50 FactoryTest runtime controls

This numbered copy preserves M5Stack FactoryTest tag V0.5 while adding a compile-time-gated, post-idle timing trace. It produces three source builds under exact ESP-IDF v5.3.3:

- `clean`: unpatched M5GFX 0.2.15;
- `off` / F1: trace-patched M5GFX with all hooks, arguments, counters, and ring storage compiled out;
- `trace` / F2: a 1024 × 48-byte fixed ring, dumped only after `boot_display_test()` completes and M5GFX reports idle.

The official merged release is F0 and remains separate. F0 has no internal ring.

From the repository root:

```bash
ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/23-prepare-factory-v0.5-trace-components.sh
ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/24-build-factory-v0.5-trace-variants.sh
ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/25-audit-factory-v0.5-trace-controls.py
```

No build or audit script flashes hardware. Physical runs use preregistered F0/F1/F2 ledgers and the guarded script `27-run-factory-comparison-flash.sh`.
