For those of you who do custom projects with parallel Eink displays (e.g. e-reader panels), you're
the target audience. Until now, getting 16 good looking gray levels on your panel was a "dark art"
full of misinformation and gate keeping. Just to clarify, there is no such thing as a "waveform"
with these displays. Internally, it's a digital logic state machine which has a row counter and row
buffer. You write digital data in parallel to the row buffer which has 2-bit codes for each
"pixel". The codes are: 00/11 = neutral, 01 = darken, 10 = lighten. These are actually the bits
which turn on the transistors which control the positive and negative electric field pushing the
pixels up and down. Each push or "pass" only gets the pixels partially toward the color you want.
Usually 5-6 pushes will get them fully seated (full black or full white). To get gray levels you
need to make each pixel 'dance' a little to settle in a middle position. Until now, EInk (the
company) only shared their precious grayscale 'wave table' with big customers and it contained
lists of many passes (up to 50) depending on the starting gray shade, destination gray shade and
ambient temperature. The colder it is, the higher the viscosity of the clear oil which suspends the
pigment granules. Over the new year holiday, I wrote my own parallel eink library from a blank
slate ([https://github.com/bitbank2/FastEPD](https://github.com/bitbank2/FastEPD)). It's aims are
to be easy to use, easy to read (code), fast, and offer more features/function compared to EPDiy.
I've also simplified the definition+use (and now editing) of the gray matrix which defines the
dance of each pixel to get to a certain gray level. Here's an example table I created for the M5
PaperS3 (0 = full black, 15 = full white):

`const uint8_t u8M5Matrix[] = {`

`/* 0 */     0,  0,  0,  2,  2,  2,  2,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1, 
1,  1,  1,  1,  1,  1,  1,  0,`
`/* 1 */     2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,  0,`
`/* 2 */ 0, 0, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0,`
`/* 3 */ 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  0,`
`/* 4 */ 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0,`
`/* 5 */ 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  0,`
`/* 6 */ 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1, 0, 0,  0,`
`/* 7 */ 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0,  0,`
`/* 8 */ 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 1, 1, 0, 0,  0,`
`/* 9 */ 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0,  0,`
`/* 10 */ 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 1,  0,`
`/* 11 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 1, 0, 0,  0,`
`/* 12 */ 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,  0,`
`/* 13 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,  0,`
`/* 14 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2,  0,`
`/* 15 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,  0`
`};`

Now, to save myself (and you) time, I created an interactive matrix editor using the serial
terminal. This allows you to quickly experiment with gray table values to get it perfect for your
display panel (each behaves slightly differently). Here is what you see in the serial terminal when
you type HELP:

[https://preview.redd.it/4jw14qm7heue1.png?width=834&format=png&auto=webp&s=f0abf3db509e77491de515ee
a07de43531031fd8](https://preview.redd.it/4jw14qm7heue1.png?width=834&format=png&auto=webp&s=f0abf3d
b509e77491de515eea07de43531031fd8)

Here is the Arduino sketch for the matrix editor:

[https://github.com/bitbank2/FastEPD/tree/main/examples/Arduino/gray\_matrix\_editor](https://github
.com/bitbank2/FastEPD/tree/main/examples/Arduino/gray_matrix_editor)
