Here’s the “raw glass” story for **ED047TC1** + your **LovyanGFX Panel_EPD/Bus_EPD** stack:

* **ED047TC1** is an **active-matrix TFT electrophoretic (E Ink) panel** (540×960, 16 gray levels depending on the controller + waveform file). ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* Your code is acting like a **mini EPD timing controller**: it converts a target image into **per-pixel drive actions over time** (the LUT “waveforms”), then scans them out line-by-line using ESP32-S3’s **I80 (8080-style) parallel bus**.

Below: (1) what the ink particles are doing, (2) what each pin does, (3) how your “waveform LUT” maps to those pins, (4) how I80 is used, and (5) ASCII timing diagrams + Python that generates them.

---

## 1) What E-Ink particles are doing

E-Ink is basically millions of microcapsules containing **two oppositely-charged pigment particles** in fluid. Apply an electric field, and one pigment moves toward the viewing side; reverse the field, the other pigment moves toward the viewing side. When you stop the field, the particles largely stay put (bistable). ([E Ink. We Make Surfaces Smart and Green][2])

**Grayscale** comes from **controlled partial motion** and hysteresis: you apply a *sequence* of pulses (often alternating polarity, with rests) so the visible mixture of particles near the top ends up as “light gray”, “dark gray”, etc. ([IEEE Spectrum][3])

That’s why ED047TC1 explicitly says gray levels depend on the **waveform file** used by the controller. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

---

## 2) ED047TC1 pins: what they are (datasheet)

From the ED047TC1 pin assignment table: ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

### Source-driver (columns) signals

* **XCL**: “Clock source driver” (shift clock) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **XLE**: “Latch enable source driver” (latch shifted bits into outputs) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **XOE**: “Output enable source driver” (enable/tri-state outputs) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **XSTL**: “Start pulse source driver” (framing/start pulse) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **D0..D7**: “Data signal source driver” (the bus) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **VPOS / VNEG**: high-voltage rails used by source outputs ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **VCOM**: common electrode reference ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

### Gate-driver (rows) signals

* **CKV**: “Clock gate driver” (row shift clock) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **SPV**: “Start pulse gate driver” (start/prime the gate shift register) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **VGG / VEE**: gate HV rails (positive/negative) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* **MODE1**: output mode selection ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

### Timing note you already matched in code

The datasheet’s gate timing note: **“After 5 CKV, gate line is on.”** ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
Your `beginTransaction()` “pre-clocks” CKV such that the first real scanline pulse becomes that 5th CKV.

---

## 3) What your “waveforms” actually are (Panel_EPD LUT → drive states)

### A) Your LUT entries are *per-pixel, per-frame drive actions*

Your LUT values are 2-bit “actions” (per pixel, per frame), with semantics described in the source:

* `1` = “to black”
* `2` = “to white”
* `3` = “no operation”
* `0` = “end of data” (software end-marker)

Conceptually, those actions correspond to selecting one of a small set of **source output states** each frame (e.g., apply a positive pulse, apply a negative pulse, hold/neutral, etc.). The *exact* electrical mapping of 2-bit codes → analog levels is inside the panel’s source driver and isn’t spelled out in the text table we can extract; but the panel clearly provides the rails (VPOS/VNEG/VCOM) that those output states are drawn from. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

### B) “Eraser” waveform = reduce ghosting

Your `lut_eraser` forces a short “push toward mid-gray” phase before the real waveform. That’s exactly the kind of multi-phase pulse strategy used to fight electrophoretic hysteresis/ghosting. (This aligns with the physics above: you often need a “reset-ish” stage before a clean new level.)

---

## 4) How the I80 bus is used in *your* Bus_EPD

ESP-IDF’s I80 bus is the classic 8080-style parallel interface:

* **WR** (pixel clock / write strobe)
* **DC** (data vs command select, aka RS)
* **CS**
* **data bus width 8 or 16**
* **pclk_hz** controls transfer rate ([Espressif Systems][4])

In your Bus_EPD:

### Mapping (what ESP-IDF drives)

* ESP I80 **WR** (`wr_gpio_num`) → your `pin_cl` → **ED047TC1 XCL** (source clock) ([Espressif Systems][4])
* ESP I80 **CS** (`cs_gpio_num`) → your `pin_sph` → **ED047TC1 XSTL** (source start pulse / framing) ([Espressif Systems][4])
* ESP I80 **D0..D7** (`data_gpio_nums[]`) → **ED047TC1 D0..D7** ([Espressif Systems][4])

