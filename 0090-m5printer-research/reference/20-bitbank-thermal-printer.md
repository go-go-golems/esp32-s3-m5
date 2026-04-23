Bluetooth Low Energy  
Thermal Printer Library  
Copyright (c) 2020 BitBank Software, Inc.  
Written by Larry Bank  
[bitbank@pobox.com](mailto:bitbank@pobox.com)  
  
[![printer demo](https://github.com/bitbank2/Thermal_Printer/raw/master/ble_prt.jpg?raw=true "Thermal_Printer")](https://github.com/bitbank2/Thermal_Printer/blob/master/ble_prt.jpg?raw=true)

  
This Arduino library allows you to easily generate text and graphics and send them to various BLE thermal printers. I can support the printers that I currently own; if your model is not yet supported, the best way to encourage me to support it is to send me funds to buy it.  
There are many different BLE APIs depending on the board manufacturer, I support the three more popular ones: ESP32, Adafruit and Arduino (e.g. Nano BLE 33). The three main features of thermal printers are supported by this code - plain text, barcodes (1D+2D) and dot addressable graphics. The graphics are treated like a display driver - you define a RAM buffer and draw text, dots, lines and bitmaps into it, then send it to the printer. Text output supports the various built-in font size+attribute options of some printers as well as Adafruit\_GFX format bitmap fonts (sent as graphics). See the Wiki: [https://github.com/bitbank2/Thermal\_Printer/wiki/Public-API](https://github.com/bitbank2/Thermal_Printer/wiki/Public-API) for a description of each function.  
  
Features  
\========  
\- Supports the GOOJPRT PT-210, MTP-3, PeriPage+ and 'cat' printers (so far)  
\- Compiles on ESP32, Adafruit nRF52 and Arduino boards with bluetooth  
\- Supports graphics (dots, lines, text, bitmaps), 1D + 2D barcodes, and plain text output  
\- Allows printing Adafruit\_GFX fonts one line at a time or drawing them into a RAM buffer  
\- Can scan/connect to printers by BLE name or auto-detect the supported models  
\- Doesn't depend on any other 3rd party code  
  

Here is a subjective chart of the printer models I've tested and are supported by this code. Please feel free to send me info about other models that work and additional comments about these printers.  

  

[![](https://github.com/bitbank2/Thermal_Printer/raw/master/printer_chart.jpg?raw=true)](https://github.com/bitbank2/Thermal_Printer/blob/master/printer_chart.jpg?raw=true)

  

If you find this code useful, please consider becoming a Github Sponsor or sending a single donation.

[![paypal](https://camo.githubusercontent.com/e1ff554a09e8e92bef25abc553ff05b88f45afd695877cf12f3a46558ef65b2e/68747470733a2f2f7777772e70617970616c6f626a656374732e636f6d2f656e5f55532f692f62746e2f62746e5f646f6e61746543435f4c472e676966)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=SR4F44J2UR8S4)