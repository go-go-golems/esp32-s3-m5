# Factory V0.5 operator observations

Date: 2026-07-14
Board: PaperS3, USB Serial/JTAG MAC `d0:cf:13:16:17:dc`
Firmware: official M5Stack `C139-PaperS3-FactoryTest-V0.5_0x0.bin`

## Boot sequence disposition

- Version text: no separate disposition reported.
- QUALITY full black (2 seconds): tentative visual failure; the operator reported that it appeared to have the same kind of whole-black issue as the qualification firmware.
- QUALITY full white (2 seconds): pending operator disposition.
- Sixteen grayscale bars (2 seconds): pending operator disposition.
- Factory dashboard: visual pass with qualification; the operator described it as decently crisp, at least for text and similar sparse content.

Operator report (verbatim):

> i think it looks like it might have had the same kind of issues when doing a whole black view.
>
> the final dashboard is decently crisp at least for text and such.

## Questions for the operator

1. Was factory full black actually dark, and was it uniform or textured/gradient-filled?
2. Did factory full white erase the black cleanly?
3. Were all sixteen grayscale bars distinguishable and monotonic from white to black?
4. Is the final dashboard readable with dark, crisp text and shapes?
5. If the transient sequence was missed, reset once and observe the same fixed sequence again.

Automatic flashing success is not a visual pass. The full-black stage has a tentative failure disposition and the dashboard has a qualified pass; white and grayscale remain pending.
