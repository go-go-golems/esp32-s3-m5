Below is a “debugging book + playbook” that’s tailored to your exact pipeline (LovyanGFX `Panel_EPD` + ESP-IDF `esp_lcd` I80) and to the specific symptom: **vertical line artifacts around quick draws + new ghosting when moving from ESP‑IDF 5.3.x → 5.4.x (and later 5.5.x)**.

I’m going to do three things:

1. Build a **mental model** of what’s being timed and what’s being driven (so you can reason from symptoms → causes).
2. Give you a **ranked list of “most likely differences”** between 5.3 and 5.4 that map directly to your code.
3. Provide a **step-by-step investigation playbook** (fast triage → deeper instrumentation → hard proofs with a logic analyzer/scope), with “if you see X, do Y”.

---

## 1) What you’re driving on PaperS3 (why it’s unusually sensitive)

PaperS3 is a **4.7" 960×540 e‑paper** unit.
The ED047TC1-style glass is a “raw panel” that depends heavily on **panel-specific waveforms (LUT / waveform file)**. The ED047TC1 datasheet explicitly notes it is driven by a waveform file (“the display is driven by waveform file…”).
And in practice, people *do* mix waveforms (e.g., using ED047TC2 waveforms on ED047TC1 hardware) to get better results, which tells you the waveform choice materially affects ghosting/banding.

So the “three buckets” you wrote are exactly the right top-level taxonomy:

1. waveform / refresh-mode mismatch
2. power sag during refresh
3. framebuffer / transport corruption (timing/PSRAM/DMA/cache)

The tricky part is: **IDF upgrades can move you between buckets without changing your application code**, because they can change:

* I80 timing (pclk generation / edges / chip-select framing)
* DMA behavior and buffering
* PSRAM/cache behavior and default configs
* GPIO init behavior (OD vs push-pull, drive strength, etc.)

---

## 2) How your pipeline actually works (and where “vertical stripes” can be born)

### 2.1 High-level dataflow in your code

You effectively have three stages:

1. **App drawing** writes grayscale into `_buf` (PSRAM).
2. `Panel_EPD::task_update` converts the changed region into per-step state in `_step_framebuf` (PSRAM).
3. `blit_dmabuf()` converts the step states into a **scanline payload** in `_dma_bufs[y&1]` (internal DMA-capable RAM), then `esp_lcd_panel_io_tx_color()` pushes that out over I80.

That last detail matters a lot: **your DMA buffer is internal SRAM (MALLOC_CAP_DMA)**, so *the common “DMA reads stale PSRAM” failure mode only applies if you ever feed `esp_lcd_panel_io_tx_color()` a PSRAM buffer directly.* Your research about PSRAM DMA coherency is still valid (and shows up in IDF issues), but in *this* driver it’s more about:

* CPU reading PSRAM correctly/consistently
* cache coherency across cores for `_buf`
* timing of the I80 peripheral vs your GPIO strobes (XLE/CKV/etc.)

(There *are* real ESP-IDF issues where `esp_lcd_panel_io_tx_color()` misbehaves with PSRAM buffers.  It’s just not the first place I’d look given your current “bounce into DMA SRAM” design.)

### 2.2 Why “vertical line artifacts around quick draws” screams “scanline framing / latch timing”

If you see **vertical line(s)** (not random dots), that typically implies something like:

* A column group is being latched wrong (XLE timing relative to XCL)
* Some bytes are missing/duplicated (FIFO underrun or CS framing change)
* A data bit line is weak/stuck (signal integrity / drive / OD mode)
* A “line start” signal (XSTL/CS) timing changed

These show up most strongly during “quick draws” because you’re doing *partial update patterns* and you’re repeatedly exercising the critical timing edges (many small refresh sequences rather than one long, forgiving full refresh).

---

## 3) The biggest “IDF 5.3 → 5.4 difference” **in your specific code** (and why I’d test it first)

### 3.1 ⚠️ You are using `dc_gpio_num` as your **power pin**

In `Bus_EPD::init()` you have:

```cpp
bus_config.dc_gpio_num = (gpio_num_t)_config.pin_pwr; // <= dummy setting
```

