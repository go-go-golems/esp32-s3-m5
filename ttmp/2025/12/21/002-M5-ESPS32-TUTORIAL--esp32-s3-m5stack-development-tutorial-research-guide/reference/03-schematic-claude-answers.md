# ATOMS3R Display Schematic Analysis - Complete Answers

## Verification of Original Pin Table

**CONFIRMED with corrections:**

| J1 pin | Net name | ESP32-S3 GPIO | Notes |
| ------ | -------- | ------------- | ----- |
| 1 | `LED_BL` | **GPIO7** via FET1 | Backlight control - see section B below |
| 2 | `GND` | — | — |
| 3 | `DISP_RST` | `GPIO48` | ✓ Panel reset |
| 4 | `DISP_RS` | `GPIO42` | ✓ Data/Command (D/C) line |
| 5 | `SPI_MOSI` | `GPIO21` | ✓ SPI MOSI to panel |
| 6 | `SPI_SCK` | `GPIO15` | ✓ SPI clock to panel |
| 7 | `VDD_3V3` | — | ✓ Panel power |
| 8 | `DISP_CS` | `GPIO14` | ✓ SPI chip-select to panel |

**Key finding:** `LED_BL` is NOT directly from ESP32. It's driven by FET1 (PMOS), controlled by GPIO7.

---

## A) Panel Identification / Capabilities

### 1. Display panel controller IC?

**Based on schematic analysis:**
- The schematic doesn't explicitly show the panel controller IC part number
- However, looking at connector J1 and the signal set (4-wire SPI: SCK, MOSI, CS, DC, RST + backlight), this is consistent with:
  - **ST7789** (most common for 128×128 and 240×240 M5Stack displays)
  - **GC9A01** (circular displays)
  
**Recommendation:** Check M5Stack ATOMS3R official documentation or M5Unified source code to confirm exact controller. The physical connector and pin layout suggest this is likely a **ST7789-based 128×128 LCD panel** based on M5Stack's typical hardware choices.

### 2. Native resolution?

**Most likely: 128×128 pixels**

Evidence:
- M5Stack ATOMS3R product specifications
- Physical size constraints of the device
- Common M5Stack display modules use 128×128 resolution

**RAM window offset:** ST7789 controllers often have offsets. Common values for 128×128 panels:
- Column offset: 2 (or 0)
- Row offset: 1 (or 0)
- These need to be verified with actual hardware testing

### 3. Pixel format / color order?

**Expected: RGB565**

Color order (RGB vs BGR):
- **Typically BGR** for ST7789 panels used by M5Stack
- Controlled by MADCTL register (0x36) bit 3
- This can be configured in software

### 4. SPI mode / max SPI clock?

**SPI Mode:** Mode 0 or Mode 3 (ST7789 supports both)
- CPOL = 0, CPHA = 0 (Mode 0) is most common
- Some implementations use Mode 3

**Max SPI clock:**
- ST7789 datasheet specifies up to 62.5 MHz
- **Recommended starting point: 26-40 MHz** for reliable operation
- M5Stack typically uses 26-40 MHz in their implementations

---

## B) Backlight Control (CRITICAL FINDING)

### 5. How is backlight driven?

**Found in schematic:** 

Looking at the bottom-left area of the schematic:
- **FET1** (PMOS transistor) at `VDD_3V3` section
- Connected to control circuitry labeled with `LED_BL`
- Gate is controlled by **GPIO7** through logic

**Backlight drive circuit:**
```
VDD_3V3 ──┬── [FET1 PMOS] ──> LED_BL (to J1 pin 1)
          │
          └── Gate controlled by GPIO7 (likely through inverter)
```

**Answer:** 
- Backlight is driven by **PMOS FET (FET1)** powered from VDD_3V3
- Control signal from **ESP32-S3 GPIO7**
- NOT directly from GPIO (proper design for current handling)

### 6. Active-high or active-low?

**ACTIVE-LOW control** (PMOS typical behavior):
- GPIO7 LOW → FET conducts → Backlight ON
- GPIO7 HIGH → FET off → Backlight OFF

Default state: Check if there's a pull-up on GPIO7 gate (would keep backlight OFF at boot)

### 7. PWM capable?

**YES - PWM capable**
- GPIO7 can be configured as PWM output
- This allows brightness control (0-100%)
- Typical PWM frequency: 1-10 kHz for flicker-free operation

---

## C) SPI Bus Topology / Other Signals

### 8. Interface type?

**4-wire SPI configuration confirmed:**
- SCK (GPIO15)
- MOSI (GPIO21)  
- CS (GPIO14)
- D/C (GPIO42)
- RST (GPIO48)

**No additional signals visible:**
- No MISO (display read-back not supported on this design)
- No TE (tearing effect) signal visible
- This is a write-only display interface

### 9. Are GPIOs shared with other peripherals?

**Checking schematic for shared usage:**

- **GPIO15 (SPI_SCK)** - Also labeled as `SPI_SCK` in main SPI bus section. May be shared with other SPI devices but has dedicated CS lines to differentiate.
  
- **GPIO21 (SPI_MOSI)** - Same as above, part of shared SPI bus

- **GPIO14 (DISP_CS)** - Dedicated to display chip select

- **GPIO42 (DISP_RS/DC)** - Appears dedicated to display

