# Tasks

## TODO

- [ ] Add tasks here

- [ ] Trace Panel_EPD::init() and post_init() to understand when _buf is actually allocated and whether clear_display triggers it
- [ ] Build minimal reproducer: M5.begin + delay(2000) + fillScreen on core 0 only, no SPIFFS, no tasks. Does it crash?
- [ ] Check if setRotation(1) frees/reallocates _buf in Panel_EPD
- [ ] Try calling M5.Display.startWrite() + endWrite() immediately after M5.begin() as a warmup before SPIFFS init
- [ ] Compare M5GFX debug log output between working 0078 and crashing 0080 boot sequences
- [ ] Print heap_caps_get_free_size(MALLOC_CAP_SPIRAM) before the crash to rule out PSRAM exhaustion