This is a giant red flag.

In the Espressif LCD API, the D/C line is normally **owned by the peripheral** (it flips command vs data). The API explicitly allows D/C to be “not controlled by LCD peripheral” by setting it to **-1** (then you control it yourself).

If ESP-IDF 5.4 changed *anything* about how/when it touches D/C (initialization default level, idle level, toggling at the start/end of a transfer, handling of cmd = -1), then you are literally letting the I80 driver **toggle your EPD power enable pin** during scanline transfers.

That would produce exactly what you describe:

* “around quick draws” (lots of short transfers → lots of toggles)
* “weird ghosting” (HV rails / source driver not stable during waveform steps)
* “vertical artifacts” (some scanlines get partially driven / mis-latched)

Even if the D/C pin is “mostly stable”, a brief low pulse on a power-enable can:

* sag the boost rails
* reset/disable the source driver output stage
* distort the analog levels that create grayscale

This is the first thing I would change before touching DMA/PSRAM.

### 3.2 Immediate experiment (fast, decisive)

**Change `dc_gpio_num` so it can never touch your power pin.**

Preferred:

* If allowed in your IDF version: `bus_config.dc_gpio_num = -1;` (per the API pattern).
* Otherwise: route it to a real unused GPIO (not connected), and **remove any shared pin usage**.

Then rerun the exact same “quick draw” scenario that triggers artifacts.

If artifacts disappear or dramatically reduce → you’ve basically proven the regression cause is “ESP-IDF now drives D/C in a way it didn’t before (or differently), and your dummy mapping is harming you.”

---

## 4) “Explain I80” in *your* EPD context (what pins/waveforms mean)

### 4.1 What I80 is doing here

The I80 (8080-style) peripheral is acting like a **hardware-timed parallel shifter**:

* It outputs bytes on D0..D7
* It toggles WR at a fixed `pclk_hz`
* It asserts CS for the transfer window

Espressif’s I80 docs explicitly warn that if `pclk_hz` is too high, you can get **flickering** if DMA bandwidth can’t keep up.
In your case, that general point maps to: if the peripheral or DMA path ever starves, the panel will see bad byte timing.

### 4.2 How that maps to ED047TC1 pins

The ED047TC1 interface names (from the datasheet) include:

* **XCL**: shift clock (you map this to I80 WR/pclk)
* **D0..D7**: data bus (I80 data lines)
* **XLE**: latch enable (you toggle this manually after a line transfer)
* **XSTL**: “start” / line framing (you map this to I80 CS)
* **XOE**: output enable (you hold this on while driving)
* **CKV, SPV**: gate driver timing (row scan control)

So:

* I80 handles **XCL + D0..D7 + XSTL(CS)** with stable precise timing.
* Your GPIO code handles **XLE + CKV + SPV + XOE + PWR**.

### 4.3 How the “waveforms” drive particles

E‑ink pixels are electrophoretic microcapsules with charged black/white particles. Grayscale is not “analog voltage once”; it’s created by **time + polarity sequences**:

* Driving “to black” applies a field that moves black particles to the front.
* Driving “to white” applies the opposite field.
* Doing “no-op” holds or does a mild balancing pulse.

Your LUT comments match this:

* `1 == to black`
* `2 == to white`
* `3 == no operation`
* `0 == end of data`

So the scanline payload bits ultimately decide whether, for that pixel *in that frame step*, the source driver outputs a black-driving or white-driving polarity (or no-op).

If timing/power is wrong, those pulses don’t match what the LUT intended → ghosting, streaks, or banding.

---

## 5) ASCII timing diagrams (generated, not hand-waved)

These diagrams are based on:

* your actual `beginTransaction()` microsecond delays, and
* a scanline transfer whose length and pclk are parameterized.

### 5.1 Code that generates the diagrams

