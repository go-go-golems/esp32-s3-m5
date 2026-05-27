# Changelog

## 2026-05-27

- Initial workspace created


## 2026-05-27

Ticket created. Wrote design doc (01-cores3-se-benchmark-firmware-design-and-intern-guide.md): 7 sections covering CoreS3 SE hardware, ESP32-S3 native WiFi vs ESP-Hosted comparison, TCP window tuning, iperf-optimized sdkconfig, benchmark matrix, implementation plan, expected results. Uploaded to reMarkable.


## 2026-05-27

Default STA benchmarks complete. Upload 3.7 Mbps (vs Tab5 4.2 Mbps), download 2.7 Mbps (vs Tab5 1.7 Mbps), ping 94ms (vs Tab5 106ms). CoreS3 has larger but fewer segments, more mid-range gaps. Surprising: native WiFi upload is SLOWER than ESP-Hosted on Tab5.


## 2026-05-27

Optimized STA benchmarks complete. Upload 16 Mbps (vs default 3.7 Mbps, 4.3x improvement), download 7.7 Mbps (vs default 2.7 Mbps, 2.9x improvement). TCP window + WiFi buffer tuning makes a dramatic difference on ESP32-S3.


## 2026-05-27

M5Dial benchmarks complete. ESP32-S3 with 8MB embedded flash, no PSRAM. Max payload 100KB (internal RAM limit). Optimized config: upload 11.5 Mbps, download 3.3 Mbps, ping 159ms (weaker signal at -50dBm). Three-device comparison data now available: Tab5 (P4+C6), CoreS3 (S3+PSRAM), M5Dial (S3 no PSRAM).


## 2026-05-27

Published ARTICLE: ESP32 WiFi Architecture Comparison (29 KB, textbook style). Uploaded to reMarkable at /ai/2026/05/27/0095. Key insight: default S3 upload is slower than ESP-Hosted (3.7 vs 4.2 Mbps); optimized S3 is 3.8x faster (16 vs 4.2 Mbps). TCP window is the primary control, not SDIO.


## 2026-05-27

Ticket closed

