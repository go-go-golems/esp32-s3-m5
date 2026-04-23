The M5Stack ATOM thermal printer kit is a desktop DIY thermal printer comprised of the company’s ATOM Lite IoT controller equipped with ESP32-Pico-D4 system-in-package and a 58mm thermal printer housed in a cardboard package.

The printer can print text, graphics, barcodes, or QR code printings, and the pre-installed firmware offers two modes of operation with “AP Connect Print” where the printer is seen as an access point and can be controlled with a smartphone or computer from a web browser, and the “MQTT Notifications” mode that prints the content of MQTT messages.

[![M5Stack ATOM Thermal Printer Kit](https://www.cnx-software.com/wp-content/uploads/2021/11/M5Stack-ATOM-Thermal-Printer-Kit-720x553.jpg)](https://www.cnx-software.com/wp-content/uploads/2021/11/M5Stack-ATOM-Thermal-Printer-Kit.jpg) Highlights of the ATOM thermal printer kit:

- [M5Stack ATOM Lite](https://www.cnx-software.com/2020/03/26/m5stack-atom-is-a-compact-fully-integrated-esp32-development-kit/#m5stack-atom-lite) IoT controller with ESP32-Pico-D4 WiFi and Bluetooth SiP fitted with 4MB Flash
- 58mm thermal printer connected over UART (9600 bps 8N1)
	- Supports for text/graphics/BarCode/QRCode
		- Speed – 60mm/s 203dpi 8 dots/mm up to 384 dots per line
- Connectivity over WiFi
	- AP hotspot connection, web-controlled printing
		- Printing content sent via MQTT (Topic is device MACac address)
- Power Supply – 12V/2.5A recommended (not included)
- Dimensions – 151 x 79 x 66mm
- Weight – 285 grams

[![ESP32 Thermal Printer](https://www.cnx-software.com/wp-content/uploads/2021/11/ESP32-Thermal-Printer-720x503.jpg)](https://www.cnx-software.com/wp-content/uploads/2021/11/ESP32-Thermal-Printer.jpg) The kit also includes a roll of paper beside the IoT controller, thermal printer, and cardboard packaging. The company supports both Arduino and UIFlow visual programming for development and the Arduino firmware source code can be [found on Github](https://github.com/m5stack/ATOM-PRINTER). There’s also a [documentation page](https://docs.m5stack.com/en/atom/atom_printer) with additional information on QR Code, barcode, BMP graphics printing capabilities.

The video below demonstrates the main features of the printer with AP and MQTT mode, the printing of various types of content, as well as the instructions to replacement the thermal paper roll.

M5Stack sells the ATOM thermal printer kit [for $59 on their website](https://shop.m5stack.com/products/atom-thermal-printer-kit). This is certainly not the first ESP32 thermal printer solution, as there are various implementations including [bitbank2 thermal printer Arduino](https://github.com/bitbank2/Thermal_Printer) connecting ESP32 and nRF52 boards to the printer over Bluetotoh LE, or a [Arduino sketches](https://github.com/lorot19/ThermalPrinter) to print bitmaps over serial or MQTT.

**Support CNX Software! Donate via [cryptocurrencies](https://www.cnx-software.com/donate-cryptocurrencies/), [become a Patron](https://www.patreon.com/cnxsoft) on Patreon, or purchase goods on [Amazon](https://amzn.to/3SXubZ0) or [Aliexpress](https://s.click.aliexpress.com/e/_DmGIIRT)**. We also use affiliate links in articles to earn commissions if you make a purchase after clicking on those links.