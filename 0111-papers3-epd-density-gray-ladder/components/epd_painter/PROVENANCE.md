# EPD_Painter component provenance

- Upstream repository: https://github.com/tonywestonuk/EPD_Painter
- Upstream commit: `753c521da8aef59756df07c1a4eb88f1c64c8227`
- Ticket snapshot: `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/epd-painter-753c521da8aef59756df07c1a4eb88f1c64c8227`
- Local patch: `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/patches/11-epd-painter-pure-idf-hardening.patch`
- Local patch SHA-256: `89e34a7f24060763c3f38aae7d4aaceeb8773e112256f1d21200b4a11fd1557b`
- Build mode: pure ESP-IDF; M5PaperS3 preset; automatic boot/shutdown controller excluded

Run `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/11-prepare-epd-painter-control.sh` from any directory to reconstruct this component. The script verifies the ticket snapshot, applies the patch with zero fuzz, and proves that `EPD_Painter_presets.h` remains byte-identical to upstream.