```python
import math
import numpy as np

def render_wave(events, t_end, dt, initial=0, hi='‾', lo='_'):
    events = sorted(events, key=lambda x: x[0])
    times = np.arange(0, t_end + 1e-9, dt)
    out, state, idx = [], initial, 0
    for t in times:
        while idx < len(events) and events[idx][0] <= t + 1e-9:
            state = events[idx][1]
            idx += 1
        out.append(hi if state else lo)
    return ''.join(out), times

def render_burst(t_end, dt, start, end, char='∿', lo='_'):
    times = np.arange(0, t_end + 1e-9, dt)
    return ''.join(char if (start <= t < end) else lo for t in times), times

def time_axis(times, major=5.0):
    s = [' '] * len(times)
    for i, t in enumerate(times):
        if abs((t / major) - round(t / major)) < 1e-9:
            label = str(int(round(t)))
            for j, ch in enumerate(label):
                if i + j < len(s):
                    s[i + j] = ch
    return ''.join(s)

# ---- beginTransaction() from your code ----
events_spv = [(0,0), (5,1)]  # SPV low, then high
events_ckv = [(1,0), (4,1), (8,0), (11,1), (14,0), (17,1), (20,0), (23,1)]

spv, t_bt = render_wave(events_spv, t_end=24, dt=1, initial=1)
ckv, _    = render_wave(events_ckv, t_end=24, dt=1, initial=1)

print("beginTransaction() timing (dt=1us):")
print("t:   " + time_axis(t_bt, major=5))
print("SPV: " + spv)
print("CKV: " + ckv)
print()

# ---- scanline transfer example ----
pclk_hz = 20_000_000
length_bytes = 240
t_line = length_bytes / pclk_hz * 1e6   # us
t_end  = math.ceil(t_line + 5)

cs, t_sc  = render_wave([(0,0), (t_line,1)], t_end=t_end, dt=1, initial=1)  # active low
xle, _    = render_wave([(0,0), (t_line,1)], t_end=t_end, dt=1, initial=1)
ckv2, _   = render_wave([(0,1), (t_line,0)], t_end=t_end, dt=1, initial=0)
xcl, _    = render_burst(t_end=t_end, dt=1, start=0, end=t_line, char='∿', lo='_')

print(f"One scanline transfer example: pclk={pclk_hz/1e6:.1f} MHz, bytes={length_bytes}, ideal time={t_line:.2f} us")
print("t:     " + time_axis(t_sc, major=5))
print("CS:    " + cs  + "   (XSTL, active LOW)")
print("XCL:   " + xcl + f"   (∿ = XCL toggling at pclk for {length_bytes} cycles)")
print("XLE:   " + xle + "   (latch, HIGH after last byte)")
print("CKV:   " + ckv2+ "   (held HIGH during transfer)")
```

### 5.2 Output you should expect (example)

```
beginTransaction() timing (dt=1us):
t:   0    5    10   15   20
SPV: _____‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
CKV: ‾___‾‾‾‾____‾‾___‾‾___‾‾

One scanline transfer example: pclk=20.0 MHz, bytes=240, ideal time=12.00 us
t:     0    5    10   15
CS:    ___________‾‾‾‾‾   (XSTL, active LOW)
XCL:   ∿∿∿∿∿∿∿∿∿∿∿∿_____   (∿ = XCL toggling burst)
XLE:   ___________‾‾‾‾‾   (latch, HIGH after last byte)
CKV:   ‾‾‾‾‾‾‾‾‾‾‾‾_____   (held HIGH during transfer)
```

These are “accurate” at the level that matters for debugging:

* **edge order** and **duration** of your GPIO-driven signals
* **transfer window** duration from `bytes/pclk`

What you verify on the logic analyzer is: does the actual system match this ordering (especially XLE relative to the last XCL edge)?

---

## 6) The playbook: isolate which bucket you’re in (fast → deep)

### 6.1 5-minute triage (do this first, in this order)

#### Step A — Force “quality / full refresh only”

Temporarily:

* use only `epd_quality` (disable `fast` / `fastest`)
* do a full-screen hard clear (multiple black/white cycles)

If **ghosting/stripes vanish** on full refresh but come back only in fast partials → you’re mostly in **bucket #1 (waveform/mode)**.
If artifacts persist even on full refresh → likely **bucket #2 or #3**.

Waveform sensitivity is not hypothetical on this panel class.

