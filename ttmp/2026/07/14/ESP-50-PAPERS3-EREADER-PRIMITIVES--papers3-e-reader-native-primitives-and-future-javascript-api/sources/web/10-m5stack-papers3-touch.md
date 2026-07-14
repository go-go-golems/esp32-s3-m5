```
English
```

[![pdf-icon](https://docs.m5stack.com/assets/pdf.svg)](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static.pdf)

![](https://docs.m5stack.com/assets/guide.png)

Arduino Quick Start

### 1\. Arduino Setup

#### 1\. Arduino IDE Install

#### 2\. Arduino Board Manager

#### 3\. Arduino Library Manager

### 2\. Devices & Examples

#### Atom Voice

#### Atom-Lite / Atom-Matrix

##### Quick Start

##### Button

##### RGB LED

##### IMU

##### IR NEC

#### AtomS3R-CAM

#### AtomS3R-M12

#### Stamp-C3

#### Stamp-C3U

#### Stamp C6LoRa

##### Quick Start

##### EXT IO

#### Stamp-Pico

#### Stamp-S3

#### Stamp-S3A

#### Stamp-P4

##### Quick Start

##### Wi-Fi

#### Unit CAM

#### Unit CamS3-5MP

##### Quick Start

##### microSD

##### Web CAM

#### Unit PoE CAM

### 3\. M5Unified

### 5\. Extensions

#### Unit

##### Unit CardKB2

##### Unit Gateway H2

##### Unit ASR

##### Unit AudioPlayer

##### Unit Mini PDM

##### Unit MIC

##### Unit HBridge

##### Unit Heart

##### Unit TimerPWR

##### Unit 8Angle

##### Unit ByteSwitch

##### Unit ByteButton

##### Unit ChainBus

##### Unit OLED

##### Unit Mini OLED

##### Unit Glass

##### Unit Glass2

##### UnitV/StickV

##### UnitV2

##### Unit INA226-1A/10A

##### Unit Reflective IR

##### Unit Grove To Grove

##### Unit Pahub

##### Unit MQ

##### Unit ENV

##### Unit CO2 / CO2L

##### Unit Mini BPS

##### Unit Mini TVOC/eCO2

##### Unit Relay

##### Unit Ultrasonic-I2C

##### Unit Ultrasonic-IO

##### Unit KMeter ISO

##### Unit Finger

##### Unit Fingerprint2

##### Unit UWB

##### Unit Cat1-CN

##### Unit Pbhub v1.1

##### Unit UHF-RFID

##### Unit RFID / RFID2

##### Unit NFC

##### Unit Step16

##### Unit AIN4-20mA

##### Unit RF433

##### Unit EXT.IO2

##### Unit ACSSR/DCSSR

#### Atom DTU

##### Atom DTU LoRaWAN-X

##### Atom DTU NBIoT2

##### Atom DTU NBIoT2 v1.1

#### Base

##### Base Dual 16340

##### Base LAN PoE v1.2

#### Cap

##### Cap LoRa868/LoRa-1262

#### StamPLC

##### StamPLC AC

##### StamPLC PoE

#### Tab5

##### Tab5 Keyboard

#### IoT

##### SwitchC6

#### Accessories

##### Servo 180°/360° Kit

### 6\. Applications

#### AWS IoT Core

##### AWS IoT Core Arduino

#### EzData 1.0

##### EzData 1.0 Arduino

## PaperS3 Touch Screen

PaperS3 touch screen related APIs and example programs.

## Example Program

### Compilation Requirements

- M5Stack Board Manager version >= 2.1.4
- Board selection = M5PaperS3
- M5Unified library version >= 0.2.5
- M5GFX library version >= 0.2.7

1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28

```cpp
#include <M5Unified.h>
#include <M5GFX.h>

m5::touch_detail_t touchDetail;
uint16_t color;

void setup() {
  M5.begin();
  M5.Display.setRotation(0);
  M5.Display.setFont(&fonts::DejaVu40);

  color = random(65535);

  Serial.begin(115200);
  Serial.println("Start drawing!");
  M5.Display.print("Start drawing!");
}

void loop() {
  M5.update();
  touchDetail = M5.Touch.getDetail();

  if (touchDetail.isPressed()) {
    Serial.printf("x:%d, y:%d\r\n", touchDetail.x, touchDetail.y);
    color = (color + 5) % 65536;
    M5.Display.fillCircle(touchDetail.x, touchDetail.y, 15, color);
  }
}
```

The main function of this program is to output the coordinates of the touch point to the computer via serial when a finger touches the screen, and draw circles with different grayscale colors at the touch point. The program reads only one touch point, but you can also use the APIs below to develop two-point touch functionality for the PaperS3.

![](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/PaperS3_Arduino_touch.jpeg)

## API

The PaperS3 touch screen uses the `Touch_Class` from the `M5Unified` library. For more related APIs, please refer to the following documentation:

- [M5Unified - Touch Class](https://docs.m5stack.com/en/arduino/m5unified/touch_class)