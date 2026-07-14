## Release list

[lovyan03](https://github.com/lovyan03) released this 09 Jul 05:31

[0.2.25](https://github.com/m5stack/M5GFX/tree/0.2.25)

[`ad9b814`](https://github.com/m5stack/M5GFX/commit/ad9b814264d4e2000e9f30070002310bbccaffc9)

## What's Changed

- Add ESP32C61 platform support. by [@TinyuZhao](https://github.com/TinyuZhao) in [#208](https://github.com/m5stack/M5GFX/pull/208)
- Add some new board id. by [@TinyuZhao](https://github.com/TinyuZhao) in [#209](https://github.com/m5stack/M5GFX/pull/209)
- fix: guard against null DMA buffer in Panel\_AMOLED\_Framebuffer::display() by [@ACEFGI](https://github.com/ACEFGI) in [#211](https://github.com/m5stack/M5GFX/pull/211)
- Harden DMA buffer allocation against transient heap exhaustion in [#212](https://github.com/m5stack/M5GFX/pull/212)
- Add M5StampC5 board detection by [@hlym123](https://github.com/hlym123) in [#214](https://github.com/m5stack/M5GFX/pull/214)
- Sync code with LovyanGFX in [#216](https://github.com/m5stack/M5GFX/pull/216)
- Fix panel transaction shutdown order in [#210](https://github.com/m5stack/M5GFX/pull/210)

**Full Changelog**: [0.2.24...0.2.25](https://github.com/m5stack/M5GFX/compare/0.2.24...0.2.25)

[0.2.24](https://github.com/m5stack/M5GFX/releases/tag/0.2.24)

[lovyan03](https://github.com/lovyan03) released this 24 Jun 11:13

[0.2.24](https://github.com/m5stack/M5GFX/tree/0.2.24)

[`c64a65e`](https://github.com/m5stack/M5GFX/commit/c64a65e37df099e734c4ab47a991c73ac83a8fbe)

[0.2.22](https://github.com/m5stack/M5GFX/releases/tag/0.2.22)

[lovyan03](https://github.com/lovyan03) released this 27 May 00:15

[0.2.22](https://github.com/m5stack/M5GFX/tree/0.2.22)

[`3944fed`](https://github.com/m5stack/M5GFX/commit/3944fedcaa7999b3c8943dfce972d804cb6dacc5)

- add support StopWatch.
- add support PaperMono.
- remove LVGL font support. ( If necessary, please use the LGFX\_Fonts library in conjunction with this. )

[0.2.20](https://github.com/m5stack/M5GFX/releases/tag/0.2.20)

[lovyan03](https://github.com/lovyan03) released this 21 Apr 05:12

[0.2.20](https://github.com/m5stack/M5GFX/tree/0.2.20)

[`d8074bb`](https://github.com/m5stack/M5GFX/commit/d8074bbac8b75d22149d128d71d55b6a34dbc808)

## What's Changed

- Add PaperColor support.
- Add additional colors including gray/grey variations and complete col… by [@yuyun2000](https://github.com/yuyun2000) in [#180](https://github.com/m5stack/M5GFX/pull/180)
- Add CI and fix compile error by [@lovyan03](https://github.com/lovyan03) in [#182](https://github.com/m5stack/M5GFX/pull/182)
- Add lvgl font support. by [@lbuque](https://github.com/lbuque) in [#184](https://github.com/m5stack/M5GFX/pull/184)

**Full Changelog**: [0.2.19...0.2.20](https://github.com/m5stack/M5GFX/compare/0.2.19...0.2.20)

[0.2.19](https://github.com/m5stack/M5GFX/releases/tag/0.2.19)

[lovyan03](https://github.com/lovyan03) released this 23 Jan 09:59

[0.2.19](https://github.com/m5stack/M5GFX/tree/0.2.19)

[`53a7184`](https://github.com/m5stack/M5GFX/commit/53a7184601f3667b030ba141c58b87ce2acfaa2a)

## What's Changed

- Change sticks3 detect method. by [@hlym123](https://github.com/hlym123) in [#175](https://github.com/m5stack/M5GFX/pull/175)
- Add Unit PoEP4 board id. by [@TinyuZhao](https://github.com/TinyuZhao) in [#176](https://github.com/m5stack/M5GFX/pull/176)
- Sync updates from LovyanGFX by [@lovyan03](https://github.com/lovyan03) in [#177](https://github.com/m5stack/M5GFX/pull/177), [#178](https://github.com/m5stack/M5GFX/pull/178)
- 0.2.19 by [@lovyan03](https://github.com/lovyan03) in [#179](https://github.com/m5stack/M5GFX/pull/179)

**Full Changelog**: [0.2.18...0.2.19](https://github.com/m5stack/M5GFX/compare/0.2.18...0.2.19)

[0.2.17](https://github.com/m5stack/M5GFX/releases/tag/0.2.17)

[lovyan03](https://github.com/lovyan03) released this 06 Nov 05:04

[0.2.17](https://github.com/m5stack/M5GFX/tree/0.2.17)

[`7614fa3`](https://github.com/m5stack/M5GFX/commit/7614fa3961b8d6e797ccbfa2ef73359698236c03)

- Fixing the problem of I2C not working correctly for ESP-IDF v5.4
- Fixing the problem of PaperS3 not working correctly for ESP-IDF v5.4
- Fix compile error