#### Step B — Eliminate power as a variable

E‑paper driving rails are “bursty” and sensitive. Run on:

* solid USB power + known-good cable
* fully charged battery (then compare)

If artifacts correlate with supply changes → bucket #2.

#### Step C — **Fix the D/C to PWR pin conflict**

This is the biggest “5.3→5.4 regression amplifier” in your code.

* Set `dc_gpio_num = -1` (if allowed) or to a harmless unused pin.
* Ensure `_config.pin_pwr` is controlled only by your `powerControl()` GPIO code.

If this fixes it, you’re done (root cause found).

---

## 7) Deep dive: diagnosing bucket #3 (transport / timing / PSRAM / DMA / cache)

### 7.1 Bucket #3 “signature table” (what the artifact *looks like* matters)

| Symptom signature                                        | Most likely cause                                      | Why                                                          |
| -------------------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------------------ |
| Vertical lines at constant X positions, repeatable       | data bit integrity or bus framing (CS/XLE/XCL)         | single-bit or byte-lane issues create column-locked patterns |
| Vertical artifacts *only around recently updated region* | latch timing / partial update + mis-latched line start | partial update exercises scanline sequence more often        |
| Large blocky patterns, sometimes “stripy blocks”         | PSRAM read errors or cache coherency                   | memory corruption often appears as repeating blocks/strides  |
| Ghosting increasing with repeated partial updates        | waveform choice / missing erase stage / power sag      | e‑paper needs balancing; sag distorts grayscale pulses       |

### 7.2 Level 1: prove or eliminate “I80 timing too aggressive”

Espressif’s I80 docs explicitly note that high `pclk_hz` can lead to flickering if the DMA bandwidth can’t keep up.
Even though you bounce into SRAM DMA buffers, you can still be “timing marginal” on:

* GPIO edge rate / drive strength
* panel latch sensitivity
* CS framing width

**Experiment:** cut `_config.bus_speed` in half (or to 1–2 MHz) and rerun quick draws.

* If artifacts disappear → you are timing/signal-integrity marginal.
* Then increase stepwise until you find the threshold.

### 7.3 Level 2: hard-proof the timing with a logic analyzer

Hook a logic analyzer to at least:

* XCL (WR/pclk)
* D0 (and ideally several D lines)
* XSTL (CS)
* XLE
* CKV
* PWR (if possible)

**Compare 5.3 build vs 5.4 build** on the same test pattern.

What you’re looking for:

1. Does **PWR twitch** during scanline bursts in 5.4?
   If yes → that’s the D/C pin issue.

2. Does **XLE rise before the last XCL edge** (or too close)?
   If yes → callback timing or transfer-complete semantics changed.

3. Did **CS polarity or CS timing** change (active level, asserted earlier/later)?
   CS is your XSTL; if it frames incorrectly, the panel shifts data into the wrong place.

4. Is **XCL frequency actually different** between the builds even if `pclk_hz` is “the same”?
   Clock divider changes can make “same config” produce slightly different real frequency.

### 7.4 Level 3: eliminate cache-coherency as a variable

Your own comment warns: *if task core differs from main core, PSRAM cache sync matters.*

Two decisive experiments:

**Experiment 1: pin task_update to same core as drawing**
Make `_config_detail.task_pinned_core` equal to the core you draw on.
If artifacts vanish → it’s a cache coherency / writeback / cross-core visibility issue.

**Experiment 2: brute-force writeback**
In `display()`, instead of partial writeback:

* writeback the **entire** `_buf` (slow, but decisive)

If that fixes artifacts → your current `cacheWriteBack()` range is insufficient under the 5.4 cache model.

(Your current `cacheWriteBack(&_buf[y * panel_width >> 1], h * panel_width >> 1)` uses the **function args** `y/h`, not the computed merged `_range_mod` rectangle, so it can miss some dirty lines if `_range_mod` spans more than the call args.)

### 7.5 Level 4: PSRAM margin tests

IDF upgrades can alter PSRAM defaults (speed, timing, cache behavior). Espressif’s external RAM guide discusses cache/PSRAM constraints and DMA-related limitations (and flash-cache interactions).

