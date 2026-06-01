# ESP32P4 as Pico Replacement

- URL: https://chatgpt.com/c/6a1753cb-5580-83ea-a636-5b375b68f5a0
- With Activity: no
- Activity Exported: 0

## Turn 0 - user

- Message ID: `3b926a98-cbfb-4e6f-9231-912af30722d3`

I want to use a esp32p4 stamp-p4 by m5stack as a replacement for the raspberry pico in the picocalc device by clockworkpi. Can you look for scehamtics and pinouts and all that to see if that can be made to work, peripheral wise?

## Turn 1 - assistant

- Message ID: `d3454643-7582-43d0-ae24-aaa71a35ceb4`
- Model: `gpt-5-5-pro`
- Thought Trace: Thought for 13m 21s

Verdict

Yes, peripheral-wise this looks workable, but it is not a drop-in Pico replacement. Treat it as a custom interposer PCB plus a firmware port. The ESP32-P4 Stamp-P4 has enough GPIO, SPI, I2C, PWM, USB/UART, flash, and PSRAM to cover PicoCalc’s main peripherals. The biggest practical blocker is power routing, followed by firmware effort and boot/USB pin handling.

ClockworkPi describes PicoCalc’s mainboard as using an SPI screen, SD slot, onboard 8 MB PSRAM, dual PWM speakers, battery charge/discharge management, and an STM32 “south bridge” that exposes keyboard/backlight/power functions over I2C. The official repo also includes the mainboard schematic and LCD spec files. 
ClockworkPi
+2
ClockworkPi
+2

One caveat: the PicoCalc schematic PDF itself did not parse/render reliably in the browser tool, so the pin map below is based on ClockworkPi’s official firmware sources plus corroborating PicoCalc pinout notes.

PicoCalc peripheral map vs. Stamp-P4 feasibility
PicoCalc function	Pico-side signals / behavior	ESP32-P4 Stamp-P4 assessment
LCD	SPI display: SCK GP10, MOSI GP11, MISO GP12, CS GP13, DC GP14, RST GP15. ClockworkPi code sets 25 MHz SPI and 320×320 resolution. 
GitHub
	Works. ESP32-P4 has GP-SPI controllers; GP-SPI master supports up to 80 MHz, multiple CS lines, DMA, and GPIO-matrix routing when direct IO-MUX speed is not required. 
Espressif Documentation
+1

Keyboard / southbridge MCU	I2C on SDA GP6, SCL GP7, address 0x1F; official code uses a very slow 10 kHz bus and reads keyboard/battery/backlight commands through that device. 
GitHub
+3
GitHub
+3
GitHub
+3
	Works. ESP32-P4 has I2C controllers, supports master mode, 100/400 kHz standard speeds, and can route I2C through the GPIO matrix. Running slower for PicoCalc compatibility should be fine. 
Espressif Documentation

microSD / TF card	PicoCalc pin maps identify SPI-mode SD as MISO GP16, CS GP17, SCK GP18, MOSI GP19, detect GP22. 
clockworkpi
+1
	Works. Use ESP-IDF SDSPI, or a separate GP-SPI bus. ESP32-P4 SD drivers support SD over SDMMC or SDSPI. 
Espressif Systems
+1

Audio	Dual PWM audio on GP26 and GP27. ClockworkPi code configures both as PWM outputs. 
GitHub
+1
	Works. ESP32-P4 LEDC can generate PWM on GPIO outputs and has 8 channels. Use two channels for left/right audio into PicoCalc’s existing audio path. 
Espressif Systems

Onboard PicoCalc PSRAM	PicoCalc code uses RP2040-specific PIO/DMA PSRAM support. Configured pins include CS GP20, SCK GP21, MOSI GP2, MISO GP3; other notes identify quad pins GP4/GP5 too. 
GitHub
+3
GitHub
+3
GitHub
+3
	Probably ignore it. Stamp-P4 already includes 16 MB flash and 32 MB PSRAM. Reusing PicoCalc’s external PSRAM would require a custom ESP32-P4 driver and is not needed for first bring-up. 
