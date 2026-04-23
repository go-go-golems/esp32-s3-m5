Beginner

Project guide

## Creating Your Own Images

You need to prepare your images as black & white (also known as 1BPP) files in "bmp" or "pbm" format. The width must be exactly 384 pixels, while the height can be unlimited (as long as the file fits on the CLUE **CIRCUITPY** drive).

Different image and photo editing programs have different steps to prepare an image. Here you can see how to use the [free and open source GIMP photo editing software](https://www.gimp.org/) to prepare an image, but you should be able to use any software that can write compatible "bmp" files!

[![circuitpython_Screenshot_2021-09-28_14-23-28.png](https://cdn-learn.adafruit.com/assets/assets/000/104/886/medium640/circuitpython_Screenshot_2021-09-28_14-23-28.png?1632857272)](https://learn.adafruit.com/assets/104886)

Open your original image.

[![circuitpython_Screenshot_2021-09-28_14-28-16.png](https://cdn-learn.adafruit.com/assets/assets/000/104/887/medium640/circuitpython_Screenshot_2021-09-28_14-28-16.png?1632857323)](https://learn.adafruit.com/assets/104887)

Resize the image to be 384 pixels wide. Let your image editor calculate the height automatically or the image will be distorted.

[![circuitpython_Screenshot_2021-09-28_14-29-38.png](https://cdn-learn.adafruit.com/assets/assets/000/104/888/medium640/circuitpython_Screenshot_2021-09-28_14-29-38.png?1632857394)](https://learn.adafruit.com/assets/104888)

Convert the image to 1 bit per pixel, using your choice of dithering methods

[![circuitpython_Screenshot_2021-09-28_14-30-13.png](https://cdn-learn.adafruit.com/assets/assets/000/104/889/medium640/circuitpython_Screenshot_2021-09-28_14-30-13.png?1632857432)](https://learn.adafruit.com/assets/104889)

Export the image as a "bmp" file on the **CIRCUITPY** drive. Your CLUE will automatically reset, and you can choose the newly uploaded file for printing!

Page last edited March 08, 2024

Text editor powered by [tinymce](https://www.tiny.cloud/).