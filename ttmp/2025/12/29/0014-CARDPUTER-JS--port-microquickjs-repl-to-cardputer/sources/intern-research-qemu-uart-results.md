Below is what I’d hand an intern as an “evidence + what to try next” write-up for **ticket 0014-CARDPUTER-JS** (QEMU UART RX dead) + **Cardputer console choices**.

---

## What’s most likely happening (high confidence)

### Finding 1: ESP-IDF *expects* “interactive UART” to work under QEMU

* **Source:** ESP-IDF “QEMU Emulator” guide for **ESP32-S3** says `idf.py qemu monitor` connects IDF Monitor to the emulated UART and that you can “interact with it.” ([Espressif Systems][1])
* **Why it matters:** Your symptom (“prompt prints, but no RX bytes arrive”) contradicts the intended dev loop, so we’re either hitting a known UART RX limitation/bug or a UART mapping/threshold mismatch.

### Finding 2: A known Espressif-QEMU UART RX failure mode is “RXFIFO timeout interrupt not implemented”

* **Source:** Espressif forum thread on QEMU UART RX/TX: “UART_RXFIFO_TOUT interrupt is not implemented in QEMU,” and without it “the software doesn’t get notified” when received bytes are **less than the RX threshold**. ([ESP32 Forum][2])
* **Why it matters:** This exactly matches “TX prints fine, but interactive input appears dead,” especially when you type short commands.

### Finding 3: Default “RX FIFO full” threshold behavior commonly shows up as **~120 bytes**

* **Source:** Another Espressif forum thread notes the RX FIFO interrupt only fires when the default FIFO FULL threshold is reached (**120 bytes**) and not on timeout. ([ESP32 Forum][3])
* **Interpretation for your case:** If QEMU doesn’t implement RX timeout interrupts (Finding 2), and your driver is effectively waiting for “FIFO full,” then **typing a few characters will never be delivered**—but blasting ≥120 bytes might suddenly “unstick” input.

---

## Concrete “try this next” (fast local validations)

### Test A (no firmware changes): prove it’s a “threshold / missing timeout IRQ” issue

1. Boot your REPL prompt in QEMU like you already do.
2. Using your **raw TCP UART harness** (or stdio path), **send a single burst of 200 bytes** (e.g., `A` repeated 200 times + newline).
3. Expected outcomes:

   * **If you suddenly see echoed bytes after a big burst:** this strongly supports “no RX timeout IRQ + high FIFO threshold” (Findings 2–3).
   * **If you still see nothing even with big bursts:** then it’s more likely UART mapping / wiring (see Test C).

### Test B (tiny firmware change): force RX FIFO “full” threshold to 1 byte under QEMU

ESP-IDF’s UART docs explicitly support configuring RX FIFO full/timeout interrupts via `uart_intr_config()` / `uart_enable_rx_intr()` and thresholds. ([Espressif Systems][4])

Patch idea (keep it minimal; you can `#ifdef` it to only affect QEMU builds):

* After `uart_driver_install()` for the UART you’re reading (likely `UART_NUM_0`):

  * Set `rxfifo_full_thresh = 1`
  * Enable `UART_INTR_RXFIFO_FULL` (and optionally `UART_INTR_RXFIFO_TOUT` even if QEMU ignores it)

If QEMU is missing RX timeout IRQ (Finding 2), the **key part is `rxfifo_full_thresh = 1`**, so a single character triggers delivery.

### Test C (mapping sanity): verify your REPL is reading from the same UART that QEMU exposes

The ESP-IDF QEMU flow connects IDF Monitor to “the emulated UART port.” ([Espressif Systems][1])
So confirm:

* Your REPL reads from **UART0** (not UART1/2).
* Your build config hasn’t moved console/stdin to a different transport in a way that bypasses the UART you think you’re using.

---

## Cardputer: what console/input path to implement (and why)

### Finding 4: Cardputer hardware supports USB Serial/JTAG (good for REPL)

* **Source:** M5Stack Cardputer docs list **USB OTG** and **USB Serial/JTAG** in specs. ([M5Stack Docs][5])
* **Source:** ESP-IDF USB Serial/JTAG Console guide says ESP32-S3 has a **bidirectional** serial console over USB Serial/JTAG, usable with IDF Monitor; it appears as `/dev/ttyACM*` on Linux. ([Espressif Systems][6])
* **Important detail:** The doc explicitly warns: if you want **input / REPL**, select `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`. ([Espressif Systems][6])

### Practical recommendation (for the “real device” dev loop)

* For Cardputer (on hardware), prefer **USB Serial/JTAG console** (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`) for interactive REPL. ([Espressif Systems][6])
* This gives you a solid “type commands, get replies” loop on `/dev/ttyACM*` without an external USB-UART bridge. ([Espressif Systems][6])

---

## Extra datapoint worth tracking (lower priority)

There’s an open Espressif-QEMU issue requesting keyboard-to-UART0 injection when the QEMU window is grabbed, which suggests input plumbing is an active topic in that repo. ([GitHub][7])
This doesn’t directly prove your `-serial` path is broken, but it’s consistent with “input UX / routing is not fully polished.”

---

## Suggested “intern report” conclusions (what to write in ticket 0015)

1. **Most likely root cause:** Espressif QEMU missing **UART RXFIFO timeout interrupt** → small RX bursts never get surfaced to IDF UART driver unless FIFO full threshold is reached. ([ESP32 Forum][2])

2. **Fastest workaround to unblock you:** set RX FIFO full threshold to **1 byte** in your QEMU build (or prove with a ≥120-byte burst test first). ([Espressif Systems][4])

3. **Long-term dev loop plan:** keep QEMU for boot/output + non-interactive tests, but use **Cardputer USB Serial/JTAG console** for real interactive REPL work. ([Espressif Systems][6])

If you want, paste the tiny UART init snippet you’re using in the REPL loop (just the UART install/config part), and I’ll point to the exact minimal place to drop the `rxfifo_full_thresh=1` change without touching the rest of your design.

[1]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/tools/qemu.html "QEMU Emulator - ESP32-S3 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[2]: https://esp32.com/viewtopic.php?t=18358&utm_source=chatgpt.com "QEMU: UART RX/TX"
[3]: https://esp32.com/viewtopic.php?t=25963&utm_source=chatgpt.com "UART timeout interrupt has no effect - ESP-IDF"
[4]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html "Universal Asynchronous Receiver/Transmitter (UART) - ESP32-S3 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[5]: https://docs.m5stack.com/en/core/Cardputer?utm_source=chatgpt.com "Cardputer - m5-docs"
[6]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html "USB Serial/JTAG Controller Console - ESP32-S3 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[7]: https://github.com/espressif/qemu/issues/128 "Keyboard input when qemu window is grabbed should be input to UART0 (QEMU-262) · Issue #128 · espressif/qemu · GitHub"