Do two quick stress toggles in `menuconfig`:

* Lower PSRAM speed (e.g., 80 → 40 MHz)
* Disable “aggressive” PSRAM features / try conservative mode

If artifacts drop sharply → you’re likely looking at external RAM margin issues.

---

## 8) “Waveform bucket” playbook (bucket #1)

If you confirm “full refresh clean / fast partial dirty”, do this:

1. **Disable `fastest` entirely**; run only `quality` and `text`.
2. If text mode is better than fast: that’s consistent with the “eraser stage vs direct drive” logic in your code.
3. Consider panel-specific waveform tuning. This panel class really does vary; even community work uses “neighbor” waveforms (ED047TC2 on ED047TC1).

---

## 9) “Power bucket” playbook (bucket #2)

If you suspect sag:

1. Put a scope on:

* 3.3 V rail
* the panel power enable (PWR)
* if accessible, the boost rails (depends on board)

2. Compare the waveform during:

* full refresh
* rapid partial refresh

3. Any dip that aligns with:

* CS asserted
* XCL burst start
* XLE toggle
  …is a smoking gun.

Also: fixing the D/C-to-PWR conflict can “magically” fix power bucket symptoms because it prevents inadvertent toggles.

---

## 10) Versioning reality check (why “works on 5.3.x” is a real clue)

M5Stack’s PaperS3 UserDemo explicitly pins ESP-IDF v5.3.3 in its setup instructions.
That strongly suggests the vendor validated the e‑paper pipeline on 5.3.x, and 5.4+ is outside their regression matrix.

Also: as of Espressif’s public tags visible now, I see v5.4.3 and v5.5.2 listed (not v5.4.7).
So if you’re seeing “5.4.7”, it may be:

* a downstream packaging version string (PlatformIO / board support package),
* or a patched fork.

Either way: the “compare logic traces” and “D/C pin conflict” approach still applies.

---

## 11) A practical “debugging harness” you can keep for 5.5 and beyond

When you upgrade IDF, run these as a regression checklist:

### A) Logic analyzer “golden trace”

Capture one “good” scanline in 5.3:

* CS duration
* XCL frequency and duty
* XLE rise time relative to last XCL
* CKV timing

Then capture same in 5.4/5.5 and diff.

### B) Stress patterns (to fingerprint bit/bus issues)

* Full white, full black
* Alternating bytes: 0xAA / 0x55 patterns across the line
* Single-bit walking patterns (to detect a weak Dn line)

If one data bit is marginal, the pattern test will make the stripe periodicity obvious.

### C) Bandwidth contention toggle

* Run with Wi‑Fi off vs on
* Run with logging off vs verbose logging
* Run single-core vs dual-core

If artifacts correlate with contention, you know you’re in timing/bandwidth land.

---

## 12) If I had to bet: top 3 root causes, in order

1. **D/C pin conflict with PWR (`dc_gpio_num = pin_pwr`)** becoming harmful due to subtle driver behavior differences in 5.4+. This is the “surgical” fix and the quickest proof.
2. **I80 timing margin** (pclk generation or CS timing changed enough to violate your panel’s comfortable margin). Drop `pclk_hz` and see.
3. **Cross-core PSRAM cache coherency** getting stricter (your current writeback range or scheduling assumptions no longer hold consistently).

---

### If you want one “do this right now” patch set (debug mode)

1. Set `dc_gpio_num` to `-1` or a harmless unused pin.
2. Set `trans_queue_depth = 1` temporarily (reduce internal pipeline complexity).
3. Drop `pclk_hz` by 4×.
4. Force full-buffer cache writeback for one test run (slow but decisive).
5. Compare logic traces 5.3 vs 5.4.

---

If you paste your actual `bus_speed` (`pclk_hz`), which GPIO numbers you’re using for D0..D7/WR/CS/XLE/CKV/SPV/PWR, and whether your update task is pinned to core 0 or 1, I can map the **most likely artifact periodicity** (e.g., every 8/16/32 pixels) to **which signal/bit** is suspect and give you a targeted “pattern test” for that specific failure mode.
