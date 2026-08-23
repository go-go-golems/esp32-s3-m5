# QR UART on G13/G14 with the H2 stacked — authoritative conclusion

Resolved using the H2 schematic (Sch_Module-Gateway_H2_v0.4.pdf, saved in
sources/h2/) and the user's photo of the physical DIP block.

## The H2 DIP switches are disconnect switches (not a mode selector)

The H2 has **6 DIPs** that disconnect the H2 from specific CoreS3 GPIOs:
G35, G36, G37, **G13**, G5, G6, G0. There is **no SPI/UART mode DIP** — and
none is needed. Each DIP to NC electrically disconnects the H2 from that
M5-Bus line.

H2 signal → CoreS3 GPIO (DIP-switchable):
- SPI_MOSI → G37
- SPI_MISO → G35
- SPI_CLK  → G36
- SPI_CS   → **G13**  ← switchable
- BT_PRIORITY → G5
- BT_ACTIVE   → G6
- WL_ACTIVE   → G0

Fixed (NOT on a DIP):
- H2 G9 → **G18** (M5-Bus pin 15) — fixed connection

## Why G13/G14 works for the scanner

QRCode (M145) UART DIP-routable pair: **pin 23 (G13) = QR_RX, pin 26 (G14) = QR_TX**.
So on the CoreS3: UART1 **TX = G13** (→ engine RX), **RX = G14** (← engine TX).

Conflict check:
- G13: H2 SPI_CS is on G13, BUT G13 is on a DIP → set H2 G13 DIP to NC →
  H2 disconnected → G13 free for CoreS3 UART TX. ✅ (No "tie CS low" needed;
  that wouldn't help anyway since G13 is bidirectional and the H2 drives it.)
- G14: H2 does not use pin 26 (G14) → free for UART RX. ✅
- G18: H2 G9 is fixed on G18, BUT the scanner no longer uses G18 (moved to
  G13/G14) → no conflict. ✅
- USB Serial/JTAG console (G43/G44) and I2C expander (G12/G11): untouched. ✅

## Configuration for a 3-layer CoreS3 + QRCode + H2 stack

1. H2 DIP: set **G13 to NC** (disconnect H2 SPI_CS from G13). Optionally also
   NC the other SPI pins (G35/G36/G37) if not using H2 SPI to the host.
2. QRCode DIP: route **QR_RX → pin 23 (G13), QR_TX → pin 26 (G14)**.
3. Firmware (0118-cores3-qrcode-scanner): change UART1 pins to
   **TX = G13, RX = G14** (`qr_module.h` kUartTx/kUartRx), rebuild + reflash.
4. G18 stays connected to the H2's G9 only — no scanner on G18.

## When this is fine / when it isn't

- Fine: H2 runs **standalone** (own Thread/Zigbee app via its ESP32-H2 +
  downloader), or H2 talks to the CoreS3 over **UART** (ESP-IDF Zigbee/Thread
  NCP supports a UART/SLIP transport; H2 RXD is on G10).
- Not fine: if you need the H2 as an **SPI NCP** to the CoreS3, G13 (SPI_CS)
  can't be freed. Use UART NCP mode instead, which frees all SPI pins.

## Note on the earlier "G18 fixed" conclusion

The M5-Bus table in the product PinMap lists pin 15 as `G9` (fixed), which is
correct — G18/G9 is NOT DIP-switchable. The earlier 2-layer analysis held that
G18 was therefore unavoidable. That stands. The G13/G14 route **sidesteps**
G18 entirely by moving the scanner off it, and G13 (unlike G18) IS on a DIP,
so the H2 can be disconnected from G13. This is the clean 3-layer solution.
