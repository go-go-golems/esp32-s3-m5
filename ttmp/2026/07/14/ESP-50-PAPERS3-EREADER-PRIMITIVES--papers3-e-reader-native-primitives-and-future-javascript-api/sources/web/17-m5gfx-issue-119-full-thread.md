# m5stack/M5GFX issue #119: m5paperS3 not working

- URL: https://github.com/m5stack/M5GFX/issues/119
- State: `closed`
- Created: 2025-03-15T12:28:25Z
- Updated: 2025-05-16T07:35:41Z
- Author: `rudiratlos`

## Issue body

Hi,
I tried to get this .ino to work, but it crashes:

`#include <epdiy.h>
#include <M5Unified.h>
#include <M5GFX.h>

const int tft_x_px= 960;
const int tft_y_px= 540;

M5GFX display;
M5Canvas canvas(&display);

void setup(void) {
  Serial.begin(115200);
  Serial.printf("size of int:%d max:%d\n",sizeof(int),INT_MAX);

  auto cfg = M5.config();
  M5.begin(cfg);

  display.begin();

  canvas.createSprite(300, 300);

  canvas.fillSprite(TFT_WHITE);
  display.startWrite();
  canvas.pushSprite(0, 0);
  display.endWrite();

  canvas.setFont(&fonts::Font4);
  canvas.setTextSize(2.0);
  canvas.drawString("HalloHallo",50,50);
  canvas.drawLine(100,100,200,100,TFT_BLACK);
  display.startWrite();
  canvas.pushSprite(0, 0);
  display.endWrite();
}

void loop(void) {
  sleep(3);
}
`

this is on the serial output:

`E (3952) epdiy: init_dma_trans_link(298): dma connect error
E (3957) epdiy: epd_lcd_init(582): install DMA failed
E (3962) epdiy: LCD initialization failed!

abort() was called at PC 0x42003bad on core 1


Backtrace: 0x40378176:0x3fceb9c0 0x4037bddd:0x3fceb9e0 0x4038273d:0x3fceba00 0x42003bad:0x3fceba80 0x420084d1:0x3fcebb30 0x42004472:0x3fcebbc0 0x42002eb9:0x3fcebc20 0x42008dc7:0x3fcebc40 0x42009ccc:0x3fcebdc0 0x42002c98:0x3fcebdf0 0x4201267a:0x3fcebe50


ELF file SHA256: f74cea1c25906bfc
`

## Comments

### Comment 1: lovyan03 at 2025-03-15T15:11:09Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2726726676

Hello, @rudiratlos
If you use M5Unified, you don't need to create an instance of M5GFX yourself. M5.Display is provided automatically.
So the code you need is as follows:

```cpp
#include <M5Unified.h>

M5Canvas canvas(&M5.Display);

void setup(void) {
Serial.begin(115200);
Serial.printf("size of int:%d max:%d\n",sizeof(int),INT_MAX);

auto cfg = M5.config();
M5.begin(cfg);

canvas.createSprite(300, 300);

canvas.fillSprite(TFT_WHITE);
canvas.pushSprite(0, 0);

canvas.setFont(&fonts::Font4);
canvas.setTextSize(2.0);
canvas.drawString("HalloHallo",50,50);
canvas.drawLine(100,100,200,100,TFT_BLACK);
canvas.pushSprite(0, 0);
}
```

### Comment 2: rudiratlos at 2025-03-16T12:40:39Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2727374657

thank you very much, this worked

### Comment 3: jesusfj710 at 2025-03-19T18:56:15Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2737710284

hey @lovyan03 could you help me here? I have been having a lot of problems with my new M5PaperS3. If I load that same code (with a dummy loop) using the `M5PaperS3` board from the M5Stack (v.2.1.3) I get some noise on the screen and no text. It also seems that unless I enter another `Serial.println("something")` in the loop, it will not print the line for the size of the int.

If I try the same with the ESP32S3 dev module, it wont change the result.

Using "SpinTile" example for from `M5GFX.h` with the ESP32S3 dev module works just fine, using the M5PaperS3 device does not, as its asking me for `miniz.h`...

### Comment 4: jesusfj710 at 2025-03-21T16:58:45Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2743943511

