I recently wrote about a [MicroPython library for Adafruit thermal printer](https://kapusta.cc/2017/12/11/thermal-printer-library-for-micropython/). I was able to test it with the LoPy version 1 board from [Pycom](http://pycom.io/), which is a bit resource-constrained. While 512kB of RAM looks fine on paper, you get roughly 10 times less right after booting up:

```
MicroPython v1.8.6-849-g0af003a4 on 2017-11-24; LoPy with ESP32
Type "help()" for more information.
>>> import gc
>>> gc.mem_free()
57296
```

And this is **with an empty** **`main.py`** **file**. The printer library itself is quite heavy and once you initialize the printer and set up WiFi connection, you'll have around 36kB available. This makes printing bitmaps (including QR codes) a bit problematic, especially the way it had been originally implemented in the Adafruit library, i.e. by loading the whole bitmap into memory and sending it to the printer. I skipped this feature for the initial version of MicroPython lib and left it for later.

Now that I have fixed issues around regular text printing, the time has come to do something about bitmaps. I'll still wait some time for [LoPy4](https://pycom.io/product/lopy4/) boards, plus it surely is an interesting challenge to try getting it to work on LoPy v1.

## The bitmap format

The thermal printer is a rather crude image-printing device when you think of it. In fact, it allows for two colors: black and no color. So, precisely, it's one color... But because of that, the bitmap can be represented as an array of bits, where 1 means *black dot* and 0 means *blank dot*.

The printer's datasheet defines a couple of different mechanisms for printing bitmaps. The simplest one (that was implemented in the original Python library) can be described as follows:

1. Send the appropriate command to start printing bitmap
2. Pass the number of lines (the height of the bitmap)
3. Pass the number of columns (the width of the bitmap) in bytes (i.e. width in pixels divided by 8)
4. Send the bitmap data as a sequence of bytes. The printer would add line feeds where appropriate, based on the number of rows.

The example bitmaps for Python library are defined in the code as arrays of integers. A 75x75px Adafruit logo is a 750B array – 75 lines per 10 bytes (required to code 75 dots), while a 135x135px Adafruit QR is over 2kB of raw data. It turns out that loading so many bytes at once into LoPy's memory is a no-go.

When I dug deeper into the code I realized that my `printBitmap()` function had been a bit buggy. Once I fixed it I was able to print the smaller bitmap straight away! But it was still failing to allocate memory when trying to load the big QR code bitmap.

## Printing from file

I tried streaming a bitmap from a file on disk. I stored the bitmap bytes in a file, and then passed it as an argument to `printBitmapFromFile()` function like this:

```python
# first two args are width and height in pixels
printer.printBitmapFromFile(135, 135, '/flash/lib/qrcode')
```

It would open the file, read it in one-line chunks and send the data to printer. This way only a small part of data would be kept in memory at once, effectively allowing for processing the whole bitmap.

In fact, this time I didn't get a memory allocation problem, but this is how the bitmap was printed:

\[caption id="attachment\_278" align="aligncenter" width="446"\] ![IMG_3169.JPG](https://kapusta.cc/assets/img_31691.jpg) The QR code as seen by thermal printer – 2017, colorized.\[/caption\]

Still suspecting some memory issue, I tried reading only part of the file, limiting the number of lines passed as a second argument to `printBitmapFromFile()`:

```python
printer.printBitmapFromFile(135, 35, '/flash/lib/qrcode')
printer.printBitmapFromFile(135, 55, '/flash/lib/qrcode')
printer.printBitmapFromFile(135, 85, '/flash/lib/qrcode')
printer.printBitmapFromFile(135, 105, '/flash/lib/qrcode')
printer.printBitmapFromFile(135, 120, '/flash/lib/qrcode')
printer.printBitmapFromFile(135, 128, '/flash/lib/qrcode')
```

![IMG_3171](https://kapusta.cc/assets/img_3171.jpg)

Apparently, the printer was able to handle smaller bitmaps but failed once the total size exceeded ~120 lines, 17 bytes each – which is 2040 bytes, and it may be meaningful or just a coincidence. I yet have to figure out whether 2kB is the real maximum limit of bitmap chunk size that the printer accepts.

The `printBitmap()` (and my file-based variation of it) has the optional `LaaT` argument - it stands for *line-at-a-time* and means sending a separate print command for every single line of the bitmap, instead of one command for the whole image. Enabling this flag fixed the printout for me, however using it slows the printing down, so it's not an optimal solution.

I ended up setting a maximum single chunk of a bitmap to 50 lines, i.e. the 135px high bitmap would be printed in 3 chunks. This is obviously transparent to the user and not visible on the printout. The value of 50 lines is arbitrary, it works for me so far, but given the 2kB hypothesis is right, it would fail for a full-page-width bitmap (384 dots which is 48 bytes, and times 50 it makes 2400 bytes). I'm going to verify it later on.

\[caption id="attachment\_280" align="aligncenter" width="458"\] ![IMG_3153](https://kapusta.cc/assets/img_3153-e1513442570290.jpg) At last!\[/caption\]

## Next steps

Having sorted out the basic issues around printing bitmaps, I have the following on my to-do list:

- an algorithm to convert actual image files to printer bitmap format (perhaps a.bmp format would be a good starter); once it's ready, experiment with different images to fix and optimize bitmap printing code
- an algorithm to create QR codes in printer bitmap format

Both of the above would enable some real-world use cases for the thermal printer, so stay tuned for more:)

You'll find the up-to-date library on [GitHub](https://github.com/ayoy/micropython-thermal-printer).