M5Stack Docs

Serial console / USB-C path	PicoCalc notes identify UART0 as GP0 TX and GP1 RX routed through the USB-C/CH340C path; ClockworkPi docs also note PicoCalc’s USB Type-C defaults to Pico serial. 
clockworkpi
+2
GitHub
+2
	Works with care. Route PicoCalc’s USB-serial TX/RX to an ESP32-P4 UART, crossed correctly. ESP32-P4 UART0 defaults are GPIO37/GPIO38, but UART pins can be changed through the GPIO matrix. 
Espressif Documentation
+1

Native USB	Pico normally brings its own USB device behavior.	Optional. Stamp-P4 exposes USB-related pins, including GPIO24/GPIO25 for one USB pair and USB host pins on the module connector. Do not blindly connect both native USB and CH340 serial to the same external connector without designing the mux/path. 
M5Stack Documentation
+1

Wireless	Pico W/Pico WH compatibility exists on the PicoCalc side. 
ClockworkPi
	Not native on ESP32-P4. ESP32-P4 is not an ESP32-S3-style Wi-Fi/BLE part. M5Stack documents a Stamp-AddOn C6 option for 2.4 GHz Wi-Fi 6 over SDIO. 
M5Stack Docs
Main hardware risk: power

This is the part I would not hand-wave.

Stamp-P4’s product page specifies DC 5 V input. 
M5Stack Docs
 PicoCalc, meanwhile, uses an AXP2101-based battery/power system; a forum analysis of the PicoCalc schematic describes USB-C feeding AXP2101 VBUS_IN, batteries feeding VBAT, and the AXP2101/power-switch path supplying the Pico VSYS rail and onboard peripherals. 
clockworkpi

So: do not assume the Pico socket gives the Stamp-P4 a valid regulated 5 V rail. Measure it under USB, battery-full, battery-low, and power-switch states.

Recommended power approach:

Item	Recommendation
Stamp-P4 input	Feed Stamp-P4 VIN from a known-good regulated 5 V rail. A small buck-boost from the PicoCalc power path is the safest adapter-board approach.
Ground	Common ground between PicoCalc and Stamp-P4.
3.3 V rails	Do not tie PicoCalc 3V3 and Stamp-P4 SOC_3.3V together unless you verify regulator topology and backfeed behavior.
I/O level	PicoCalc peripherals appear to be 3.3 V-class. ESP32-P4 I/O rails are 3.3 V-class; Espressif lists VDD I/O recommended range up to 3.6 V, with absolute max 3.6 V. 
Espressif Documentation
+1

Reset/boot	Bring out Stamp-P4 CHIP_EN and boot/download strapping access on the adapter.
Suggested adapter pin plan

This is a practical first-pass mapping, not a final PCB sign-off. It avoids relying on the original Pico pin numbers matching the Stamp-P4 footprint, because they do not.

LCD on a fast/direct SPI group

ESP32-P4 has direct GP-SPI IO-MUX options on GPIO28–GPIO33, and the Stamp-P4 schematic exposes GPIO28–GPIO33 on the module edge. 
Espressif Documentation
+1

PicoCalc net	Original Pico pin	Suggested Stamp-P4 pin
LCD SCK	GP10	GPIO30
LCD MOSI	GP11	GPIO31
LCD MISO	GP12	GPIO29, optional
LCD CS	GP13	GPIO28
LCD DC	GP14	GPIO32
LCD RST	GP15	GPIO33

Note: verify your actual PicoCalc LCD controller. ClockworkPi’s repo includes an ST7365P spec, while firmware/community material also references ILI9488/ST7365P variants. Use the initialization sequence for the panel in your unit. 
GitHub
+2
GitHub
+2

I2C southbridge