Or maybe @rudiratlos can you share your settings? Board manager version, board selected, libraries installed and all of that? I cannot make this example work on my side

### Comment 5: rudiratlos at 2025-03-22T14:01:38Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2745280615

you need epdiy.h included (place it in front of m5unified):

#include <epdiy.h>
#include <M5Unified.h>
....

use a command window and clone epdiy to your Arduino library (e.g. on mac os) :

cd /Users/{username}/Documents/Arduino
git clone https://github.com/vroland/epdiy.git

pls. see:
[https://github.com/vroland/epdiy](https://github.com/vroland/epdiy)

### Comment 6: jesusfj710 at 2025-03-22T15:10:06Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2745314039

> you need epdiy.h included (place it in front of m5unified):
>
> #include <epdiy.h> #include <M5Unified.h> ....
>
> use a command window and clone epdiy to your Arduino library (e.g. on mac os) :
>
> cd /Users/{username}/Documents/Arduino git clone https://github.com/vroland/epdiy.git
>
> pls. see: https://github.com/vroland/epdiy

I already had clone the repo from epdiy in my arduino libs. The moment I include the epidy.h lib on top, with the ESP32S3 dev module from Espressif System (v3.0.7 as per [this](https://docs.m5stack.com/en/arduino/m5papers3/program)), it crash with this:

```
16:03:41.685 -> Rebooting...
16:03:41.685 -> ESP-ROM:esp32s3-20210327
16:03:41.685 -> Build:Mar 27 2021
16:03:41.685 -> rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
16:03:41.685 -> Saved PC:0x40378561
16:03:41.685 -> SPIWP:0xee
16:03:41.685 -> mode:DIO, clock div:1
16:03:41.685 -> load:0x3fce3818,len:0x109c
16:03:41.685 -> load:0x403c9700,len:0x4
16:03:41.685 -> load:0x403c9704,len:0xb50
16:03:41.685 -> load:0x403cc700,len:0x2fe4
16:03:41.718 -> entry 0x403c98ac
16:03:41.815 -> E (134) ADC: CONFLICT! driver_ng is not allowed to be used with the legacy driver
16:03:41.815 ->
16:03:41.815 -> abort() was called at PC 0x42015c87 on core 0
16:03:41.815 ->
16:03:41.815 ->
16:03:41.815 -> Backtrace: 0x40377f2a:0x3fceb200 0x4037d7cd:0x3fceb220 0x4038386d:0x3fceb240 0x42015c87:0x3fceb2c0 0x4201dfde:0x3fceb2e0 0x40378237:0x3fceb310 0x403cdb0e:0x3fceb340 0x403cdea5:0x3fceb380 0x403c9919:0x3fceb4b0 0x40045c01:0x3fceb570 0x40043ab6:0x3fceb6f0 0x40034c45:0x3fceb710
16:03:41.847 ->
16:03:41.847 ->
16:03:41.847 ->
16:03:41.847 ->
16:03:41.847 -> ELF file SHA256: c27cfd68b5b60382
16:03:41.847 ->
16:03:41.847 -> E (162) esp_core_dump_flash: Core dump flash config is corrupted! CRC=0x7bd5c66f instead of 0x0
16:03:41.847 -> E (171) esp_core_dump_elf: Elf write init failed!
16:03:41.880 -> E (175) esp_core_dump_common: Core dump write failed with error=-1
```

Not sure what to do already, all libs are ok in theory, config for the devide too. I have tried in W10 as well in Ubuntu 24.4.2.

![Image](https://github.com/user-attachments/assets/7b6d8d5c-5fa8-429b-8ff3-0ff68f9b03bd)

### Comment 7: rudiratlos at 2025-03-23T07:55:23Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2746075786

<img width="373" alt="Image" src="https://github.com/user-attachments/assets/c067ad74-ba13-4532-817b-089661f53df1" />

I have m5paperS3 as selected Board

Sketch uses 1242629 bytes (39%) of program storage space. Maximum is 3145728 bytes.
Global variables use 56420 bytes (17%) of dynamic memory, leaving 271260 bytes for local variables. Maximum is 327680 bytes.
esptool.py v4.5.1
Serial port /dev/cu.usbmodem14501
Connecting...
Chip is ESP32-S3 (revision v0.2)
Features: WiFi, BLE
Crystal is 40MHz

### Comment 8: jesusfj710 at 2025-03-23T12:24:39Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2746183152

Everytime i tried with the M5PaperS3 board I get this error:
```
jesus@XPS-13:~/arduinooo/bin$ ./arduino-cli compile --fqbn m5stack:esp32:m5stack_papers3 test/
/home/jesus/Arduino/libraries/epdiy/src/font.c:8:10: fatal error: miniz.h: No such file or directory
 #include <miniz.h>
          ^~~~~~~~~
compilation terminated.

Used library Version Path
epdiy        2.0.0   /home/jesus/Arduino/libraries/epdiy
M5Unified    0.2.5   /home/jesus/Arduino/libraries/M5Unified
M5GFX        0.2.6   /home/jesus/Arduino/libraries/M5GFX

Used platform Version Path
m5stack:esp32 2.1.3   /home/jesus/.arduino15/packages/m5stack/hardware/esp32/2.1.3

```

### Comment 9: jesusfj710 at 2025-03-23T12:52:51Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2746196845

@rudiratlos could you please check with this file and deps? This does compile for me, but shows noise on the screen.


=== Arduino CLI Version ===
```plaintext
arduino-cli  Version: 1.2.0 Commit: 9c495211 Date: 2025-02-24T15:57:30Z
```

=== Arduino CLI Config ===
```yaml
board_manager:
    additional_urls:
        - https://espressif.github.io/arduino-esp32/package_esp32_index.json
        - https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
```

=== Installed Board Managers ===
```plaintext
ID            Installed Latest Name
arduino:avr   1.8.6     1.8.6  Arduino AVR Boards
esp32:esp32   3.0.4     3.1.3  esp32
m5stack:esp32 2.1.3     2.1.4  M5Stack
```

=== Available M5Stack Core Versions ===
```plaintext
ID            Version Name
m5stack:esp32 2.1.4   M5Stack
```

=== Installed Libraries ===
```plaintext
Name      Installed Available    Location Description
M5GFX     0.2.6     -            user     -
M5Unified 0.2.5     -            user     -
```

=== test/test.ino Content ===
```cpp
#include <M5Unified.h>

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.println("Hello from M5Unified!");
}

void loop() {}
```
=== Connected Serial Devices ===
```
Port         Protocol Type              Board Name          FQBN                     Core
/dev/ttyACM0 serial   Serial Port (USB) ESP32 Family Device esp32:esp32:esp32_family esp32:esp32
```

=== USB Devices (lsusb) ===
```
# ...
Bus 003 Device 025: ID 303a:1001 Espressif USB JTAG/serial debug unit
# ...
```

=== Compile Output ===
```plaintext
Sketch uses 399821 bytes (12%) of program storage space. Maximum is 3145728 bytes.
Global variables use 21108 bytes (6%) of dynamic memory, leaving 306572 bytes for local variables. Maximum is 327680 bytes.
```

=== Upload Output ===
```plaintext
esptool.py v4.5.1
Serial port /dev/ttyACM0
Connecting...
Chip is ESP32-S3 (revision v0.2)
Features: WiFi, BLE
Crystal is 40MHz
MAC: a0:85:e3:f2:89:ec
Uploading stub...
Running stub...
Stub running...
Changing baud rate to 921600
Changed.
Configuring flash size...
Flash will be erased from 0x00000000 to 0x00003fff...
Flash will be erased from 0x00008000 to 0x00008fff...
Flash will be erased from 0x0000e000 to 0x0000ffff...
Flash will be erased from 0x00010000 to 0x00071fff...
Compressed 15104 bytes to 10401...
Writing at 0x00000000... (100 %)
Wrote 15104 bytes (10401 compressed) at 0x00000000 in 0.3 seconds (effective 456.8 kbit/s)...
Hash of data verified.
Compressed 3072 bytes to 144...
Writing at 0x00008000... (100 %)
Wrote 3072 bytes (144 compressed) at 0x00008000 in 0.0 seconds (effective 508.9 kbit/s)...
Hash of data verified.
Compressed 8192 bytes to 47...
Writing at 0x0000e000... (100 %)
Wrote 8192 bytes (47 compressed) at 0x0000e000 in 0.1 seconds (effective 645.9 kbit/s)...
Hash of data verified.
Compressed 400192 bytes to 224535...
Writing at 0x00010000... (7 %)
Writing at 0x00018135... (14 %)
Writing at 0x0002a3a0... (21 %)
Writing at 0x00030afd... (28 %)
Writing at 0x00036243... (35 %)
Writing at 0x0003bdfd... (42 %)
Writing at 0x00041557... (50 %)
Writing at 0x0004699a... (57 %)
Writing at 0x0004be0d... (64 %)
Writing at 0x00051567... (71 %)
Writing at 0x0005944e... (78 %)
Writing at 0x00060515... (85 %)
Writing at 0x00067e46... (92 %)
Writing at 0x0006d7f2... (100 %)
Wrote 400192 bytes (224535 compressed) at 0x00010000 in 3.4 seconds (effective 945.5 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
New upload port: /dev/ttyACM0 (serial)
```


### Comment 10: mitsuharu at 2025-03-28T04:01:51Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2760119975

epdiy of current main brach drops to support ESP-IDF 4.
see: https://github.com/vroland/epdiy/commits/main/

I develop with epdiy that used to support ESP-IDF 4.

```diff
lib_deps =
-	epdiy=https://github.com/vroland/epdiy.git
+	epdiy=https://github.com/vroland/epdiy.git#d84d26ebebd780c4c9d4218d76fbe2727ee42b47 ; for example
```

Incidentally, if use epdiy 2.0.0, M5GFX is error.
M5GFX use lcd_bus_config_t type that has been changed after epdiy 2.0.0.

### Comment 11: lovyan03 at 2025-04-17T05:14:56Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2811764975

Hello everyone.
I apologize for the long wait.

I have updated the `develop` branch.
The new version works with M5GFX alone, without EPDiy.
Please try it out.
If there are no problems, it will be included in the next update release.

### Comment 12: felmue at 2025-04-17T08:23:45Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812158404

Hello @lovyan03

thank you for the update. I encountered the following runtime error:
```
assert failed: prvInitialiseNewTask tasks.c:1061 (uxPriority < ( 25 ))
```
I got past it by lowering `task_priority` from 25 to 24 in `Panel_EPD.hpp`.

Thanks
Felix

### Comment 13: lovyan03 at 2025-04-17T09:00:58Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812244182

thanks @felmue
I fixed it and update develop branch.

### Comment 14: felmue at 2025-04-17T09:02:37Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812248329

Hello @lovyan03

hmm, running my M5PaperS3 clock [example](https://github.com/felmue/MyM5StackExamples/blob/main/M5PaperS3/RTCClockNonFlickering/RTCClockNonFlickering.ino) with the new M5GFX code gives me some artifacts:

![Image](https://github.com/user-attachments/assets/f84e05ee-3e71-4f0b-8c8d-d44d3ad64f35)

Thanks
Felix

### Comment 15: lovyan03 at 2025-04-17T09:24:34Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812299365

Thank you for reporting. @felmue
I have updated the develop branch again.
This fixes the waitDisplay function, which waits until the display drawing operation is completed, but this function was not implemented.

### Comment 16: lovyan03 at 2025-04-17T09:46:09Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812350691

sorry, I just updated develop branch again.

### Comment 17: felmue at 2025-04-17T10:16:37Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812419813

Hello @lovyan03

no worries. With all your latest changes the display has improved.

That said, I think there is still something not quite right with `M5.Display.clearDisplay()` as it leaves a very faint imprint of the previous screen.

![Image](https://github.com/user-attachments/assets/e0479df6-3626-45c5-97fd-dc375bf5216a)

and after some time the display looks like this:

![Image](https://github.com/user-attachments/assets/6292f776-b025-44d3-bb09-dfff84a116d3)

Thanks
Felix

### Comment 18: lovyan03 at 2025-04-17T11:51:33Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812626609

@felmue
The develop branch has been readjusted.
The clearDisplay function is now less likely to leave behind garbage.
However, `epd_fastest` still tends to leave behind garbage, but this is by design.
I recommend that you run clearDisplay before changing to epd_fastest.

### Comment 19: jesusfj710 at 2025-04-17T12:04:03Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812652337

So @lovyan03, with this config, it means i can use latest platform and unified from espressif and m5stack? No need to have epdiy anywhere? So, I just also need to include the M5Unified header in the code and that's it?


```ini
; platformio.ini
; ...

[env:PaperS3]
platform = espressif32 ; latest
board = esp32-s3-devkitm-1
framework = arduino
board_build.partitions = default_16MB.csv
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.arduino.memory_type = qio_opi
monitor_speed = 115200
build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
	-DARDUINO_USB_MODE=1
lib_deps =
    m5stack/M5Unified ; latest
    M5GFX=https://github.com/m5stack/M5GFX.git#develop ; pointing dev
```

### Comment 20: jesusfj710 at 2025-04-17T12:30:10Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2812720030

Here my latest examples: https://github.com/jesusfj710/test-papers3-m5gfx

However, let me tell you, this is having a really weird behaviour on the screen and make it flash it every second. Even on FASTEST

### Comment 21: felmue at 2025-04-17T21:15:18Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2814037135

Hello @lovyan03

thank you for the suggestion regarding `clearDisplay()` and EPD mode. I modified my clock [example](https://github.com/felmue/MyM5StackExamples/blob/main/M5PaperS3/RTCClockNonFlickering/RTCClockNonFlickering.ino) accordingly and it improved the result after clearing the display.

That said, I still see a difference when my clock example runs for almost an hour (and before doing a full clear again) between the new M5GFX library version and using EPDiy library.

With the new M5GFX library the areas which are not updated gradually get greyer and greyer.

New M5GFX library ...

![Image](https://github.com/user-attachments/assets/55f14a7c-244a-4adc-8a2f-1ed151522d52)

... and running the identical code, but using EPDiy library:

![Image](https://github.com/user-attachments/assets/a23c6782-9d46-4bd9-b8e6-858773927f6f)

Thanks
Felix

### Comment 22: rudiratlos at 2025-04-18T07:53:44Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2814856858

between 2 update cycles (1-2sec) in between it's getting darker/grayer and grayer. until it gets a new update with fastest mode. right after the update, all details are crystal clear, after a second it's wearing out. all details are getting weaker. all within 1-2secs. you can see this, if you compare the aircraft symbol in the middle of screen with all pictures.

![Image](https://github.com/user-attachments/assets/656a43fa-99fa-4689-bb01-8b29a405475d)
![Image](https://github.com/user-attachments/assets/d0f0beb3-501d-4b4d-8694-031142eee82d)
![Image](https://github.com/user-attachments/assets/735219aa-d563-483c-959e-6d19244f252c)
![Image](https://github.com/user-attachments/assets/47ff47f1-c0fe-4175-a19f-af2fac3ee75b)
![Image](https://github.com/user-attachments/assets/a832b0d1-6972-4db6-b344-ec6d4932fca5)
![Image](https://github.com/user-attachments/assets/fb63d0bf-6eca-435e-9e19-bda5814d2737)


### Comment 23: lovyan03 at 2025-04-19T14:39:14Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2816732789

Hello, everyone.
I've updated the M5GFX develop branch again, this is fixed some control issues, so please take a look at this.

### Comment 24: rudiratlos at 2025-04-19T16:30:27Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2816773494

it's better now. the wearing out and the getting grayer within 2sec are gone now. But I get light black streaks, where pixels where set and reset to white

### Comment 25: lovyan03 at 2025-04-19T23:13:25Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2816900438

Inherently, EPD is not easy to control, and it is impossible to achieve perfect drawing at high speed.
Therefore, I provide several epd_mode_t and let the user choose.
The epd_fastest mode is offered as a fast mode at the cost of leaving an afterimage of the previous image, so if you want to avoid afterimages, try changing the mode.

### Comment 26: rudiratlos at 2025-04-20T08:43:41Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2817067145

I'm using platformio.ini with the following paras:
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino

But this pulls old wifi libraries for the esp8266. pls. see my conversation here
[https://github.com/espressif/arduino-esp32/issues/11256#issuecomment-2816793550](https://github.com/espressif/arduino-esp32/issues/11256#issuecomment-2816793550)
How to get the newest libraries from espressif for the ESP32_S3R8 chip?

### Comment 27: lovyan03 at 2025-04-20T08:46:41Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2817068221

It is not recommended to discuss topics that are not related to the topic.
Also, I don't know about it, so I think it would be best to ask the question somewhere else.

### Comment 28: felmue at 2025-04-20T19:25:11Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2817305335

Hello @lovyan03

for my 'non flickering' clock example I have no choice but to select `fastest` as EPD mode. I understand the implications and I am sure controlling EPD isn't easy at all.

That said, I tried with the latest M5GFX library changes and I can see a slight improvement, but unfortunately it's still not as good as when I run my clock example compared to using EPDiy library.

![Image](https://github.com/user-attachments/assets/6f05197e-76a0-49ea-bbaf-68d9a0b2a4d8)

Thanks
Felix

### Comment 29: lovyan03 at 2025-04-21T10:10:08Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2818097482

Hello, @felmue
I've adjusted the develop branch again.
I now assume that the initial memory is white, so that no processing is done the first time you try to draw white.
I think this should solve the problem.

### Comment 30: rudiratlos at 2025-04-21T14:21:40Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2818514350

I got this error in .../M5GFX/src/lgfx/v1/LGFXBase.hpp
cannot declare variable 'file' to be of abstract type 'lgfx::v1::DataWrapperT<fs::SDFS>'

### Comment 31: lovyan03 at 2025-04-22T00:45:02Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2819784513

Again, if you would like to discuss a topic that is off-topic, please create a separate issue....

### Comment 32: rudiratlos at 2025-04-22T07:17:07Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2820332213

sorry, but I'm getting this error in platformio/vsc and in arduino IDE.

### Comment 33: lovyan03 at 2025-04-22T07:18:23Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2820336925

need your code.

### Comment 34: lovyan03 at 2025-04-22T07:18:49Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2820338547

are you using # include <SD.h> ?

### Comment 35: lovyan03 at 2025-04-22T07:25:52Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2820365487

@rudiratlos
https://github.com/m5stack/M5GFX/issues/79

### Comment 36: rudiratlos at 2025-04-22T08:21:27Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2820547522

thank you very much, this was the solution

### Comment 37: felmue at 2025-04-23T06:52:29Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2823251005

Hello @lovyan03

below is the result with the latest development branch. I'd say it is now more or less identical with when using EPDiy library. Well done. Thank you.

![Image](https://github.com/user-attachments/assets/d4479cca-2c8d-4c1d-93b3-318d4fa80499)

Thanks
Felix

### Comment 38: felmue at 2025-04-29T03:48:50Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2837370874

Hello @lovyan03

just an FYI - with your latest changes (commit d805f12 April 27, 2025) to M5GFX library I get a quite dark display after M5PaperS3 waking up and before updating to the next minute which I did not get when testing with a previous version (commit 1138e93 / April 21, 2025).

![Image](https://github.com/user-attachments/assets/f2145454-28aa-4f90-914a-52f6f1986764)

Thanks
Felix

### Comment 39: lovyan03 at 2025-04-29T03:54:49Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2837376314

Thanks, @felmue
As far as I've tried it, there are no problems, but can you take a video of the startup and post it here?

### Comment 40: felmue at 2025-04-29T04:02:46Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2837384546

Hello @lovyan03

I am sorry, I misspoke, the issue already existed in the earlier version (commit 1138e93 / April 21, 2025).

The difference is that I tried my clock example without displaying the battery voltage in the lower left corner. E.g. I commented out the call to `drawBatteryVoltage();`.

Thanks
Felix

### Comment 41: lovyan03 at 2025-04-29T04:05:05Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2837387700

@felmue
Since the situation is unclear, please provide the source code that reproduces the phenomenon again.
I cannot read English, so I make mistakes in the translation and cannot understand exactly what you are saying.

### Comment 42: lovyan03 at 2025-04-29T04:40:05Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2837431026

@felmue
I just update develop branch again. please try this .

### Comment 43: felmue at 2025-04-29T05:05:16Z

Permalink: https://github.com/m5stack/M5GFX/issues/119#issuecomment-2837460497

Hello @lovyan03

that was it. It works ok for me. Thank you very much!

Thanks
Felix
