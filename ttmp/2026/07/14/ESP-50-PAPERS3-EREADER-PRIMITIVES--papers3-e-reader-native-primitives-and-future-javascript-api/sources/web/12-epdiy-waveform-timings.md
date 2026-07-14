The waveform is a look up table with timings for the epaper display controller to determine how to
drive the pixels.

The waveform file is independent to the resolution. With an incorrect or bad waveform, the screen
should at least display some recognizable image, although might be distorted, and with Grays
switched or half display only rendered in some cases. But it should do something. A display that
does not do anything at all it's not fault of the bad waveform, it should be another issue.

This WiKi page is under construction. Is actually only a draft of ideas until it get's some form
and code examples.

### How epdiy waveforms work

A phase (row) always consists of 16 "slots", where each slot has **4 bytes**. Each phase determines
how to pass from one pixel color to the other. For example from white to complete black, and
determines whether to darken, lighten the pixel, or do nothing for each of this 16 slots. This
actually determines what voltages are applied +15 or -15 making the eink particle to move up
(darken) or down (lighten). These **four bytes** actually represent tuples of 2 bits each, where
these tuples represent what to do: 00 could be no-op, 01 is probably to make lighter and 10 is
probably to make darker, while 11 is probably ignored or no-op again. So basically, a row from
[Zephray's](https://github.com/zephray/NekoInk/wiki) csv format, which looks like this:
0,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0 represents the operations needed to go from white to black are
actually encoded in a column(?) in the epdiy header. e.g. 2,2,0,0 would become 10, 10, 00, 00 in
binary (10100000) which equals to 0xA0 in hex. Hence, 16 operations can be packed into 4 bytes.

Now there is just two things left to do until [aceisace](https://github.com/aceisace) can finish
his parser:

Question: I'm still not quite sure about the transitions from one grayscale to another. Could you
explain it more concretely with this line:
{{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,
0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{
0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x
55,0x55},{0x15,0x55,0x55,0x55},{0x15,0x55,0x55,0x54}}

Valentin reply on this point:

> For each pixel, we first choose the inner chunk according to the target grayscale. E.g., if we
want to go from 2 to 9 (zero-based), we first choose the 3rd 4-byte array from the outer array.
Then, we look up the origin grayscale in the inner array: Since the target is 9, we need the second
operation tuple of the third byte, in this case 01. So this is the operation to push to the display
for this particular pixel. We need to do this for every pixel for every phase of the waveform. This
is why it's so hard to make it fast on an ESP Regarding the waveform timings: This is a clutch for
epdiy V1-V6 to use less cycles to draw an image. Normally, each frame that is sent to the display
has exactly the same timing and only the direction of the voltage applied is different. To save
some cycles when going directly from white or black, my idea was to modify the timing so one frame
brings the particles exactly to the next gray level. The time is the high time of the CKV time in
10s of microseconds, which controls for how long the line driver is active. The timings do not come
from a waveform file, I just made them up through experimentation. Hence in the parsed waveforms
they are NULL.

#### Difference of v6 (esp32) versus v7 (esp32s3)

Main difference of operation in this two PCBs is that v7 uses the **LCD module** and v6 uses the
**I2S peripheral** in LCD transmission mode. IMPORTANT Note: Currently, **v7** ignores the timings
in the epdiy waveforms, making them not work correctly in some cases. The vendor waveforms should
work though, because they assume constant timing.

#### Research articles

- [Issue 226](https://github.com/vroland/epdiy/issues/226)
- [Zephray's NekoInk project WiKi](https://github.com/zephray/NekoInk/wiki/Waveform-for-the-screen)
- [Python waveform parser](https://github.com/aceisace/python-waveform-parser)
