[![pdf-icon](https://docs.m5stack.com/assets/pdf.svg)](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static.pdf)

![](https://docs.m5stack.com/assets/guide.png)

UiFlow Tutorial

### UiFlow1 Development Guide

#### IDE Layout Introduction

#### Project Management

##### File Mangement

#### Use LTE network

##### Module COMX LTE

#### UIFlow 1.x Desktop IDE

### UiFlow1 Blockly

#### Event

##### Event

#### UI

##### Screen

#### Unit

##### Unit TMOS PIR

##### Unit Joystick2

##### Unit TimerPWR

##### Unit DMX

##### Unit Puzzle

##### Unit CO2

##### Unit CO2L

##### Unit CardKB

##### Unit ENV

##### Unit ToF

##### Unit ToF4M

##### UnitV/StickV V-Function

##### Unit RFID2

##### Unit Ultrasonic

##### Unit Ultrasonic-IO

##### Unit 8Angle

##### Unit 2Relay

##### Unit 4Relay

##### Unit 8Encoder

##### Unit 8Servos

##### Unit Accel

##### Unit ACSSR

##### Unit AC Measure

##### Unit ADC

##### Unit ADC v1.1

##### Unit AIN4-20mA

##### Unit AMeter

##### Unit Angle

##### Unit ASR

##### Unit BLDC Driver

##### Unit Mini BPS

##### Unit Mini BPS v1.1

##### Unit Button

##### Unit Buzzer

##### Unit CAN

##### Unit CatM

##### Unit CatM GNSS

##### Unit Color

##### Unit DAC

##### Unit DAC2

##### Unit DCSSR

##### Unit DDS

##### Unit DigiClock

##### Unit DLight

##### Unit Dual Button

##### Unit Earth

##### Unit Encoder

##### Unit ENV-Pro

##### Unit ExtEncoder

##### Unit EXT.IO

##### Unit EXT.IO2

##### Unit Fader

##### Unit Fan

##### Unit Finger

##### Unit FlashLight

##### Unit Gesture

##### Unit Glass

##### Unit Glass2

##### Unit GPS

##### Unit Grove to Grove

##### Unit Hall

##### Unit HBridge

##### Unit Heart

##### Unit ID

##### Unit Mini IMU

##### Unit Mini IMU-Pro

##### Unit IR

##### Unit RS485-ISO

##### Unit Joystick

##### Unit Key

##### Unit KMeter

##### Unit KMeter-ISO

##### Unit Laser RX

##### Unit Laser TX

##### Unit LCD

##### Unit Light

##### Unit Limit

##### Unit LoRaWAN470

##### Unit LoRaWAN868

##### Unit LoRaWAN915

##### Unit LoRaE220-920

##### Unit LoRaE220-433

##### Unit Makey

##### Unit MIC

##### Unit Mini CAN

##### Unit Mini OLED

##### Unit Mini Scales

##### Unit MQTT

##### Unit NBIoT

##### Unit NBIoT2

##### Unit NCIR

##### Unit NCIR2

##### Unit Neco

##### Unit OLED

##### Unit OP90/180

##### Unit Pahub

##### Unit Pbhub

##### Unit PIR

##### Unit PoESP32

##### Unit QRCode

##### Unit Reflective IR

##### Unit Relay

##### Unit RF433R

##### Unit RF433T

##### Unit RGB

##### Unit RGB LED

##### Unit Roller485

##### Unit RS485

##### Unit RTC

##### Unit Scales

##### Unit Servo

##### Unit SSR

##### Unit Synth

##### Unit Thermal

##### Unit Thermal2

##### Unit Trace

##### Unit Tube Pressure

##### Unit Mini TVOC/eCO2

##### Unit RFID-UHF

##### Unit UWB

##### Unit Vibrator

##### Unit VMeter

##### Unit Watering

##### Unit Weight

##### Unit Weight-I2C

##### Unit ZigBee

#### Module

##### Module13.2 2Relay

##### Module 4EncoderMotor

##### Module13.2 4In8Out

##### Module13.2 4Relay

##### Module13.2 AIN4-20mA

##### Module COMMU

##### Module COMX Cat1

##### Module COMX GSM

##### Module COMX LoRaWAN470

##### Module COMX LoRaWAN915

##### Module COMX LTE

##### Module COMX NBIoT

##### Module COMX Zigbee

##### Module DCMotor

##### Module13.2 Display

##### Module13.2 Dual Kmeter

##### Faces Calculator

##### Faces Encoder

##### Faces Finger

##### Faces Gameboy

##### Faces Joystick

##### Faces Keyboard

##### Faces RFID

##### Module GNSS

##### Module13.2 GoPlus

##### Module13.2 GoPlus2

##### Module GPS

##### Module GPS v2.0

##### Module13.2 GRBL

##### Module HMI

##### IoT Base CatM

##### IoT Base NBIoT

##### Module13.2 LAN

##### Module LoRa433

##### Module LoRa868

##### Module Plus

##### Module13.2 PPS

##### Module13.2 RS232

##### Module Servo

##### Module13.2 Servo2

##### Module Stepmotor

##### Module13.2 Stepmotor Driver

##### Module USB

##### Module LLM

#### Base

##### PM2.5

##### Base LAN

##### Base DMX

##### Base X

#### EzData 1.0

##### EzData blockly

##### Remote+

##### Remote(old version)

#### MediaTrans

##### Atom Printer

##### Audio

##### Echo STT

##### Timer Camera

#### Blockly Custom

### UiFlow2 Development Guide

#### IDE Layout Introduction

#### UI Editor

##### Custom Fonts

#### Project Management

##### Import & Export Project

##### Project Zone

##### File Management

#### Device Security & Sharing

##### Device Sharing

### UiFlow2 Blockly

#### UiFlow2 API Docs

#### EzData 2.0

##### EzData 2.0

##### EzData blockly

## Atom Printer

Function Description

The default firmware of Atom Printer will automatically connect to the server after configuring the Wi-Fi connection. Other devices can be controlled remotely by using Atom Printer Block in UIFlow by configuring the same token as the device.

## Example

![](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/assets/img/uiflow/blockly/media_trans/atom_printer/uiflow_block_mqtt_printer_example.svg)

```python
from m5stack import *
from m5ui import *
from uiflow import *
from MediaTrans.Mqtt_Printer import Mqtt_Printer

setScreenColor(0x222222)
def buttonA_wasPressed():
  # global params
  mqtt.text_print('Hello', 10, 0)
  pass
btnA.wasPressed(buttonA_wasPressed)

mqtt = Mqtt_Printer('94:B9:7E:AC:41:81')
mqtt.start()
```

## API

![](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/assets/img/uiflow/blockly/media_trans/atom_printer/uiflow_block_mqtt_printer_set_topic.svg)

```python
from MediaTrans.Mqtt_Printer import Mqtt_Printer
mqtt = Mqtt_Printer('94:B9:7E:AC:41:81')
mqtt.start()
```

- Setting the Topic (Mac address) of the Atom Printer device
![](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/assets/img/uiflow/blockly/media_trans/atom_printer/uiflow_block_mqtt_printer_print_text.svg)

```python
mqtt.text_print('Hai', 10, 0)
```

- Controls the printing of text messages and sets the location of the print coordinates.
![](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/assets/img/uiflow/blockly/media_trans/atom_printer/uiflow_block_mqtt_printer_bar_print.svg)

```python
mqtt.bar_print('1234')
```

- Control Printing BarCode
![](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/assets/img/uiflow/blockly/media_trans/atom_printer/uiflow_block_mqtt_printer_qr_print.svg)

```python
mqtt.qr_print('1234')
```

- Control Printing QRCode