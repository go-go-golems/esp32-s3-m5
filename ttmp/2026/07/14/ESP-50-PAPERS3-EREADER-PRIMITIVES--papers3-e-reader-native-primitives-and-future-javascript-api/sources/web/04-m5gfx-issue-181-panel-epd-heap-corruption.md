While working on an app for M5Paper S3 I encountered some unexpected crashes. After debugging the issue I found the reason in Panel\_EPD.cpp. There are two separate issues that I will describe here.

**Root Cause 1: task\_update() odd byte-width overrun**

**Repro Instructions:**

- Clone repository with `git clone https://codeberg.org/mikaryyn/panel-epd-bug-repro.git`. The repo contains a test project using ESP-IDF 5.3.3 and M5GFX 0.2.19.
- Checkout the repro case: `git checkout bug1`
- Run the repro case on M5PaperS3: `idf.py build flash monitor` and observe a crash in the logs.

**Proposed Fix:**

- Checkout the fix to Panel\_EPD.cpp: `git checkout fix1`
- Run the fixed application: `idf.py build flash monitor`. Now it should run without crashing.

**Details:** Panel\_EPD::task\_update() can write past the end of a row when the update width (in bytes) is odd.

- In task\_update(), the update width is converted from pixels to bytes: w\_bytes = new\_data.w >> 1 (because 1 byte = 2 pixels).
- The hot loop processes 2 bytes at a time (s\[0\] and s\[1\]) and writes 4 uint16\_t values (d\[0..3\]), then advances s += 2, d += 4.
- If w\_bytes is odd (example: a 2-pixel-wide update becomes w\_bytes = 1), the “process 2 bytes” loop still runs once and effectively performs one extra iteration worth of work, resulting in a 4-byte out-of-bounds write past the intended end of the row.
- If that write lands at the tail of the \_step\_framebuf allocation, it can clobber heap poison/tail metadata, and the next PSRAM heap check  
	(heap\_caps\_check\_integrity(...)) will fail (often tripping in tlsf\_check()), even though the heap checker is only reporting earlier corruption.

**Root Cause 2: display() not rotating the update rectangle**

**Repro Instructions:**

- Use the same git repo as before.
- Checkout the repro case: `git checkout bug2`
- Run the repro case on M5PaperS3: `idf.py build flash monitor` and observe a crash in the logs.

**Proposed Fix:**

- Checkout the fix to Panel\_EPD.cpp: `git checkout fix2`
- Run the fixed application: `idf.py build flash monitor`. Now it should run without crashing.

**Details:**  
Panel\_EPD::display(x, y, w, h) can enqueue an update rectangle using unrotated logical coordinates, which becomes out-of-range for the panel’s physical backing buffer, causing task\_update() to walk off the end of \_step\_framebuf.

- LGFXBase::display(x,y,w,h) clips and forwards the rectangle, but does not apply rotation. Rotation must be handled inside the panel implementation.
- Panel\_EPD::display() updates the internal dirty region (\_range\_mod) using the unrotated (x,y,w,h), and then builds the queued update (update\_data\_t) from \_range\_mod.
- On M5Paper S3 the physical EPD backing buffer in PSRAM is 960×540, while the logical size exposed to the app is typically 540×960 under rotation.
- As a result, calls like display.display(0, 0, display.width(), display.height()) (i.e. display(0,0,540,960)) can enqueue an update with h = 960 even though \_step\_framebuf only has 540 rows. When task\_update() iterates h rows, it writes past the end of \_step\_framebuf and corrupts PSRAM heap metadata; the next heap\_caps\_check\_integrity(...) is just where the corruption gets detected.