### What *you* toggle manually (not part of I80)

* **XLE**: you hold low during shift, then raise on DMA-done callback
* **CKV / SPV**: you generate gate timing in software

### The key: your bus payload is “2 bits per pixel”

Your Panel_EPD pipeline packs “actions” into bytes such that:

* **1 byte = 4 pixels**
* each pixel is a 2-bit action code (`0..3`)
  So the I80 bus is being used as a **fast shifter** that blasts those 2-bit codes into the source driver shift register for one scanline.

### Rail generation on PaperS3

PaperS3’s schematic shows discrete rails/labels like **EPD_VPOS / EPD_VNEG / EPD_VGH / EPD_VGL / VCOM**, so board power circuitry is creating the required HV rails for the panel. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][5])
The panel datasheet’s sequencing requirements (VDD → VNEG → VPOS → VCOM, etc.) explain why rail behavior matters. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

---

## 5) Timing constraints from the datasheet (the “rules”)

From ED047TC1 timing table snippets (text-extracted):

* **XLE high pulse width** `tLEw` ≥ **300 ns** (at VCC 3.0–3.6V) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* XSTL setup/hold are expressed in fractions of the XCL cycle (`tcy`) ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
* There’s also an “output settling” style timing (`tout`) on the order of microseconds in the same section ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

And for gate: “After 5 CKV, gate line is on.” ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

Power sequencing is explicitly specified too. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

---

## 6) ASCII timing diagrams (generated from code)

These are **not screenshots** of the datasheet diagrams; they’re derived directly from:

* your `Bus_EPD::beginTransaction()` toggles
* your `Bus_EPD::writeScanLine()` + `notify_line_done()` behavior
  …and shaped to respect the conceptual meaning of the datasheet pins.

### A) `beginTransaction()` (SPV/CKV priming)

(Each tick = ~1 µs in this diagram.)

```text
ticks (us): 01234567890123456789012
   SPV: _____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
   CKV: ‾|__|‾‾‾|__|‾‾|__|‾‾|__
```

Interpretation:

* SPV pulses low then goes high (frame/row-start priming)
* CKV does the “pre-clocks” so the first real scanline aligns with the “5 CKV” gate-on note. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

### B) One scanline transfer (I80 burst + latch)

(Each tick = **tcy/2** i.e., half a WR/XCL cycle; shown here for 8 bytes just to keep it readable.)

```text
ticks (tcy/2): 01234567890123456789
  XSTL: ‾‾________________‾‾
   XCL: ___‾_‾_‾_‾_‾_‾_‾_‾__
  DATA: __‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾__
   XLE: __________________‾‾
   CKV: __‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾__
```

Interpretation (matches your code):

* **XLE low** while shifting (so source driver shift register is filling)
* **XSTL active** during burst (ESP I80 “CS”)
* **XCL toggles** each byte (ESP I80 “WR”)
* **CKV held high** during the scanline burst, then dropped low at end-of-transfer (your callback)
* Finally, **XLE goes high** to latch the line’s source data

---

## 7) Python code to generate these diagrams (so you can vary pclk/bytes)

