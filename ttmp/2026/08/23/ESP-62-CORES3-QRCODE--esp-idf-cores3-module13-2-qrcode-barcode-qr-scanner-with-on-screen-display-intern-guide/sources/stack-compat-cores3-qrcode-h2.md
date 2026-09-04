# Stack compatibility: CoreS3 + Module13.2 QRCode (M145) + Module Gateway H2 (M141)

Source: M5Stack Stack Compatibility tool,
https://docs.m5stack.com/en/compatible_stack?host=K128&module=M145,M141
(captured 2026-08-23 via Playwright; the page is a JS app — static curl
returns no matrix.)

## Pin map (M5-Bus pin | CoreS3 | Module13.2 QRCode M1 | Module Gateway H2 M2)

```
PIN  CoreS3        Module13.2 QRCode (M1)   Module Gateway H2 (M2)
1    GND           /                        /
2    G10           /                        RXD            <- H2 UART RX
3    GND           /                        /
4    G8            /                        /
5    GND           /                        /
6    RST/EN        /                        /
7    G37           /                        SPI_MOSI       <- H2 SPI
8    G5            /                        BT_PRIORITY
9    G35           /                        SPI_MISO       <- H2 SPI
10   G9            /                        /
11   G36           /                        SPI_CLK        <- H2 SPI
12   3V3           /                        /
13   G44/RXD0      QR_UART_TX / NC          /              (USB-JTAG console)
14   G43/TXD0      QR_UART_RX / NC          /              (USB-JTAG console)
15   G18/PC_RX     QR_UART_TX / NC          G9             *** CONFLICT (G18)
16   G17/PC_TX     QR_UART_RX / NC          /
17   G12/In_SDA    I2C_SDA                  /              (expander 0x43)
18   G11/In_SCL    I2C_SCL                  /              (expander 0x43)
19   G2/PA_SDA     /                        /
20   G1/PA_SCL     /                        /
21   G6            QR_UART_RX / NC          BT_ACTIVE
22   G7            QR_UART_TX / NC          EN             <- H2 enable
23   G13/I2S_DOUT  QR_UART_RX / NC          SPI_CS         <- H2 SPI
24   G0/I2S_LRCK   /                        WL_ACTIVE
25   HPWR          HPWR                     /
26   G14/I2S_DIN   QR_UART_TX / NC          /
27   HPWR          HPWR                     /
28   5V            /                        /
29   HPWR          HPWR                     /
30   BAT           /                        /
```

## Verdict for the ESP-62 QRCode firmware

Our firmware (0118-cores3-qrcode-scanner) uses:
- UART1 on **G17 TX (pin 16) / G18 RX (pin 15)** — DIP-routed to the QR engine.
- I2C0 on **G12/G11 (pins 17/18)** — PI4IOE5V6408 @0x43.
- USB Serial/JTAG on **G43/G44 (pins 13/14)** — console + flash.

The Module Gateway H2 (M141) uses **G10, G37, G5, G35, G36, G6, G7, G13, G0, and G18** on the CoreS3.

**Conflicts with our QR scanner:**
- **pin 15 / G18 — CONFLICT.** Both the QR engine UART RX (our DIP route) and the H2's GPIO9 drive G18. If both are active they fight.
- pin 16 / G17 — H2 does NOT use it → our TX is fine.
- I2C (pins 17/18) — H2 does NOT use them → expander is SAFE.
- USB Serial/JTAG (pins 13/14) — H2 does NOT use them → console/flash SAFE.

**Conclusion:**
- Stacking the H2 under the QRCode module is *physically possible* (H2 is a Module-layer device), but for THIS firmware the H2's G18 use collides with the scanner's UART RX on the only clean DIP route (pins 15/16 → G18/G17).
- If you want the H2 in the stack too, either:
  1. Keep the H2 **disabled** (its `EN` on G7 held low / not initialized) so it doesn't drive G18, and keep the QR UART on pins 15/16; or
  2. Re-route the QR engine UART to a DIP pair the H2 doesn't touch — but every other QR UART DIP option (pins 13/14 = USB-JTAG console; pins 21/22 = H2 BT_ACTIVE/EN; pin 23 = H2 SPI_CS) also conflicts or is reserved. So option 1 (H2 disabled) is the practical one.
- For the scanner firmware alone (no H2), the current stack CoreS3 + Module13.2 QRCode is the supported, conflict-free configuration.