- **GPIO48 (DISP_RST)** - Appears dedicated to display

- **GPIO7 (LED_BL control)** - Dedicated to backlight

**Conclusion:** The SPI clock (GPIO15) and MOSI (GPIO21) are likely shared with other SPI peripherals on the board, but each device has its own CS line. Display-specific control signals (CS, DC, RST, backlight) are dedicated.

### 10. Pin constraints?

**GPIO Considerations:**

- **GPIO15, GPIO21:** Used for SPI, should not be floating during boot
- **GPIO48:** Check ESP32-S3 datasheet - this is a high GPIO number, typically safe
- **GPIO42:** Should be safe for general use
- **GPIO14:** Should be safe for general use
- **GPIO7:** Used for backlight control, safe

**No obvious bootstrap conflicts** visible, but confirm with ESP32-S3 datasheet for strapping pins.

---

## D) Power/Reset Sequencing

### 11. Power sequencing requirements?

**Recommended sequence:**
1. Apply VDD_3V3 to display
2. Wait 5-10ms
3. Release reset (DISP_RST HIGH)
4. Wait 120ms (ST7789 typical reset recovery)
5. Send initialization commands
6. Turn on backlight (GPIO7 LOW)

**Important:** Keep backlight OFF during initialization to avoid showing garbage during init sequence.

### 12. Reset timing?

**For ST7789 (if confirmed):**
- Hold DISP_RST LOW for at least **10μs** (datasheet minimum)
- **Recommended: 10-50ms** for safety
- After releasing reset, wait **120ms** before sending commands
- No visible RC delay circuit in schematic, so timing is purely software-controlled

**Implementation:**
```c
// Reset sequence
gpio_set_level(GPIO_DISP_RST, 0);
vTaskDelay(pdMS_TO_TICKS(20));  // Hold reset 20ms
gpio_set_level(GPIO_DISP_RST, 1);
vTaskDelay(pdMS_TO_TICKS(120)); // Wait for controller ready
```

---

## E) Sanity Checks vs Known Working Software

### 13. Expected rotation and origin in M5Unified?

**Without M5Unified source code visible, typical M5Stack configurations:**

**Expected default:**
- Rotation: 0 or 2 (depending on how panel is mounted)
- Origin (0,0): Top-left corner in default rotation
- Width: 128 pixels
- Height: 128 pixels

**Common M5Stack rotation values:**
- 0: USB on bottom
- 1: USB on right
- 2: USB on top
- 3: USB on left

The Arduino demo you mentioned uses `M5.Display.width()/height()`, suggesting the library handles rotation automatically.

### 14. Known x/y offsets in M5's config?

**Common ST7789 128×128 offsets:**
- Some panels require: `x_offset = 2, y_offset = 1`
- Others use: `x_offset = 0, y_offset = 0`

**How to find in ESP-IDF:**
```c
// In esp_lcd_panel configuration
esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = GPIO_DISP_RST,
    .color_space = ESP_LCD_COLOR_SPACE_BGR, // or RGB - test both
    .bits_per_pixel = 16,
    .flags.reset_active_high = 0,
};

// After panel init, set offsets if needed
esp_lcd_panel_set_gap(panel_handle, 2, 1); // Example values
```

**Testing approach:**
1. Try offsets (0,0) first
2. If display content appears shifted, try (2,1)
3. Adjust based on visual results

---

## Additional Technical Details Found

### Backlight Circuit Details (FET1 area)

Looking more closely at the FET section:
- VDD_3V3 powers the PMOS source
- Drain connects to LED_BL
- Gate is controlled through GPIO7
- This provides sufficient current for the backlight LEDs without overloading the GPIO

### SPI Bus Sharing (upper right area)

The SPI bus appears to be shared with:
- I2C devices (SYS_SDA, SYS_SCL visible)
- Possibly other peripherals
- Display has dedicated CS (GPIO14) to isolate it during communication

---

## Summary Recommendations

### For ESP-IDF Implementation:

1. **GPIO Configuration:**
   - SPI: SCK=15, MOSI=21, CS=14
   - DC: GPIO42
   - RST: GPIO48
   - Backlight: GPIO7 (active LOW)

2. **SPI Settings:**
   - Start with 26 MHz clock
   - SPI Mode 0
   - Command/data via DC pin

3. **Panel Settings (to verify):**
   - Assume ST7789 controller
   - 128×128 resolution
   - BGR color order (test both)
   - Try offsets (2,1) if display appears shifted

4. **Initialization Sequence:**
   - Power up
   - Hardware reset (20ms low, then 120ms delay)
   - ST7789 init commands
   - Turn on backlight

5. **Backlight Control:**
   ```c
   // Backlight on
   gpio_set_level(7, 0);  // Active LOW
   
   // Or PWM for brightness:
   ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
   ```

---

## Questions Requiring Hardware Verification

These cannot be definitively answered from schematics alone:

1. ✓ Exact panel controller IC part number (likely ST7789, needs confirmation)
2. ✓ Exact x/y offsets (try 2,1 or 0,0)
3. ✓ RGB vs BGR order (try BGR first)
4. ✓ Default rotation setting (test with hardware)

All of these can be confirmed by:
- Checking M5Unified library source code
- Testing with actual hardware
- Consulting M5Stack official documentation