```python
from dataclasses import dataclass, field
from typing import List

@dataclass
class Segment:
    duration: int  # ticks
    state: int     # 0/1

@dataclass
class Signal:
    name: str
    segments: List[Segment] = field(default_factory=list)

    def extend(self, duration: int, state: int):
        if duration <= 0:
            return
        if self.segments and self.segments[-1].state == state:
            self.segments[-1].duration += duration
        else:
            self.segments.append(Segment(duration, state))

    def total(self) -> int:
        return sum(s.duration for s in self.segments)

def render_levels(signals: List[Signal], tick_label="ticks", max_width: int = 120) -> str:
    total = max(s.total() for s in signals)
    rows = []
    for sig in signals:
        states = []
        for seg in sig.segments:
            states.extend([seg.state] * seg.duration)
        if not states:
            states = [0] * total
        elif len(states) < total:
            states.extend([states[-1]] * (total - len(states)))
        line = "".join("‾" if st else "_" for st in states)
        rows.append((sig.name, line))

    width = total
    if width > max_width:
        factor = (width + max_width - 1) // max_width
        rows = [(n, "".join(line[i] for i in range(0, len(line), factor))) for n, line in rows]
        width = len(rows[0][1])

    axis = "".join(str(i % 10) for i in range(width))
    out = [f"ticks ({tick_label}): {axis}"]
    out += [f"{name:>6}: {line}" for name, line in rows]
    return "\n".join(out)

def diagram_begin_transaction():
    # 1 tick = 1us (coarse), derived from your beginTransaction() delays.
    spv = Signal("SPV")
    ckv = Signal("CKV")

    # Assume CKV idles high going in (endTransaction drives CKV high).
    spv.extend(1, 0)  # SPV low 1us
    ckv.extend(1, 1)

    spv.extend(3, 0)  # CKV low 3us
    ckv.extend(3, 0)

    spv.extend(1, 0)  # CKV high 1us
    ckv.extend(1, 1)

    # SPV high; 3 cycles: CKV high 3us then low 3us
    for _ in range(3):
        spv.extend(3, 1); ckv.extend(3, 1)
        spv.extend(3, 1); ckv.extend(3, 0)

    print(render_levels([spv, ckv], tick_label="us", max_width=80))

def diagram_scanline(bytes_n=240):
    # 1 tick = half XCL cycle (tcy/2). Use 2 ticks per byte for a square wave.
    xstl = Signal("XSTL")
    xcl  = Signal("XCL")
    data = Signal("DATA")
    xle  = Signal("XLE")
    ckv  = Signal("CKV")

    pre = 2
    xstl.extend(pre, 1); xcl.extend(pre, 0); data.extend(pre, 0); xle.extend(pre, 0); ckv.extend(pre, 0)

    transfer_ticks = bytes_n * 2
    xstl.extend(transfer_ticks, 0)  # active during burst
    data.extend(transfer_ticks, 1)
    xle.extend(transfer_ticks, 0)
    ckv.extend(transfer_ticks, 1)

    for i in range(transfer_ticks):
        xcl.extend(1, i % 2)  # _‾_‾_‾...

    post = 2
    xstl.extend(post, 1); xcl.extend(post, 0); data.extend(post, 0); xle.extend(post, 1); ckv.extend(post, 0)

    print(render_levels([xstl, xcl, data, xle, ckv], tick_label="tcy/2", max_width=120))

if __name__ == "__main__":
    diagram_begin_transaction()
    print()
    diagram_scanline(bytes_n=8)      # small example
    # diagram_scanline(bytes_n=240)  # typical 960px/4 = 240 bytes per line
```

---

## 8) Putting it all together: “what each signal does to the particles”

Think in layers:

### Gate side (SPV/CKV + VGH/VGL)

Gate driver uses **SPV + CKV** plus **VGH/VGL rails** to select one row at a time (turn the TFTs for that row “on”). ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
Selecting the row lets the column voltages actually charge the pixel capacitors.

### Source side (XCL/XSTL/D0..D7/XLE/XOE + VPOS/VNEG/VCOM)

Source driver shifts in the per-pixel **2-bit action** codes using:

* **XCL** clocking data on **D0..D7**
* **XSTL** framing the burst
* **XLE** latching the whole line into output latches
* **XOE** enabling the outputs

Those output states connect each pixel electrode (through the TFT) to some combination of **VPOS/VNEG/VCOM/neutral** for that frame. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])
That creates an **electric field across the electrophoretic layer**, moving charged pigment particles toward/away from the viewing surface. ([E Ink. We Make Surfaces Smart and Green][2])

### Waveforms (LUT over time)

Your LUT decides *which polarity / hold* to apply this frame, then again next frame, etc., until the particles settle at the intended gray. That’s the “waveform file” concept the datasheet calls out. ([m5stack-doc.oss-cn-shenzhen.aliyuncs.com][1])

---

If you want, tell me the `pclk_hz` you actually configured (`_config.bus_speed`) and the effective bytes-per-line (`memory_w/4 + padding`), and I’ll extend the generator to label real **µs/ns** on the scanline diagram and check against the datasheet’s XLE pulse width (≥300 ns) and the tcy-derived setup/hold constraints.

[1]: https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/C139_ED047TC1_datasheet.pdf "Spec no"
[2]: https://www.eink.com/tech/detail/How_it_works?utm_source=chatgpt.com "Electronic Ink｜E Ink Technology"
[3]: https://spectrum.ieee.org/how-e-ink-developed-full-color-epaper?utm_source=chatgpt.com "How E Ink Developed Full-Color e-Paper"
[4]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/lcd/i80_lcd.html?utm_source=chatgpt.com "I80 Interfaced LCD - ESP32 - — ESP-IDF Programming ..."
[5]: https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/sch_papers3_V1.0.pdf "Schematic Prints"
