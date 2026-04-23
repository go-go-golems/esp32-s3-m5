Publish Date: 2021-11-26  
Last Modified: 2025-07-08  

## ATOM Thermal Printer Kit

![img](https://m5stack.lang-ship.com/images/shop/K118.png)

- SKU: K118
- Shop: [https://shop.m5stack.com/products/atom-thermal-printer-kit?ref=langship](https://shop.m5stack.com/products/atom-thermal-printer-kit?ref=langship)
- Shop(Japan): [https://www.switch-science.com/products/7630](https://www.switch-science.com/products/7630)
- Document: [https://docs.m5stack.com/en/atom/atom\_printer](https://docs.m5stack.com/en/atom/atom_printer)
- Release Date(probably): 2021-11-26

ATOM Printer is a desktop DIY thermal printer that offers excellent value for its features and capabilities. It includes an ESP32-Pico based IoT controller called ATOM Lite, as well as a 58mm thermal printer, making it suitable for various printing requirements.The printer supports printing text, graphics, barcodes, and QR codes, allowing for versatile printing applications. Whether you need to print labels, receipts, tickets, or other types of content, the ATOM Printer can handle it.ATOM Lite serves as the brains of the printer, providing control and connectivity features. It is based on the ESP32-Pico chip, which offers powerful processing capabilities and supports various communication protocols.ATOM Printer comes with built-in firmware that offers two modes: AP Connect Print and MQTT Notifications Print. The AP Connect Print mode allows you to connect to the printer directly via Wi-Fi access point mode, enabling easy configuration and printing. The MQTT Notifications Print mode allows you to receive notifications and print content from MQTT (Message Queuing Telemetry Transport) sources, adding a level of automation and integration with other IoT devices or systems.

### Datasheet

- ATOM\_PRINTER\_CMD\_v1.06 PDF: [ATOM\_PRINTER\_CMD\_v1.06.pdf](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/atombase/atom_pritner/ATOM_PRINTER_CMD_v1.06.pdf)

## Review

### Box

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1810.jpg)

It is in a plastic case.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1811.jpg)

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1812.jpg)

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1813.jpg)

A printer and ATOM are included as a set.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1814.jpg)

It’s the back side. The command is written.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1815.jpg)

I removed ATOM. This board connects the printer and ATOM. Also step down from 12V to 5V.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1816.jpg)

I opened the box. Please remove ATOM. Spare paper is also included.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1817.jpg)

It is a printer. TTL (UART), RS232, USB can be used.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1819.jpg)

I opened the lid.

### Power ON

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1853.jpg)

AC adapter is not included. Requires 2.5A at 12V.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1854.jpg)

The plug is 5.5mmx2.1mm and is center plus.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1820.jpg)

When you turn on the power, it will start up as a Wi-Fi access point.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1821.jpg)

I connected with my smartphone. This screen will appear automatically.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1822.jpg)

You can print from your browser.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1824.jpg)

Printing is not possible with a 1A power supply.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1832.jpg)

Press the button on the printer to feed the paper. Press and hold to self-test.

### Adafruit Thermal Printer Library

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1833.jpg)

Characters can be printed. Images cannot be printed. The command of the image seems to be different.

### M5Stack Library

- [https://github.com/m5stack/ATOM-PRINTER](https://github.com/m5stack/ATOM-PRINTER)

### Image printing

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1844.jpg)

Is possible.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1845.jpg)

If there is a lot of black, a line will be included.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/IMG_1846.jpg)

It disappears when Wait is entered.

### Image printing method

### Image to Data

Prepare a Jpeg, PNG or BMP file.

- [https://m5stack.lang-ship.com/tools/image2data/?format=1bit\_2](https://m5stack.lang-ship.com/tools/image2data/?format=1bit_2)

There is a conversion page in Tools on this site.

![](https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/m5.png)

Let’s convert this 32x32 image.

```c
// m5.png
// https://m5stack.lang-ship.com/tools/image2data/

const uint16_t imgWidth = 32;
const uint16_t imgHeight = 32;

// 1bit Dump
const unsigned char img[128] PROGMEM = {
0b11111111, 0b11111111, 0b11111111, 0b11111111,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000111, 0b00000111, 0b00011111, 0b11000001,
0b10000111, 0b00000111, 0b00010000, 0b00000001,
0b10000111, 0b10000111, 0b00010000, 0b00000001,
0b10000111, 0b10001111, 0b00010000, 0b00000001,
0b10000110, 0b10001011, 0b00011111, 0b00000001,
0b10000110, 0b11011011, 0b00000001, 0b10000001,
0b10000110, 0b01110011, 0b00000000, 0b11000001,
0b10000110, 0b01110011, 0b00000000, 0b11000001,
0b10000110, 0b00100011, 0b00000000, 0b11000001,
0b10000110, 0b00000011, 0b00000000, 0b11000001,
0b10000110, 0b00000011, 0b00100001, 0b10000001,
0b10000110, 0b00000011, 0b00011111, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b10000000, 0b00000000, 0b00000000, 0b00000001,
0b11111111, 0b11111111, 0b11111111, 0b11111111,
};
```

I was able to convert.

```c
void Print_BMP(int width, int height, const unsigned char *data, int mode, int wait) {
AtomSerial->write(0x1D);
AtomSerial->write(0x76);
AtomSerial->write(0x30);                      // 0
AtomSerial->write(mode);                      // m
AtomSerial->write((width / 8) & 0xff);        // xL
AtomSerial->write((width / 256 / 8) & 0xff);  // xH
AtomSerial->write((height) & 0xff);           // yL
AtomSerial->write((height / 256) & 0xff);     // yH
for (int i = 0; i < (width / 8 * height); i++) {
AtomSerial->write(data[i]);                 // data
delay(wait);
}
}
```

Create a function like this.

```c
Print_BMP(32, 32, img);
```

call!

The maximum width is 384.