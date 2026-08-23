# Module Gateway H2 (M141) — pinmap & DIP analysis

Source: https://docs.m5stack.com/en/products/sku/M141 (Module Gateway H2 product page),
captured 2026-08-23 via Playwright. Doc page:
https://docs.m5stack.com/en/module/Module%20Gateway%20H2

## What it is

A **stackable Module-layer** board (ESP32-H2-MINI-1, RISC-V, Zigbee/Thread/Matter
IEEE 802.15.4 NCP). 8-bit DIP switch for "interface switching (UART/SPI) and
wireless function control". Used as a Thread/Zigbee border-router/NCP with an
M5 host (CoreS3, Core2, Basic, etc.).

## M5-Bus pinmap (H2 side)

```
PIN  LEFT          RIGHT      Notes
1    GND           RXD        pin2 RXD = H2 UART RX (to host TX)
3    GND
5    GND
7    SPI_MOSI      (pin8 BT_PRIORITY)
9    SPI_MISO
11   SPI_CLK       3V3
13   (empty)       (empty)    *** G43/G44 NOT used by H2 ***
15   G9            (empty)    *** H2 GPIO9 on M5-Bus pin 15 ***
17   (empty)
19   (empty)
21   BT_ACTIVE     H2-EN      (pin22 = H2 enable)
23   SPI_CS        WL_ACTIVE
25   HPWR
27   HPWR          5V
29   HPWR          BAT
```

## CoreS3-side mapping (from the H2 ↔ CoreS3 compatibility view)

```
M5-Bus pin  CoreS3 GPIO       H2 function
2           G10              RXD            (H2 UART RX)
7           G37              SPI_MOSI
8           G5               BT_PRIORITY
9           G35              SPI_MISO
11          G36              SPI_CLK
13          G44/RXD0          (empty)        <- USB-JTAG console (H2 does NOT use)
14          G43/TXD0          (empty)        <- USB-JTAG console (H2 does NOT use)
15          G18/PC_RX         G9             <- H2 GPIO9 = FIXED connection
16          G17/PC_TX         (empty)
17          G12/In_SDA        (empty)        <- I2C (H2 does NOT use)
18          G11/In_SCL        (empty)        <- I2C (H2 does NOT use)
21          G6               BT_ACTIVE
22          G7               H2-EN
23          G13/I2S_DOUT      SPI_CS
24          G0/I2S_LRCK       WL_ACTIVE
26          G14/I2S_DIN       (empty)
```

## DIP switches (user-confirmed on the physical board)

The H2 board exposes DIP switches for these CoreS3 GPIOs only:
**G35, G36, G37, G13, G5, G6, G0** (top→bottom on the board, mapped to H2
G1/G0/G3/G2/G5/G4/G12 on the H2 side).

### CRITICAL: G18 / H2-G9 is NOT on a DIP

The H2's connection on **M5-Bus pin 15 (CoreS3 G18 / H2 GPIO9)** is a
**fixed connection** — it is NOT one of the DIP-switchable pins. So you
CANNOT disconnect the H2 from G18 by flipping a DIP. The DIPs only cover
G35/36/37/13/5/6/0 (the SPI + BT/WL control lines).

This means: if the H2 is stacked, its GPIO9 is electrically tied to CoreS3
G18. If our QRCode scanner also routes its UART RX onto G18 (M5-Bus pin 15,
the pins 15/16 DIP option), the H2's G9 and the QR engine's TX both drive
the same CoreS3 input → conflict / contention.

## Verdict for the ESP-62 QRCode firmware + H2 stack

- The H2 does NOT use G43/G44 (pins 13/14), G17 (pin 16), or the I2C bus
  (pins 17/18). USB Serial/JTAG console + flash and the PI4IOE5V6408
  expander are all SAFE with the H2 stacked.
- The H2 DOES use G18 (pin 15, fixed, non-switchable). The QR engine's
  only clean UART route is pins 15/16 (G18 RX / G17 TX). So G18 collides
  and cannot be freed via an H2 DIP.
- Therefore, for a 3-layer CoreS3 + QRCode + H2 stack, the scanner UART
  cannot share G18 with the H2's G9. Options:
  1. **Do not stack the H2** for the scanner firmware (2-layer CoreS3 + QRCode
     is the supported config and what works now).
  2. **Keep the H2 but never drive G9** — leave the H2 un-powered/un-enabled
     (H2-EN on G7 low) so its GPIO9 is high-impedance. Risky: an un-powered
     H2's GPIO may still load the line; not guaranteed clean.
  3. **Re-route the QR UART to pins 13/14 (G43/G44)** — free of the H2, but
     G43/G44 are the CoreS3 USB Serial/JTAG console (TXD0/RXD0), so this
     sacrifices the USB console/flash path. Not recommended.
- Conclusion: the clean, supported configuration for THIS firmware is
  **CoreS3 + Module13.2 QRCode (2 layers)**. Adding the H2 forces a G18
  conflict that the H2's DIPs cannot resolve.
