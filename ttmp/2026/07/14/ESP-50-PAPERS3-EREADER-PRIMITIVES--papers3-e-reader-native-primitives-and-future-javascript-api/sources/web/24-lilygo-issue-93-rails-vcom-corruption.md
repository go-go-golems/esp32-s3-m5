# Xinyuan-LilyGO/LilyGo-EPD47 issue #93: T5-4.7 Plus S3 hardware issues & partial clear corruption

- URL: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93
- State: `closed`
- Created: 2023-03-26T12:03:49Z
- Updated: 2025-03-21T02:42:12Z
- Author: `AdriVerhoef`

## Issue body

Hello all, I got my T5-4.7" S3 a week ago and it's a bit of a let down. Straight out of the box I notice a few lines of vertical dead pixels, ghosting on writing text and each time the screen refreshed the PCB made a high pitched noise. The repair demo did not fix the dead pixels. For the noise I expected to swap some capacitors to fix the issue, but it was not that easy.

I probed the +22V and -22V outputs and noticed high ripple (~3.5Vpp) on the +22V each time the screen updated:
![20230321_210735](https://user-images.githubusercontent.com/22544605/227772079-8f4d8244-4e4d-49c6-b1f2-479a8a1f2f75.jpg)

Changing the output capacitors for X7R 50V 1uF reduced this a bit, but not much. Adding additional capacitance wasn't helping either. On further probing I discovered that the +15V output of the LM7815 was oscillating. Adding more capacitance (2x22uF 25V X5R) made things better, but the output still had high ripple.


In the schematic I found that the second op-amp in the dual op-amp package was not terminated properly. The second opamp's inputs where left floating and most likely oscillating:
![opamp_first](https://user-images.githubusercontent.com/22544605/227772983-daac7815-aaf6-4cc4-acac-67a20b032a9e.png)
I soldered the opamp inverting input and the output together and added a wire to the first op-amp's non-inverting input, so the op-amp was in unity gain configuration, just like the first opamp. I did not connect both op-amp outputs as this is not a good idea, since both op-amps can have a different offset.
![opamp_modded](https://user-images.githubusercontent.com/22544605/227773013-1dadecd4-6ff6-4fe5-9dfa-5ced2b8e75dd.png)

With the op-amp properly terminated and additional capacitance on the +15V output the display was now silent on each update and text printing on wrong locations was eliminated.
I also resoldered the pins of the ESP32S3, as I had to use a microscope to confirm that there was at least some solder on some pads. This did not improve functionality, so the pads where properly connected before resoldering even though it was discouraging how little solder was on some pads:
![20230322_164752](https://user-images.githubusercontent.com/22544605/227774063-f221f787-0e02-4fa2-854e-e33114a2de1b.jpg)


On the diy-epd support I also found a comment that the VCOM should be buffered with an additional capacitor for improved performance, so I added a 22uF capacitor with a 10 Ohm series resistance in the output trace of the opamp output. I first scraped the soldermask from the output trace, then cut it, so the 10 Ohm is in series to the display VCOM input and soldered the 22uF capacitor to the resistor and soldered it to the ground plane. Most op-amps do not like driving a high capacitance on the output, so the resistor is used to isolate the capacitance from the output and keep the output stable. Though this modification seems to have the least impact on the functionality of the display.

All modifications:
![20230326_130134](https://user-images.githubusercontent.com/22544605/227771956-3e2a7047-1b04-48d7-8e52-8540150344b9.jpg)
The 47uF elco is not necessary, but is a left-over from testing if additional capacitance solved the issues left.

I still have the issue that the display shows a block around the parameter which is updated due to the rest of the display slowly getting corrupted/darkening when doing a partial clear:

I've changed my VCOM voltage by adjusting the potentiometer to ~-1.1V which reduced the corruption significantly, but still with each area clear the previously displayed data corrupts more and more over time, while the refreshed data is clear.

The epd_push_pixels function does not seem to write a complete block of pixels. The last row being cleared does not seem to write a 1 or 0 to the screen, but most likely something random in a buffer.
![20230326_130639](https://user-images.githubusercontent.com/22544605/227772700-885a7165-0c54-43f3-8074-40c555e793f6.jpg)
This causes some residue to remain after clearing:
![20230326_130410](https://user-images.githubusercontent.com/22544605/227773115-66b8d1ab-cab0-4c4c-ae47-96e85b5f627e.jpg)

So I hope someone has a solution for the partial clears corrupting the full screen as I suspect it's a software problem. Erasing and rewriting the full screen using the framebuffer is a solution, but I hope it's not required doing so after a single parameter changes.

Here is a code snippet for a printf like function in order to make the examples a bit more clean:
```
#define TEXT_BUFF_SIZE 128
void printLine(int32_t x, int32_t y, const char * format, ... )
{
    static char local_buff[TEXT_BUFF_SIZE] = {0};
    int32_t cursor_x = 0;
    int32_t cursor_y = 0;
    va_list args;

    // epd_poweron();
    memset(local_buff, 0, TEXT_BUFF_SIZE);
    va_start (args, format);
    vsnprintf(local_buff, TEXT_BUFF_SIZE, format, args);

    cursor_x = x;
    cursor_y = y + FiraSans.advance_y + FiraSans.descender;
    writeln((GFXfont *)&FiraSans, local_buff, &cursor_x, &cursor_y, NULL);
    va_end (args);
    // epd_poweroff();
}
```
Usage:
```
    epd_poweron();

    printLine(200, 250, "➸ 16 color grayscale  😀 \n");
    delay(500);
    printLine(200, 300, "➸ Use with 4.7\" EPDs 😍 \n");
    delay(500);
    printLine(200, 350, "➸ High-quality font rendering ✎🙋");
    delay(500);
    printLine(200, 400, "➸ ~630ms for full frame draw 🚀\n");
    delay(500);

    epd_poweroff();
```

## Comments

### Comment 1: random-0110-dude at 2023-04-10T22:54:23Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1502436854

@AdriVerhoef Thank you very much for sharing!

Could you please provide more close-up photos, what exactly did you solder to silence the board?

### Comment 2: AdriVerhoef at 2023-04-11T17:22:43Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1503806892

![20230411_184951](https://user-images.githubusercontent.com/22544605/231237354-f929e789-e9c9-4601-adb5-1bbd0c1a9960.jpg)
![20230411_184951_2](https://user-images.githubusercontent.com/22544605/231237484-13528937-0d62-4ad1-8dd9-3d9745733721.jpg)

Mods:
- Orange: 2x 22uF + 1x47uF, but as stated earlier, that didn't improve the ripple voltage as the oscillation was already resolved.
- Red: The connections of the opamps as indicated in the first post to create a voltage follower/unity gain configuration. Added a 100nF to buffer the VCOM voltage from the potentiometer.
- Calibrated the VCOM voltage to -1.1V
- Dark Green: Added an additional 1uF to the positive output of the symmetrical boost converter
- Purple: Changed the pull-ups on the enable from 100k to 10k, since the voltage on those pins was not stable due to the high resistance of the original resistors and the input current of the inputs.

![20230411_184959](https://user-images.githubusercontent.com/22544605/231238792-f26539d1-99ed-4e96-a0cc-389000f68ba5.jpg)
![20230411_184959_2](https://user-images.githubusercontent.com/22544605/231239020-17c9acb1-e8de-4bf6-b2ef-c20158e44983.jpg)
Mods:
Cut the trace at the green line and scrape the solder mask from the trace, so the resistor can be soldered. For the buffer capacitor the ground plane needs to be solderable, so remove the soldermask at an appropriate distance from the trace.

So do you have similarly problems with the board? Noise and ghosting?

I've also discovered that the STR_IO0 signal is connected to two pins on the ESP32S3. Would have been nice if the STR and IO0 functionality had been separated, so the shift register and push-button where not connected. However the connecting trace seems to be under the ESP32S3 module or on the other side of the board below the adhesive.

![STR_IO0](https://user-images.githubusercontent.com/22544605/231240694-595e5c7f-d5b8-472f-8c73-f5e7876b57c1.png)


### Comment 3: random-0110-dude at 2023-04-11T18:46:18Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1503911075

@AdriVerhoef Thanks!

> So do you have similarly problems with the board? Noise

Noise. Especially when updating the screen. Gone only after `epd_poweroff_all();`.

I did some modifications already, before your answer. Here they are:

![image](https://user-images.githubusercontent.com/129990954/231254950-4fe3cd63-c96b-4624-b949-713b242df68e.png)

![image](https://user-images.githubusercontent.com/129990954/231256187-1e88fbb3-0559-4bed-9046-fea637009856.png)

- I connected 2 with 5 on U5 / LM358. Noise goes a bit down.
- I connected 7 with 6 on U5 / LM358. Noise goes a bit down
- I added 2x22pF ceramic capacitors, because they were lying around. Noise goes a bit more down .
- I finally added 47uF electrolytic capacitor (lying around as well). Noise gone completely.
- **ADDED EDIT:** I could then unsolder 2x22pF, but decided to leave them as is.

By the way, your images show connection between 3 and 5, **is this intentional? Or did I mix things up?**

I connected all these without any idea what I'm doing (except maybe that I'm filtering some pulsations). No idea about opamps as well. Noise gone.

**Are my modifications safe for the screen?**

I don't own an oscilloscope, but I measured a couple of voltages, here they are:
![image](https://user-images.githubusercontent.com/129990954/231258797-556cfa6f-87d5-49ff-86c1-945789695f1a.png)



### Comment 4: AdriVerhoef at 2023-04-11T19:07:16Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1503956912

I see I've connected pin 2 in my example of the schematics and pin 3 on my board. It doesn't really matter, the second opamp is now in series with the first, whereas on my board both opamp inputs are connected to the potentiometer.

Just ensure that the leads of your capacitors do not make contact with the rest of the circuit and you should be fine. The 22pF don't do much, but won't harm either. The 47uF is 47000000pF so that adds the missing capacitance the +15V regulator requires, as it is oscillating when the screen is updating.
You do need to reduce the VCOM voltage by turning the potentiometer, the voltage should be around -1.1V ~ -1.3V. When this is reduced, the screen corruption during a partial clear and draw is significantly reduced. However I've programmed my board with the weather example which updates the screen fully, as I still found it too visible.

### Comment 5: random-0110-dude at 2023-04-11T19:18:34Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1503969421

@AdriVerhoef Thanks!

Yep, I double-checked that there are no solder traces left, and the capacitors remain isolated.

> You do need to reduce the VCOM voltage by turning the potentiometer, the voltage should be around -1.1V ~ -1.3V.

Is VCOM a voltage between 3 and GND? _Sorry for the dumb question_


### Comment 6: AdriVerhoef at 2023-04-11T19:35:28Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1503986985

VCOM is the common voltage of the Eink display, the potentiometer is connected to R10, which is connected to -15V. I do not know the resistance of the potentiometer(U4 in the schematic), but it creates a voltage divider, so the output voltage can be adjusted from 0V(GND) and a negative value. The opamp buffers this voltage as the screen will draw more current than a voltage divider can supply without being affected.
![afbeelding](https://user-images.githubusercontent.com/22544605/231267970-16ba130f-df7d-4889-81d7-323609fca896.png)
![20230411_184951_3](https://user-images.githubusercontent.com/22544605/231269697-1535198e-165d-494f-86a9-ee31671bf803.jpg)

If you use a screwdriver and turn the potentiometer slightly, the voltage you measure on pins 2 and 5 will change. Just make sure that the display's power converters are on or the measured voltage will not be correct.

### Comment 7: random-0110-dude at 2023-05-01T14:24:15Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1529772043

@AdriVerhoef And one more question - did you find any issues with RTC?

Mine resets to zeros every time I reboot the board. Is it intended? Does your work the same?

### Comment 8: AdriVerhoef at 2023-05-01T16:45:14Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1529935739

I have not tested the RTC, since I'm not using it. Sounds like the back-up battery does not hold any charge or your code might reset the device on initialization. Have you measured if the battery voltage is ok?

### Comment 9: random-0110-dude at 2023-05-03T13:11:34Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1533008019

@AdriVerhoef The battery shows 2.66V between its PCB pins.

As far as I know, I don't reset the RTC intentionally. I've skimmed through https://github.com/lewisxhe/PCF8563_Library/tree/master/src , but did not find any code that resets it during initialization.

### Comment 10: homonto at 2023-06-13T07:10:16Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1588678659

I've just bought this board with the display and EVERYTHING you describe is completely true in my case as well (S3 version): the noise, the partial grayout etc.
I think that goes without saying: "you pretend to pay me - I pretend to be a good board" - eh...

btw when you power this board using the BAT connector, when. you enter epd_poweroff_all() it still draws about 270uA that is way too much - if I am going to use this board I would add some circuit to disconnect the power completely and only turn ON periodically (i.e. every 3min or so)

on top of that, the library provided by Lilygo is full of errors - I am wondering if other libraries would work, i.e. TFT_eSPI - that would be amazing

### Comment 11: brianwyld at 2023-08-22T07:43:43Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1687646861

Some comments after using the board for a bit (under micropython):

- the button STR_IO0 is essentially not usable as it is used for the epaper control signals (via the shift reg) -> if you try to use it as a general button then the epaper updates don't work... I'm thinking about cutting the trace and wiring the button to a different IO to be able to use it for the application
- bizarrely I can't get the button IO inputs to generate an IRQ (using micropython, but the code works on other ESP32S3 boards...)
- 50% of the the time when I start the epaper driver, the board browns out (powered by USB) and resets -> I need to try some of your 'add caps' fixes
- epaper driver code is somewhat buggy, specially for partial updates -> need to fix that code for the last line writiing
- power consumption : not fab, I suspect because the buck-boost for the epaper has no way to control its 'shutdown' input
- for the micropython command line, I ended up soldering a header to the GND/RX/TX on the 40 way header to access the UART, instead of using the native USB uart -> doesn't disconnect from the PC at every reset, and lets you see the reboot logs (this is how I saw the brownout reset for example)

definitely feels like a friday afternoon design...

### Comment 12: brianwyld at 2023-09-07T10:15:29Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1709888585

update:
- button IRQ : use esp-idf-4.4.1 or later, there is a bug for the ESP32S3 in 4.4
- epaper driver : in need of debug/refactoring to seperate versions for ESP32 / ESP32S3 as they use a different lib to transfer the data (I2S on ESP32, esplcd on ESP32S3)?


### Comment 13: fuef at 2024-11-16T18:03:07Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2480691277

> * epaper driver : in need of debug/refactoring to seperate versions for ESP32 / ESP32S3 as they use a different lib to transfer the data (I2S on ESP32, esplcd on ESP32S3)?

Forking `esp32s3` branch gives more or less clean ESP32S3 variant, doesn't it?

Screen ghosting with partial refreshes appears to be indeed a big issue. I am slightly confused by timing partial refreshes. It comes a way more than 400 ms, no matter how small the refresh area is. Could it mean that partial refresh implementation is broken?

Stepping away from this board, M5Paper seems to be using exactly the same EPD. I cannot spot much use of partial refresh on [the reference code](https://github.com/m5stack/M5EPD/blob/main/src/M5EPD_Driver.h). The interesting bit is [L21](https://github.com/m5stack/M5EPD/blob/main/src/M5EPD_Driver.h#L21) onwards which describes various modes of drawing on EPD.

Now, the code could be detached from the reality, if no example is using a partial refresh mode. The most exciting piece is [on page 3 of this paper](https://wiki.diustou.com/cn/w/upload/c/c4/E-paper-mode-declaration.pdf) with the original file name 800-1101 REV01 AF 16 TONE GRAYSCALE 5-BIT WAVEFORM FLASH FILE PRODUCT SPECIFICATION.

A particular unidentifiable epaper assumes partial refreshes follow up in a peculiar order.

Am I right, the current driver is in a total denial of the intended refresh sequence? Aren't we mis-using `draw_grayscale` primarily? How do we know if M5Paper engineers have borrowed the right code?

Well, I must have bet on the wrong horse (T5-4.7 Plus S3). On the other hand, M5Paper comes twice as expensive, even without an S3 designator.


### Comment 14: Tasshack at 2024-11-20T12:49:05Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2488500117

I have managed to get it working with latest version of the [EPDiy library](https://github.com/vroland/epdiy/) by removing the shift register from board and connecting some unused but populated gpios directly to the EDP display control pins to utilize onboard LCD prepherial correctly.
Residue left of the images issue has been completely solved by using the latest library along with correct implementation of the ESP32-S3 LCD prepherial and also it is extremely fast now like the M5Paper. In fact, it is so fast that full and partial updates are just a flashing screen now.

I have also tied touch interrupt pin to another rtc capable gpio and did not used gpio 0 for anything else within the mod which was connected to the shift register previously. Now, I can wake up the board using touch and pressing top middle button does not affect screen refreshing anymore.

Here is how you do it;

![Capture](https://github.com/user-attachments/assets/76d68e7a-4b8d-440c-94af-5da5acf81ff8)
![51x9DjYLOKL](https://github.com/user-attachments/assets/24829ece-138e-4565-af45-8534dc1ef045)

_Please note that printed pin names on the board are different from the schematic and documentation but I am using printed names on the PCB as reference below._

- **Green**: Pin 2 (GPIO13) -> Pin 13 (PWR_EN)
- **Orange**: Pin 3 (GPIO12) -> Pin 4 (EPD_LE)
- **Yellow**: Pin 14 (EPD_STV) -> SCL (GPIO45)
- **Blue**: Pin 12 (EPD_MODE) ->MISO (GPIO48)
- **Purple**: Pin 11 (EPD_OE) -> CS (GPIO39)
- **Brown**: GPIO47 (47) -> MOSI (GPIO10) _*Optional, if you want to wake the mcu using touch*_

I really think this is how the board should be designed in the first place. Some related issues about this;
https://github.com/vroland/epdiy/issues/317
https://github.com/vroland/epdiy/issues/273
https://github.com/vroland/epdiy/issues/203
https://github.com/martinberlin/lv_port_esp32-epaper/issues/11

You also need modified version of the `epd_board_lilygo_t5_47.c` file since it requires some changes to make it work with the modded hardware.
[epd_board_lilygo_t5_47.zip](https://github.com/user-attachments/files/17833173/epd_board_lilygo_t5_47.zip)

Also default waveform for this display is not correct in the library and you should be using [ED047TC2](https://github.com/vroland/epdiy/blob/main/src/waveforms/epdiy_ED047TC2.h) waveform since it is the one used with the [factory firmware](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/blob/esp32s3/Waveform_header/ED047TC2.h) and the board has shipped with also [provided by Lilygo to EPDiy project](https://github.com/vroland/epdiy/blob/main/src/builtin_waveforms.c#L5).

`epd_init(&epd_board_lilygo_t5_47, &ED047TC2, EPD_LUT_64K);`

Partial update is also works extremely well with using this waveform instead of the default `ED047TC1`. I think this is because ET047TC1 (Flexible backing) is not exactly same with ED047TC1 (Hard backing) which is the non S3 version of this board was shipped with.
_Unfortunately, there are only `MODE_DU`, `MODE_GC16` and `MODE_GL16` update modes are available with this waveform._

And here are the files you need to overwrite if you still want to use the official arduino library after this hardware modification.
[ed047tc1.zip](https://github.com/user-attachments/files/17831585/ed047tc1.zip)

After all of this along with the modifications from @AdriVerhoef now I can get the performance similar to a M5Paper board with the exact same e-paper display but please note that M5Paper has a dedicaded epd driver onboard (IT8951E) which has a lot of proprietary stuff happening inside it to make the magic of driving these displays happen so you will never get it's performance using an ESP32 or ESP32-S3.

Thanks...

### Comment 15: lewisxhe at 2024-11-21T01:08:18Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2489857963

@Tasshack Thank you for your contribution. I will test it according to your method. If it works well, I may update it according to this hardware connection later.

### Comment 16: random-0110-dude at 2024-11-21T10:19:56Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2490712176

@lewisxhe If you will do a new revision of the board, could you please also look into #98 as well?
Currently I have to do two modifications:
* https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-1503806892 to silence the board
* https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/98#issuecomment-1715584471 to prevent brownout on boot and strange battery behavior: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/98#issuecomment-2267149084

### Comment 17: lewisxhe at 2024-11-22T01:01:45Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2492659599

Subsequent revisions may be based on the model in the picture, and the power supply may be replaced. I will update the progress here later.
![image](https://github.com/user-attachments/assets/edea0efa-95e5-4a7c-aced-162a9944bda4)


### Comment 18: Tasshack at 2024-11-22T01:20:41Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2492679281

Looks very nice...

### Comment 19: random-0110-dude at 2024-11-22T13:38:38Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2493791338

@lewisxhe looks really nice! Does it mean that it can be used with any magsafe charger?

Some more items for wishlist:
* please move those two molex connectors a bit apart (up to 5-7mm) from each other to make room for cables
* please add BOARD_I2C connector (always powered, unlike molex ones that require `epd_poweron()`) - for extra versatility
* please consider adding APDS9960 sensor
* please add more buttons, with a capability to wake the ESP. 5 buttons total (BOOT+RST+3 custom) will be enough IMO
* fix those pullups on USB-C (#116) (if not already)

### Comment 20: lewisxhe at 2024-11-25T03:01:09Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2496577379

@random-0110-dude Thank you for your suggestion. Please refer to the official website for the design of this model, because it has been designed and I cannot influence it. I can only improve it according to your feedback in the subsequent design.

### Comment 21: random-0110-dude at 2024-11-27T07:11:42Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2503075693

@lewisxhe well, my suggestions are also valid for the current (V2.4) model. Are you going to sell it alongside the new 'MagSafe' one?

### Comment 22: lewisxhe at 2024-11-27T07:20:22Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2503096880

@random-0110-dude  Currently V2.4 and T5-Epapaer-Pro are of the same design. We will listen to relevant opinions and then summarize and make changes.

### Comment 23: lewisxhe at 2024-11-27T07:21:47Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2503098954

> @lewisxhe looks really nice! Does it mean that it can be used with any magsafe charger?
>
> Some more items for wishlist:
>
> * please move those two molex connectors a bit apart (up to 5-7mm) from each other to make room for cables
> * please add BOARD_I2C connector (always powered, unlike molex ones that require `epd_poweron()`) - for extra versatility
> * please consider adding APDS9960 sensor
> * please add more buttons, with a capability to wake the ESP. 5 buttons total (BOOT+RST+3 custom) will be enough IMO
> * fix those pullups on USB-C ([usb-c pull down on B5 missing #116](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/116)) (if not already)

High-speed wireless charging cannot be used. Long-term use will cause severe heat and damage. Only standard wireless charging can be used.
The wish list he mentioned may require another revised version without a shell to achieve the functions he wants. The Pro version currently being made does not meet its needs.
1. There is only one Qwiic interface on the new version. If more are needed, a replacement lora function module on the back needs to be designed.
2. Same as above.
3. APDS9960 is a sensor that requires a separate reserved design for the placement position. There is no reservation on the screen glass of the new version, and it can only be expanded through the Qwiic interface.
4. There are only 2 customizable (GPIO0, GPIO48) buttons on the new version, two function buttons RST and PWR buttons. No more reservations.
5. Fixed.

The above is the reply of Design T5-Epaper-Pro Design Yes.

### Comment 24: fuef at 2024-12-01T20:40:47Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2510242840

> I have managed to get it working with latest version of the [EPDiy library](https://github.com/vroland/epdiy/) by removing the shift register from board and connecting some unused but populated gpios directly to the EDP display control pins to utilize onboard LCD prepherial correctly.

Thank you very much @Tasshack for your perseverance! By reading @martinberlin's refusal to invest into supporting Lilygo T5 Epaper version 2.3, I gave up and finalised my setup in a case, made of sustainable materials. The case design wouldn't let me de-solder 74HCT4094D by now to validate the very promising fixes :o(.

I might have an emerging use case for the touch screen, but it will definitely require much more responsive refresh rates.

Out of curiosity, what's your partial refresh's timings, please? Lilygo claims full screen refresh to be below 500 ms. In my experience, with an ESPHome overhead, it's about 1.6 seconds. A countdown kitchen timer is still okay-ish, even when updated once every three seconds, but it has to be a reliable partial refresh, not damaging the screen permanently.

### Comment 25: fuef at 2024-12-01T21:11:00Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2510253052

> @random-0110-dude Currently V2.4 and T5-Epapaer-Pro are of the same design. We will listen to relevant opinions and then summarize and make changes.

Pin-wise [V2.4](https://cdn.shopify.com/s/files/1/0617/7190/7253/files/T5-S3V4.7_1.jpg) is closer to [V2.3](https://cdn.shopify.com/s/files/1/0617/7190/7253/files/T5-S3V4.7_8a9a24db-26f2-4f6e-b3c7-d1293de0e30b.jpg) than [T5-Epapaer-Pro](https://cdn.shopify.com/s/files/1/0617/7190/7253/files/T5-4_10.jpg).

Stating V2.4 and T5-Epapaer-Pro are of the same design is misleading 8o|.

Not every housewife would happen to have a soldering kit in the kitchen, and not every one will be brave enough to use it to conserve battery power by desoldering the green LED. To continue on the future design improvements, would it be wise to make the green LED easily disconnectable by cutting off a shunt?

### Comment 26: martinberlin at 2024-12-01T22:01:46Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2510269626

I’m sorry about voting down to support Lilygo S3 efforts into epdiy. In fact they did got the S3 working first. But the fact that they still use kind of epdiy v2 without really making the efforts to use S3 LCD full signals to drive the whole hardware really put me down.
Please check my analysis here:
https://youtu.be/r4-dz_x7K4k?si=8xecWLySTkGISP_Z

REMARKS: Lilygo updated this situation now and their new S3 board should be fully compatible with epdiy v7!

### Comment 27: lewisxhe at 2024-12-03T05:44:40Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2513612494

Hello everyone, we have discussed and decided to directly use the [epdiy v7](https://vroland.github.io/[epdiy](https://vroland.github.io/epdiy-hardware/build/boards/epdiy-v7_schematic.pdf)-hardware/build/boards/epdiy-v7_schematic.pdf) hardware design, the same driver method and GPIO. What do you think about this?

### Comment 28: random-0110-dude at 2024-12-03T09:27:09Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2513981547

@lewisxhe I think this is a great idea that will allow maximum compatibility and will probably boost your sales :) A **reliable** and **inexpensive** board will definitely be a hit.

In the same time, I personally would not like to see CH340, and would prefer having direct USB access to ESP32-S3, so the board would not lose functionality (USB-MSC, USB-HID etc) that would happen if you would expose only UART over USB. [Other boards](https://www.tindie.com/products/fasani/epdiy-v7-816-bit-parallel-epaper-controller/) have two USB-C ports; I'm not sure if I want two, maybe one is enough, but it should be the one with direct ESP access.

RTC is a must. microSD card slot - also very much so.

Please add more buttons! Five (including RST & BOOT). Also, **all** these buttons should be able to wake ESP from deep sleep.

Please expose both I2C lines available on the ESP. One line (probably the one with RTC+touch) should always be powered, while the second line should be controlled like in v7. **Both** I2C lines should have pin headers available (or at least holes for them).

Probably you could keep ['your'](https://aliexpress.com/item/4000473537275.html) sockets (to keep compatibility) but please expose the same lines via 2.54" pin headers as well.



### Comment 29: lewisxhe at 2024-12-03T09:32:12Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2513992828

1. No USB2TTL or any serial port bridge will be used, but the USB of ESP32S3 will be used directly as the download and debug interface

2. RTC and MICROSD are reserved

3. Five buttons need to be determined based on the subsequent design, because a LoRa module needs to be compatible

4. Public I2C is a must, and it is always powered

5. The second I2C depends on whether it can be enabled, because LoRa is required. I will follow up on the hardware design side. After the schematic diagram is modified, I will send it to you for review.
Perhaps you can switch freely between LoRa or I2C, which is a better idea, because not everyone needs LoRa.

### Comment 30: random-0110-dude at 2024-12-03T09:41:06Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2514013122

> Perhaps you can switch freely between LoRa or I2C, which is a better idea, because not everyone needs LoRa.

This is a great idea!

I would like to see this board as a good starting point for both your own DIY project and ready-to-use inexpensive e-ink display for ESPhome etc. For this (DiY), I would also like to see a larger case/shell **option** so that one will be able to put a few I2C modules inside alongside the battery. Ideally, you can sell this shell as an option/bundle on amazon/aliexpress/whatever, but even having just files for 3D printing will be great :)

### Comment 31: G6EJD at 2024-12-03T10:33:46Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2514163758

There are no dedicated I2C pins on the ESP32 every pin below 33 can be programmed as an I2C and the whole idea is it's a bus, so no need for a separate port...

### Comment 32: lewisxhe at 2024-12-05T08:27:23Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2519603956

Ladies and Gentlemen, this is the schematic diagram of the modified version according to the design of EPDIY V7, retaining the original GPIO, please check it out. Comments are welcome.

[T-EPD47-PRO V1.0.pdf](https://github.com/user-attachments/files/18019780/T-EPD47-PRO.V1.0.pdf)


### Comment 33: martinberlin at 2024-12-05T10:39:02Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2519932862

This looks so much better @lewisxhe  I will find sometime to review it in the next days but so far I see all control lines there in the S3 MCU:

```C
static lcd_bus_config_t lcd_config = {
    .clock = CKH,
    .ckv = CKV,
    .leh = LEH,
    .start_pulse = STH,
    .stv = STV,
    .data[0] = D0,
    .data[1] = D1,
    .data[2] = D2,
    .data[3] = D3,
    .data[4] = D4,
    .data[5] = D5,
    .data[6] = D6,
    .data[7] = D7,
}
```

![comparison](https://github.com/user-attachments/assets/00cf7f88-2e45-4d1b-b4dd-0f27e0ffdb20)

I guess it will be fairly easy to create  a new board in epdiy and add support to this one. I can do it will send you an email later. Thanks for the update

### Comment 34: lewisxhe at 2024-12-06T00:54:34Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2521847742

@martinberlin   Well, any suggestions you may have, we are listening.

### Comment 35: martinberlin at 2024-12-06T07:17:16Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2522300292

You can try it already. Since it's very similar to v7 but with just 8 data channels it should work out of the box (Unless I miss something)
https://github.com/vroland/epdiy/pull/383
Just clone epdiy, switch to branch:   [new/lilygo-s3](https://github.com/vroland/epdiy/tree/new/lilygo-s3)
Using that new display then you could render the Dragon demo in your new board.  Just tell if it does something. For me it builds without errors but I cannot test it since I don't have the hardware.

> Well, any suggestions you may have, we are listening.

So far the PCB looks very nice and the fact that has LoRa and should be LVGL capable if using touch, can make it a very nice fun toy for a slow chat :P
Honestly what I didn't like in your first S3 versions is that the display was flexible and for me, it had some dead rows, I've got 2 of them and had the same result (Maybe a bad batch). But I guess by now you would have that sorted out since it was not only me who had this issue.
Also now the problem with the sound that the PCB generated in initial versions should be gone with a professional dedicated PMIC to generate the voltages. It's also fully compatible with v7 since uses same IO expander and the TPS65185. This is cool hardware to play with but of course makes the PCB costs go a bit higher.

UPDATE: Take care with the Layout design around TPS65185 because it's a bit picky. That's why I try not to touch it and I copy Vroland layout even if I do sometimes different versions of the PCB.  The fabricant Texas Instruments has a forum where you can submit your design and get it analyzed by their tech team (For free as far as I know)

2 nd advice:  In my humble opinion if you use a 16 Channel IO expander like PCA9535 then it would be cool to at least expose that pins in the PCB (Or at least some). If you are not going to do that makes little sense to use such a hardware just to control the slow signals and leave 8 nice additional IOs disconnected.

### Comment 36: lewisxhe at 2024-12-07T06:38:22Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2524970618

@martinberlin Thank you. We are just discussing it now, and there is no hardware to test it.

Thank you for your reminder. I will forward it to the hardware engineer and let him see your proposal.

We will send you samples after we have preliminary samples.

### Comment 37: github-actions[bot] at 2025-01-07T02:35:26Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2574278955

This issue is stale because it has been open for 30 days with no activity.

### Comment 38: random-0110-dude at 2025-01-07T08:47:20Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2574709341

@lewisxhe I'm also happy with the design. But still I'd like to see more buttons, and wired in a way that it would be possible to wake the ESP from deep sleep.

Is it possible to add them?

### Comment 39: github-actions[bot] at 2025-03-07T02:39:31Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2705387237

This issue is stale because it has been open for 30 days with no activity.

### Comment 40: github-actions[bot] at 2025-03-21T02:42:11Z

Permalink: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93#issuecomment-2742109868

This issue was closed because it has been inactive for 14 days since being marked as stale.
