# Changelog

## 2026-05-27

- Initial workspace created


## 2026-05-27

Ticket created. Wrote design doc (01-cores3-se-benchmark-firmware-design-and-intern-guide.md): 7 sections covering CoreS3 SE hardware, ESP32-S3 native WiFi vs ESP-Hosted comparison, TCP window tuning, iperf-optimized sdkconfig, benchmark matrix, implementation plan, expected results. Uploaded to reMarkable.


## 2026-05-27

Default STA benchmarks complete. Upload 3.7 Mbps (vs Tab5 4.2 Mbps), download 2.7 Mbps (vs Tab5 1.7 Mbps), ping 94ms (vs Tab5 106ms). CoreS3 has larger but fewer segments, more mid-range gaps. Surprising: native WiFi upload is SLOWER than ESP-Hosted on Tab5.


## 2026-05-27

Optimized STA benchmarks complete. Upload 16 Mbps (vs default 3.7 Mbps, 4.3x improvement), download 7.7 Mbps (vs default 2.7 Mbps, 2.9x improvement). TCP window + WiFi buffer tuning makes a dramatic difference on ESP32-S3.