The Stamp-P4 schematic labels low-power GPIO pins including LPG9_SCL and LPG11_SDA. 
M5Stack Documentation

PicoCalc net	Original Pico pin	Suggested Stamp-P4 pin
I2C SCL	GP7	LPG9 / GPIO9
I2C SDA	GP6	LPG11 / GPIO11
SD card on separate SPI

Use a second SPI bus through the GPIO matrix. ESP32-P4 GP-SPI3 can use arbitrary GPIO via the GPIO matrix. 
Espressif Documentation

PicoCalc net	Original Pico pin	Suggested Stamp-P4 pin
SD MISO	GP16	GPIO16
SD CS	GP17	GPIO17
SD SCK	GP18	GPIO18
SD MOSI	GP19	GPIO19
SD detect	GP22	GPIO20
Audio PWM
PicoCalc net	Original Pico pin	Suggested Stamp-P4 pin
Audio L PWM	GP26	GPIO21
Audio R PWM	GP27	GPIO22
Serial console
PicoCalc serial path	Suggested Stamp-P4 connection
PicoCalc USB-serial TX into Pico RX	Stamp-P4 UART RX
PicoCalc USB-serial RX from Pico TX	Stamp-P4 UART TX

GPIO37/GPIO38 are the Stamp-P4 schematic’s UART0 TX/RX labels, but ESP32-P4 UART pins are configurable. Keep in mind that the Stamp-P4 schematic/datasheet material marks some pins around GPIO35–GPIO38 as boot/strap-related, so avoid loading them in ways that disturb boot. 
M5Stack Documentation
+2
M5Stack Documentation
+2

Pins and features to be cautious with

Avoid these for random peripheral CS/DC/reset signals unless you check the boot behavior:

Area	Why
GPIO35/GPIO36/GPIO37/GPIO38	Stamp-P4 schematic marks boot/strap/UART-related functions around these pins. 
M5Stack Documentation
+1

GPIO24/GPIO25 and GPIO26/GPIO27	These are tied to USB-related functions on ESP32-P4/Stamp-P4 documentation. Use them intentionally, not casually. 
M5Stack Documentation
+1

RMII-labelled pins	Fine if you are not using Ethernet, but avoid conflict if you later want RMII Ethernet. Stamp-P4 labels several GPIO28–GPIO52 pins with RMII alternate functions. 
M5Stack Documentation

PicoCalc PSRAM pins	Leave unconnected for first revision. Stamp-P4 already has PSRAM, and PicoCalc’s PSRAM driver path is RP2040-specific. 
GitHub
+1
Firmware impact

The hardware is only half the project. You will not run PicoCalc’s Pico firmware unchanged.

You need an ESP32-P4 firmware port that implements:

SPI LCD driver at roughly the same mode/speed and correct panel init sequence.

I2C southbridge protocol for keyboard, battery readout, backlight, and power-related commands.

SDSPI card mount.

Two-channel PWM audio.

UART/USB console behavior.

Optional support for the Stamp-P4’s native PSRAM, which M5Stack already exposes through its Arduino/PlatformIO examples. 
M5Stack Docs

M5Stack documents Stamp-P4 support for Arduino, ESP-IDF, and PlatformIO, so the firmware platform is viable. 
M5Stack Docs
+1

Bottom line

Peripheral compatibility: yes. Drop-in compatibility: no.

A good first adapter revision would:

generate a clean 5 V rail for Stamp-P4 VIN;

route GND and leave 3.3 V rails isolated;

wire LCD, I2C keyboard/southbridge, SD, audio PWM, and serial as above;

leave PicoCalc PSRAM disconnected;

expose CHIP_EN, boot/download control, UART, and USB test pads;

avoid boot-strapping and USB pins unless deliberately used.

The project is realistic. The two items I would verify before PCB layout are the actual PicoCalc power rail voltage at the Pico socket and the exact LCD controller/init sequence in your PicoCalc unit.

